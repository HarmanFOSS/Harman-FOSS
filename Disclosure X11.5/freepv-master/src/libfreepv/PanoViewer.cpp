/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoViewer.cpp 150 2008-10-15 14:18:53Z leonox $
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of
 * the License
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

#include <config.h>

#include "PanoViewer.h"
#include "OpenGLRenderer.h"
#include "QTVRDecoder.h"
#include "imageReader.h"
#include "Utils/signatures.h"
#include "utils.h"
#include "Behavior.h"
#include "EventProcessor.h"

using namespace FPV;

PanoViewer::PanoViewer()
  : m_state(STATE_NOT_INTIALIZED),
    m_platform(0),
    m_renderer(0),
    m_controller(0),
//    m_currentDownload(0),
//    m_currentDownloadSize(0),
    m_currentCubeFaceDownload(0)

{
    std::cerr << "Starting freepv, revision: $Revision: 150 $ " << std::endl;
    DEBUG_TRACE("");
    m_currentCube=0;
    m_scene = new Scene();
    m_next_scene = NULL;
}

PanoViewer* PanoViewer::Instance(){
    static PanoViewer viewer;
    return &viewer;
}

PanoViewer::~PanoViewer()
{
    DEBUG_TRACE("");
    if (m_renderer)
        delete m_renderer;
    if (m_scene)
        delete m_scene;
}

bool PanoViewer::start()
{
    // create the new controller
    m_controller = new Controller(m_scene);

    // This pointer holds a reference to a parser
    // in case it's needed.
    m_spiv_parser = NULL;

    //The renderer is initialized
    m_renderer->init();

    m_scene->setStatusText("initializing");

    // notice: currently the scene constructs
    // its own uiElement, just motify the renderer here.
    m_renderer->initElement(*(m_scene->getUIElement()));

    // TODO: check for preview image, once it is supported

    // TODO: setup loading progress bar

    // always download to memory...
    if (m_param.getSrc().size() != 0) {
        // single image pano, QTVR, SPi-V or XML file

        // download to file
        FPV_S2S(m_statusMessage, "downloading " << m_param.getSrc());
        m_scene->setStatusText( m_statusMessage );
        redraw();
        changeState(STATE_DOWNLOADING_SRC);
        if (!m_platform->startDownloadURLToFile( m_param.getSrc() ) ) {
            FPV_S2S(m_statusMessage, "download failed: " << m_param.getSrc());
            m_scene->setStatusText( m_statusMessage );
            redraw();
            changeState(STATE_ERROR);
        }
    } else {
        // try to download the cube faces
        if (m_param.getCubeSrc(0).size() > 0) {
        // download singe pano file
	    
            FPV_S2S(m_statusMessage, "downloading: " << m_param.getCubeSrc(0));
            m_scene->setStatusText( m_statusMessage );
            redraw();
            changeState(STATE_DOWNLOADING_CUBEFACES);
            m_currentCubeFaceDownload = 0;
            if (!m_platform->startDownloadURL( m_param.getCubeSrc(0) ) ) {
                FPV_S2S(m_statusMessage, "download failed: " << m_param.getCubeSrc(0));
                m_scene->setStatusText( m_statusMessage );
                redraw();
                changeState(STATE_ERROR);
            }
            return true;
        } else {
            m_statusMessage = "error: no panorama specified";
            m_scene->setStatusText( m_statusMessage );
            redraw();
            changeState(STATE_ERROR);
        }
    }


    return true;
}

bool PanoViewer::init(Platform & platform, const Parameters & para)
{
    DEBUG_TRACE("");
    platform.setListener(this);
    m_platform = &platform;
    m_param = para;

    // create viewer structure
    // TODO: select suitable renderer here
    m_renderer = new OpenGLRenderer(&platform, para.getMaxTexMem(), para.getRenderQuality());
    return true;
}


void PanoViewer::onDestroy()
{
    m_platform->quit(0);
}


void PanoViewer::onResize(Size2D size)
{
    m_renderer->resize(size);

    // render scene
    if (m_renderer) {
        m_renderer->render(*m_scene);
    }
}

void PanoViewer::onTimer(double time)
{
    if (m_controller){
        if (!m_controller->onTimer(time)) {
            // stop redrawing if no animation is needed
            m_platform->stopTimer();
        } else {
            m_platform->startTimer(10);
        }
    }
    if (m_renderer) {
        m_renderer->render(*m_scene);
    }
}


void PanoViewer::onRedraw(int x, int y, int w, int h, int count)
{
    // just render scene
    if (count == 0) {
        if (m_renderer) {
            m_renderer->render(*m_scene);
        }
    }
}


