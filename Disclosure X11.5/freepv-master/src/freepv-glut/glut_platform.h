/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Thomas Rauscher <t.rauscher@sinnfrei.at>
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: glut_platform.h 91 2006-10-16 20:32:08Z dangelo $
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

#ifndef FPV_GLUT_PLATFORM
#define FPV_GLUT_PLATFORM

#include <libfreepv/Platform.h>

#include <stdio.h>
#include <stdlib.h>


using namespace FPV;

/** base GLX platform for unix plugin and standalone executable */
class GLUTPlatform : public Platform
{
public:

    GLUTPlatform()
    : m_timerPeriod(0), m_modifiers(0)
    {
    }

    /** start the timer, fires each \p delay milliseconds */
    virtual void startTimer(int delay);

    /** stop the timer */
    virtual void stopTimer();

    /** methode called by by the GLUT display callback  */
	virtual void glutDisplayCallback(void);
    /** methode called by by the GLUT keyboard callback  */
	virtual void glutKeyboardCallback(unsigned char key,int x, int y, bool down);
    /** methode called by by the GLUT keyboard callback  */
    virtual void glutKeyboardSpecialCallback(int key,int x, int y, bool down);
    /** methode called by by the GLUT mouse callback  */
	virtual void glutMouseCallback(int button, int state,int x, int y);
    /** method called by GLUT mouse wheel callback */
    virtual void glutScrollWheelCallback(int button, int state, int x, int y);
    /** methode called by by the GLUT mousemotion callback  */
	virtual void glutMouseMotionCallback(int x, int y);
    /** methode called by by the GLUT idle callback  */
	virtual void glutIdleCallback(void);
    /** methode called by by the GLUT timer callback  */
	virtual void glutOnTimerCallback(void);
    /** method called by GLUT reshape callback */
    virtual void glutReshapeCallback(int width, int height);

    /** starts the event loop.
     *  This function will only return if the window is closed
     *  or quit() has been called
     */
	virtual void ProcessEvents(void);

    /** make the drawing surface current */
    virtual void glBegin() {};

    /** required by some platforms (gtkglext) */
    virtual void glEnd() {};

	/** swap the gl buffers */
    virtual void glSwapBuffers();

protected:
    /* stuff about our window grouped together */
    typedef struct {
        int screen;
        bool fs;
        int x, y;
        unsigned int width, height;
        unsigned int depth;    
    } GLWindow;

    GLWindow m_glwin;
    int m_timerPeriod;
    /// current state of modifiers (mouse buttons, shift keys etc), see ModifierType
    int m_modifiers;
};

/** base GLUT platform for standalone executable */
class GLUTPlatformStandalone : public GLUTPlatform
{

public:
    /** construct a glut platform for standalone usage.
     *
     *  This creates and initialises the opengl window, and sets
     *  up the event handler
     */
    GLUTPlatformStandalone(int argc, char ** argv);

    /** Start file download (just reads the file from disk...)
     */
    bool startDownloadURL(const std::string & url);

    bool startDownloadURLToFile(const std::string & url);

    const std::string & currentDownloadURL();

    std::string currentDownloadMimeType()
    { 
        return "";
    }

    virtual void quit(int ret);

protected:

    bool createGLWindow(char* title, int width, int height, int bits,
                        bool fullscreenflag);

    /* function to release/destroy our resources and restoring the old desktop */
    void killGLWindow();

    bool m_running;
    int m_exitcode;

    // the url that is currently being downloaded
    std::string m_currentURL;

    int m_currentGlutMod;
};

#endif
