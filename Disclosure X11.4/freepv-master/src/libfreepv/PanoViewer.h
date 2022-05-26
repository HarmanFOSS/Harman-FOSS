/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoViewer.h 150 2008-10-15 14:18:53Z leonox $
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


#ifndef FPV_VIEWER_H
#define FPV_VIEWER_H

#include "Platform.h"
#include "Parameters.h"
#include "Renderer.h"
#include "Scene.h"
#include "Controller.h"
#include "SPiVparser.h"

namespace FPV
{

/** This is the main viewer class */
class PanoViewer : public PlatformEventListener
{
private:
    PanoViewer();
    virtual ~PanoViewer();
public:
    static PanoViewer* Instance();

    /** Initialisation function
     *
     *  This will prepare the viewer for operation,
     *  but not start the viewer or initialize the
     *  renderer.
     *
     *  @param argc  number of key=value pairs
     *  @param argn  array of key name strings
     *  @param argv  array of value strings
     *  @return true if ok, false if something was broken
     */
    bool init(Platform & platform, const Parameters & para);

    /** main viewer function, will be called by the main
     *  plugin/program to run the viewer
     *
     *  The opengl context is already set up, and should
     *  be initialized and useable
     *
     *  This is the main() function of the freepv viewer.
     *  It should start the downloading, if it is not already
     *  happening and should then create the suitable
     *  renderers and start rendering.
     */
    bool start();

    /** set a status text */
    void setStatus(const std::string & str);

    // =============================================================
    // These function will be called by the platform

    /** Called whenever the window is resized */
    virtual void onResize(Size2D size);

    /** called when the window (or a part of it) needs to be redrawn 
     *
     *  @param x,y  upper left corner of area that should be redrawn
     *  @param w,h  width and height of area that should be redrawn
     *  @param count number of pending areas requiring a update.
     *               If the function redraws the full screen, it
     *               should just respond to calls with count == 0.
     */
    virtual void onRedraw(int x, int y, int w, int h, int count);

    /** called when a mouse event happend */
    virtual void onMouseEvent(const MouseEvent & event);

    /** called when a key event has happend */
    virtual void onKeyEvent(const KeyEvent & event);

    /** called before the instance is destroyed.
     *
     *  This function should acknowledge the destruction with a call
     *  to Platform::quit()
     */
    virtual void onDestroy();
    /** called when a timer event happens.
     *
     *  @param time contains the absolute time in seconds relative to a unknown
     *              date
     */
    virtual void onTimer(double time);

    // download events

    /** notify about the current download progress.
     *
     */
    virtual void onDownloadProgress(void * data, size_t downloadedBytes, size_t size);

    /** notify about a finished download */
    virtual void onDownloadComplete(void * data, size_t downloadedBytes);

    virtual void onDownloadComplete(const std::string & filename);

    /** change scene */
    void changeScene();

    /** load next scene*/
    void loadNextScene(const char* _scene, float _fov, float _yaw, float _pitch);

    /** change camera */
    void changeCamera(float _fov, float _yaw, float _pitch);

    /** return parameter object */
    Parameters & getParam()
    {
        return m_param;
    }

    Scene * getScene(){ return m_scene; }
    Renderer* getRenderer(){ return m_renderer; }

protected:


    /** an enum of the viewer state. useful to remember
     *  what should be done next in the various callbacks
     */
    enum State 
    {
        STATE_NOT_INTIALIZED,       ///< the viewer has just been constructed, not initialized
        STATE_INIT,                 ///< setting up, arguments available
        STATE_DOWNLOADING_PREVIEW,  ///< currently downloading the preview image
        STATE_DOWNLOADING_SRC,      ///< currently downloading the file from the src attribute
        STATE_DOWNLOADING_CUBEFACES,///< currently downloading cubefaces
        STATE_DOWNLOADING_SPIV,
        STATE_VIEWING,              ///< viewer is fully constructed, normal viewing
        STATE_ERROR                 ///< an error has occured, cannot continue
    };

    /** internal function to change the state */
    void changeState(State state) {
        fprintf(stderr, "state %d: %s\n", state, m_statusMessage.c_str());
        m_state = state;
    }

    void redraw();
    
    /** Note: all changes to state should be done through changeState() */
    State m_state;

    Platform * m_platform;

    Parameters m_param;

    Renderer * m_renderer;
    Scene * m_scene;
    Scene * m_next_scene;
    Controller * m_controller;

    //Parser for the SPiV xml files
    SPiVparser *m_spiv_parser;
    
    // downloaded data
    //    unsigned char * m_currentDownload;
    //    size_t m_currentDownloadSize;

    // these are some hacky variables, required during cube loading
    CubicPano * m_currentCube;
    int m_currentCubeFaceDownload;

    // a user visible message describing the current status
    std::string m_statusMessage;
};

}
#endif