void PanoViewer::onMouseEvent(const MouseEvent & event)
{
    if (m_controller) {
        if (m_controller->onMouseEvent(event)) {
            m_platform->startTimer(10);
        }

        EventProcessor::Instance()->processMouseEvent(event);

	changeScene();
    }

}


void PanoViewer::onKeyEvent(const KeyEvent & event)
{
    if (m_controller) {
        if (m_controller->onKeyEvent(event)) {
            m_platform->startTimer(10);
        }
    }
}


void PanoViewer::onDownloadProgress(void * data, size_t downloadedBytes, size_t size)
{
    if (size) {
        FPV_S2S(m_statusMessage, "downloading " << m_platform->currentDownloadURL() << ", received " << ((int)downloadedBytes) /1024 << " of " << ((int)size)/1024 << " kB.");
    } else {
        FPV_S2S(m_statusMessage, "downloading " << m_platform->currentDownloadURL() << ", received " << ((int)downloadedBytes)/1024 << " kB.");
    }
    m_scene->setStatusText( m_statusMessage );
    redraw();
}

/** notify about a finished download
 *
 * Maybe a special SceneElement factory should be added, when more
 * elements are supported
 */
void PanoViewer::onDownloadComplete(void * data, size_t sz)
{
    Image * img=NULL;
    const char*file_to_download;
    fprintf(stderr, "state %d: %d bytes downloaded\n",(int)m_state, (int)sz);
    // do some useful stuff here.. decode image etc.
    switch (m_state) {
        case STATE_DOWNLOADING_CUBEFACES:
            if (m_currentCubeFaceDownload == 0) {
                // we have downloaded the first cube
                // TODO: Create panorama object (and 5 empty images of the same size)
                FPV_S2S(m_statusMessage, "received first cubeface. preparing rendering");
                DEBUG_TRACE(m_statusMessage);
                m_scene->setStatusText( m_statusMessage );
                redraw();
                
            }
            {

	    if (!decodeImage((unsigned char*) data, sz, img, "AUTO")) {
                delete img;
                free(data);
                FPV_S2S(m_statusMessage, "IMAGE decoding error: " << m_platform->currentDownloadURL());
                m_scene->setStatusText( m_statusMessage );
                redraw();
                changeState(STATE_ERROR);
            } else {
                free(data);
	           if  (!m_currentCube) {
		      m_currentCube = new CubicPano(img->size());
	              m_scene->setSceneElement(m_currentCube);
	           }
                m_currentCube->setCubeFace((CubicPano::FaceID)m_currentCubeFaceDownload, img);
                //notify renderer of new cube face
                m_renderer->initElement(*m_currentCube);
                redraw();
	    }
            }
            m_currentCubeFaceDownload++;

            // start download of next cubeface, if required
            if (m_currentCubeFaceDownload <6){
                FPV_S2S(m_statusMessage, "Downloading cube face " << m_currentCubeFaceDownload << ".");
                DEBUG_TRACE(m_statusMessage);
                m_scene->setStatusText( m_statusMessage );
                redraw();
                // more cube faces are waiting
                if (m_param.getCubeSrc(m_currentCubeFaceDownload).size() > 0) {
                    if (!m_platform->startDownloadURL( m_param.getCubeSrc(m_currentCubeFaceDownload) ) ) {
                        FPV_S2S(m_statusMessage, "download failed: " << m_param.getCubeSrc(m_currentCubeFaceDownload));
                        m_scene->setStatusText( m_statusMessage );
                        changeState(STATE_ERROR);
                    }
                } else {
                    m_statusMessage = "not all cube faces specified";
                    DEBUG_TRACE(m_statusMessage);
                    m_scene->setStatusText( m_statusMessage );
                    redraw();
                }
            } else {
                // start viewing!
                m_currentCubeFaceDownload = 0;
                m_statusMessage = "viewing";
                m_scene->setStatusText(m_statusMessage);
                changeState(STATE_VIEWING);
                redraw();
            }
            break;
        default:
            DEBUG_ERROR("INVALID state after downloading to memory");
            // unknown file, ignore
            free(data);
            break;
    }
}

