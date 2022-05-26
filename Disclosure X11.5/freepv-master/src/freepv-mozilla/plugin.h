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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "pluginbase.h"

#include <libfreepv/Platform.h>
#include <libfreepv/Parameters.h>

// #include "parameters.h"
// #include "browserinterface.h"
// #include "viewerkernel.h"

class nsPluginInstance : public nsPluginInstanceBase, public FPV::Platform
{
public:
  // modified constructor to receive the plugin creation data
  nsPluginInstance(nsPluginCreateData *pcd);

  ~nsPluginInstance();

  virtual NPBool init(NPWindow* aWindow) = 0;
  virtual void shut() = 0;
  virtual NPBool isInitialized();

  // locals
  const char * getVersion();

protected:
  NPP mInstance;
  NPBool mInitialized;

  nsPluginCreateData *pCData;

  bool ignoreThisNewStream;

public:

    // these functions are called by the browser
	NPError NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype);
	NPError DestroyStream(NPStream* stream, NPError reason);
	int32 WriteReady(NPStream* stream);
	int32 Write(NPStream* stream, int32 offset, int32 len, void* buffer);
    void StreamAsFile(NPStream* stream, const char* fname);

protected:


    // =====================================================
    // from here: common code for netscape plugin on all
    // platforms


    // =====================================================
    // from here: implementation of FPV::Platform
	
public:

    /** quit the program.
     * 
     *  Should be called as response to EventListener::onDestory()
     *  @param ret the return code returned to the shell, if
     *             this is not a browser plugin
     */
    virtual void quit(int ret) {};


    ////////////////////////////////////////////////

    /** Download an URL to memory.
     *
     *  It might report the download progress with
     *  onDownloadProgress().
     *  onDownloadComplete() will be called once the download
     *  has been completed.
     */
    virtual bool startDownloadURL(const std::string & url);

    /** Download an URL to a file.
     *
     *  It might report the download progress with
     *  onDownloadProgress().
     *  onDownloadComplete() will be called once the download
     *  has been completed.
     *
     *  As a special case, if the file is local, it will just
     *  be delivered by onDownloadComplete().
     *
     *  The file should not be deleted after usage.
     */
    virtual bool startDownloadURLToFile(const std::string & url);

    virtual const std::string & currentDownloadURL()
    {
        return m_fpvCurrentURL; 
    };

    std::string currentDownloadMimeType()
    { 
        return m_fpvMimeType;
    }

protected:
    bool m_fpvDownloadToFile;

    // the browser automatically starts a stream to download the object pointed by the "SRC" parameter
    // of the EMBED tag. I want to download other (smaller) data first so
    // this flag is used to identify the automatic download and to abort it
    bool m_fpvIgnoreStreams;

    FPV::Parameters * m_fpvParam;
    unsigned char * m_fpvDownBuffer;
    size_t m_fpvBufferSize;
    // size of file on server
    size_t m_fpvDownloadSize;
    size_t m_fpvDownloadedBytes;
    std::string m_fpvCurrentURL;
    std::string m_fpvMimeType;

};

#endif // __PLUGIN_H__
