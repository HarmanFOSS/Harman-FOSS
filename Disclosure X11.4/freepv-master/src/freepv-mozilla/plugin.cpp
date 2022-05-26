/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include <stdio.h>
#include <cstring>

#include "plugin.h"

#include <libfreepv/utils.h>

#define FPV_CHUNKSIZE 32*1024



//////////////////////////////////////
//
// general initialization and shutdown
// NS_PluginGetValue
// NS_PluginShutdown
// NS_PluginInitialize
// this needs to be implemented for each platform separately...
//


////////////////////////////////////
//
// plugin instance
//

nsPluginInstance::nsPluginInstance(nsPluginCreateData *pcd) : nsPluginInstanceBase(),
  mInstance( pcd->instance ),
  mInitialized(FALSE)
{
    DEBUG_TRACE("");
    pCData = pcd;

    m_fpvParam = new FPV::Parameters;
    m_fpvParam->parse( pcd->argc, pcd->argn, pcd->argv );
    m_fpvIgnoreStreams = true;
    m_fpvDownloadToFile = true;
    m_fpvDownBuffer = 0;
    m_fpvDownloadSize = 0;
    m_fpvDownloadedBytes = 0;

}

nsPluginInstance::~nsPluginInstance()
{
    DEBUG_TRACE("");
    delete m_fpvParam;
}

NPBool nsPluginInstance::isInitialized()
{
    DEBUG_TRACE("");
    return mInitialized;
}

const char * nsPluginInstance::getVersion()
{
    DEBUG_TRACE("");
    return NPN_UserAgent(mInstance);
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
    DEBUG_TRACE("mime type:" << type);
    if( m_fpvIgnoreStreams ) {
        DEBUG_DEBUG("killing stream");
		return NPERR_GENERIC_ERROR;
	}

    // ignore next stream, if it isnt requested by us:
    m_fpvIgnoreStreams = true;
    
    if (m_fpvDownloadToFile) {
        *stype = NP_ASFILEONLY;
    } else {
        *stype = NP_NORMAL;
    }

    // request memory for the current download
    if (stream->end == 0)
    {
        // memory size not known, start with a small buffer
        m_fpvDownBuffer = (unsigned char *) malloc(FPV_CHUNKSIZE);
        if (!m_fpvDownBuffer) {
            // TODO: notify viewer
            DEBUG_ERROR("could not allocate memory for download buffer");
            return NPERR_GENERIC_ERROR;
        }
        m_fpvBufferSize = FPV_CHUNKSIZE;
    } else {
        // allocate memory for whole stream
        m_fpvDownBuffer = (unsigned char *) malloc(stream->end);
        if (!m_fpvDownBuffer) {
            DEBUG_ERROR("could not allocate memory for download buffer");
            // TODO: notify viewer
            return NPERR_GENERIC_ERROR;
        }
        m_fpvBufferSize = stream->end;
    }
    // no bytes downloaded so far
    m_fpvDownloadedBytes = 0;
    m_fpvMimeType = type;
	return NPERR_NO_ERROR;
}

NPError nsPluginInstance::DestroyStream(NPStream* stream, NPError reason)
{
    DEBUG_TRACE("reason: " << reason);
    // ignore stream destruction if download to file
    if (m_fpvDownloadToFile)
        return NPERR_NO_ERROR;
    switch (reason)
    {
        case NPRES_DONE:
            m_eventListener->onDownloadComplete(m_fpvDownBuffer, m_fpvDownloadedBytes);
            break;
        case NPRES_NETWORK_ERR:
            DEBUG_ERROR("Error receiving stream, network error");
        default:
            m_eventListener->onDownloadComplete(m_fpvDownBuffer, m_fpvDownloadedBytes);
            break;
    }
	return NPERR_NO_ERROR;
}

int32 nsPluginInstance::WriteReady(NPStream* stream)
{
    DEBUG_TRACE("");
    return FPV_CHUNKSIZE;
}

int32 nsPluginInstance::Write(NPStream* stream, int32 offset, int32 len, void* buffer)
{
    DEBUG_TRACE("offset: " << offset << "  length: " << len);
    if (!m_fpvDownloadToFile) {
        // check if the data fits into the buffer...
        if (offset+len >= (int32) m_fpvBufferSize) {
            // allocate more memory
            unsigned char * newmem = (unsigned char*) realloc(m_fpvDownBuffer, m_fpvBufferSize*2);
            if (newmem == 0) {
                fprintf(stderr, "Fatal error, realloc failed\n");
                free(m_fpvDownBuffer);
                // TODO: notify player somehow
                return -1;
            }
            m_fpvDownBuffer = newmem;
            m_fpvBufferSize = m_fpvBufferSize*2;
        }
        // copy data to our buffer.
        memcpy(m_fpvDownBuffer + offset, buffer, len);
    }
    m_fpvDownloadedBytes = offset + len;
    m_eventListener->onDownloadProgress(m_fpvDownBuffer, m_fpvDownloadedBytes, m_fpvDownloadSize);
	return len;
}

void nsPluginInstance::StreamAsFile(NPStream* stream, const char* fname)
{
    DEBUG_TRACE(fname);
    std::cerr<<"File requested: "<<m_fpvCurrentURL<<std::endl;
    std::cerr<<"The downloaded file is "<<fname<<std::endl;
    m_eventListener->onDownloadComplete( fname );
    NPN_DestroyStream(mInstance, stream, NPRES_DONE);
}


// from here: implementation of common FPV::Platform functions


/* Download an URL to memory.
 */
bool nsPluginInstance::startDownloadURL(const std::string & url)
{
    DEBUG_TRACE(url);
    m_fpvCurrentURL = url;
    // request a download
    m_fpvIgnoreStreams = false;
    NPError r = NPN_GetURL( this->mInstance, url.c_str(), NULL );
    if (r != NPERR_NO_ERROR) {
        DEBUG_DEBUG("NPError: " << r);
    }
    m_fpvDownloadToFile = false;
    return (r == NPERR_NO_ERROR);
}

/* Download an URL to a file.
 */
bool nsPluginInstance::startDownloadURLToFile(const std::string & url)
{
    DEBUG_TRACE(url);
    m_fpvCurrentURL = url;
    std::cerr<<"Downloading: "<<m_fpvCurrentURL<<std::endl;
    // request a download
    m_fpvIgnoreStreams = false;
    NPError r = NPN_GetURL( this->mInstance, url.c_str(), NULL );
    if (r != NPERR_NO_ERROR) {
        DEBUG_DEBUG("NPError: " << r);
    }
    m_fpvDownloadToFile = true;
    return (r == NPERR_NO_ERROR);
}