void PanoViewer::onDownloadComplete(const std::string & filename)
{
    // currently only called if the 
    fprintf(stderr, "state %d: file %s downloaded\n", m_state, filename.c_str());

    // figure out what kind of file we got
    std::string type;

    //by checking the magic bytes

    type=Utils::CheckMagicBytes(filename.c_str());
    std::cerr<<"FILE type:"<<type<<std::endl;

    //The viewer begins to download the files
    if (m_state == STATE_DOWNLOADING_SRC) {

       //If the viwer get a DCR file means that we probably have a SPi-V file
       //this means that the XML file is specified in swURL attribute.
       if (type == "DCR"){
              if(m_param.get_SW_Src()!="")
	         m_platform->startDownloadURLToFile(m_param.get_SW_Src());
	   }else if (type == "QTVR") {

           FPV_S2S(m_statusMessage, "Received " << m_platform->currentDownloadURL() << ", decoding QTVR.");
           DEBUG_TRACE(m_statusMessage);
           m_scene->setStatusText( m_statusMessage );
           redraw();

           QTVRDecoder decoder;
           bool ok = false;
           ok = decoder.parseHeaders(filename.c_str());
           if (! ok) {
               FPV_S2S(m_statusMessage, "Error during QTVR parsing: " << decoder.getErrorDescr());
               m_scene->setStatusText( m_statusMessage );
               changeState(STATE_ERROR);
               redraw();
               return;
           }

           if (decoder.getType() == Parameters::PANO_CYLINDRICAL) {
               DEBUG_TRACE("got a cylindrical pano");
               Image * img = 0;
               if (!decoder.extractCylImage(img)) {
                   FPV_S2S(m_statusMessage, "Error during QTVR decoding: " << decoder.getErrorDescr());
                   m_scene->setStatusText( m_statusMessage );
                   changeState(STATE_ERROR);
                   redraw();
               } else {
                   m_statusMessage = "Preparing rendering";
                   DEBUG_TRACE(m_statusMessage);
                   m_scene->setStatusText( m_statusMessage );
                   redraw();
                   CylindricalPano * pano = new CylindricalPano(img->size(), 360);
                   pano->setImage(img);
                   m_scene->setSceneElement(pano);
                   // notify renderer of new cube faces
                   //m_renderer->initElement(*pano);
                   //m_platform->startTimer(10);
                   m_statusMessage="viewing";
                   m_scene->setStatusText("QTVR successfully loaded");
                   changeState(STATE_VIEWING);
                   redraw();
               }

           } else if (decoder.getType() == Parameters::PANO_CUBIC) {
               DEBUG_TRACE("got a cubic pano");
               Image *cubeFaces[6];
               for (int i=0; i < 6 ; i++) {
                   cubeFaces[i] = 0;
               };


               std::string decoderError;
               if (decoder.extractCubeImages(cubeFaces))
               {
                   m_statusMessage = "Preparing rendering";
                   DEBUG_TRACE(m_statusMessage);
                   m_scene->setStatusText( m_statusMessage );
                   redraw();
                   m_currentCube = new CubicPano(cubeFaces[0]->size());
                   m_currentCube->setCubeFaces(cubeFaces);
                   m_scene->setSceneElement(m_currentCube);
                   // notify renderer of new cube faces
                   //m_renderer->initElement(*m_currentCube);
                   //m_platform->startTimer(10);
                   m_statusMessage="viewing";
                   m_scene->setStatusText("QTVR successfully loaded");
                   changeState(STATE_VIEWING);
                   redraw();
               } else {
                   FPV_S2S(m_statusMessage, "Error during QTVR decoding: " << decoder.getErrorDescr());
                   m_scene->setStatusText( m_statusMessage );
                   changeState(STATE_ERROR);
                   redraw();
               }
           } else {
               FPV_S2S(m_statusMessage, "Error during QTVR parsing: No panorama found");
               m_scene->setStatusText( m_statusMessage );
               changeState(STATE_ERROR);
               redraw();
               return;
           };

       } else if (type=="PNG" || type=="JPG") {

           FILE * f = fopen (filename.c_str(), "rb");
           if  (!f)
           {
               FPV_S2S(m_statusMessage, "Error opening downloaded file: " << filename);
               m_scene->setStatusText( m_statusMessage );
               changeState(STATE_ERROR);
               redraw();
               return;
           }

           Image * img = new Image();
	   
           if (!decodeImage(f,img,type)) {
               delete img;
               FPV_S2S(m_statusMessage, type<<" decoding error: " << m_platform->currentDownloadURL());
               m_scene->setStatusText( m_statusMessage );
               redraw();
               changeState(STATE_ERROR);
               return;
	   }

           //A Place Holder is created, that way the desition of which type of pano 
	   //the viewer got is made by the renderer (for more details look at 
           //Renderer render method). 
           SceneElement *pano=new PlaceHolder();
           ((PlaceHolder *)pano)->setImage(img);
           m_scene->setSceneElement(pano);
           redraw();
           m_statusMessage = "viewing";
           m_scene->setStatusText(m_statusMessage);
           changeState(STATE_VIEWING);
           redraw();
	   
       } else if(type=="XML"){

           const char* file_to_download=NULL;
	   if(m_spiv_parser==NULL){
               m_statusMessage = "SPiVparser";
               m_scene->setStatusText( m_statusMessage );
               redraw();
	       m_spiv_parser=new SPiVparser();
	       m_spiv_parser->parseURL(filename.c_str());
	       m_spiv_parser->setPath(m_param.getPath());
	       m_spiv_parser->loadTour();
	   }else{
	       m_spiv_parser->parseNodeURL(filename.c_str());
	   }
	   file_to_download = m_spiv_parser->getXMLToDownload();
	   if(file_to_download!=NULL)
	   {
	       std::cerr<<"Dowloading: "<<file_to_download<<std::endl;
               if (!m_platform->startDownloadURLToFile(file_to_download) ) {
                        FPV_S2S(m_statusMessage, "download failed: " << file_to_download);
                        m_scene->setStatusText( m_statusMessage );
                        changeState(STATE_ERROR);
               }
	   }else{
              m_state=STATE_DOWNLOADING_SPIV;
              m_scene->setStatusText( "SPiVparser: downloading scene" );
              redraw();
              file_to_download = NULL;
              m_spiv_parser->loadScene(*m_scene);
	      redraw();
              file_to_download = m_spiv_parser->getImgURLToDownload();
              if(file_to_download != NULL){
                 if (!m_platform->startDownloadURLToFile(file_to_download) ) {
                    FPV_S2S(m_statusMessage, "download failed: " << file_to_download);
                    m_scene->setStatusText( m_statusMessage );
                    changeState(STATE_ERROR);
                 }
	      }
	   }
           
       }else{
           m_statusMessage = "internal error: invalid state after downloading to file";
           m_scene->setStatusText( m_statusMessage );
           changeState(STATE_ERROR);
       }
    }else if(m_state == STATE_DOWNLOADING_SPIV){
       const char* file_to_download=NULL;
       Image * img=NULL;
       if (type=="PNG" || type=="JPG") {
       	//we get the image object
       	img=m_spiv_parser->extractImage();
       	//use the data, decoded and add it to the image object.
       	//decodeImage((unsigned char*) data, sz, img, AUTO);
       	FILE * f = fopen (filename.c_str(), "rb");
       	if  (!f)
       	{
	
       	   FPV_S2S(m_statusMessage, "Error opening downloaded file: " << filename);
       	   m_scene->setStatusText( m_statusMessage );
       	   changeState(STATE_ERROR);
       	   redraw();
       	   return;
       	}
       	decodeImage(f,img,type);

       	//we get the next url to download.
       	file_to_download= m_spiv_parser->getImgURLToDownload();

       	if(file_to_download!=NULL){
          	if (!m_platform->startDownloadURLToFile(file_to_download) ) {
             	FPV_S2S(m_statusMessage, "download failed: " << file_to_download);
             	m_scene->setStatusText( m_statusMessage );
             	changeState(STATE_ERROR);
        }
       	}else{
          	m_statusMessage = "viewing";
          	m_scene->setStatusText(m_statusMessage);
          	changeState(STATE_VIEWING);
          	redraw();
       	}
      }else{
	changeState(STATE_ERROR);
      }
    }
}

void PanoViewer::changeScene(){
    if (m_next_scene){
	if(m_scene){
	    delete(m_controller);
	    delete(m_scene);
	}
	m_scene=m_next_scene;
	m_controller = new Controller(m_scene);
	m_next_scene=NULL;
	redraw();
    }
}

void PanoViewer::loadNextScene(const char* _scene, float _fov, float _yaw, float _pitch){
    if(m_spiv_parser){
	m_next_scene=new Scene();
	Camera *cam=m_next_scene->getCamera();
	cam->setPitch(_pitch);
	cam->setYaw(_yaw);
	cam->setFOV(_fov);
	m_spiv_parser->loadScene(*m_next_scene, _scene);
	changeState(STATE_DOWNLOADING_SPIV);
	const char* file_to_download = m_spiv_parser->getImgURLToDownload();
	if(file_to_download != NULL){
	    std::cerr<<"File name to download: "<<file_to_download<<std::endl;
	    if (!m_platform->startDownloadURLToFile(file_to_download) ) {
		FPV_S2S(m_statusMessage, "download failed: " << file_to_download);
		m_next_scene->setStatusText( m_statusMessage );
		changeState(STATE_ERROR);
	    }
	}
    }
}

void PanoViewer::changeCamera(float _fov, float _yaw, float _pitch){
    if(m_scene){
	Camera *cam=m_scene->getCamera();
	if(cam){
	    cam->setPitch(_pitch);
	    cam->setYaw(_yaw);
	    cam->setFOV(_fov);
	}
    }
}



void PanoViewer::redraw()
{
    if (m_renderer) {
	m_renderer->render(*m_scene);
    }
}
