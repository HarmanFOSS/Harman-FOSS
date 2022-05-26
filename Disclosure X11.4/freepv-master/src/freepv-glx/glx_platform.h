/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#ifndef FPV_GLX_PLATFORM
#define FPV_GLX_PLATFORM

#include <libfreepv/Platform.h>

#include <stdio.h>
#include <stdlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#ifdef HAVE_XF86VMODE_H
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/keysym.h>


using namespace FPV;

/** base GLX platform for unix plugin and standalone executable */
class GLXPlatform : public Platform
{
public:

    GLXPlatform()
    : m_timerPeriod(0)
    {
    }

    /** start the timer, fires each \p delay milliseconds */
    virtual void startTimer(int delay)
    {
        m_timerPeriod = delay;
    };

    /** stop the timer */
    virtual void stopTimer()
    {
        m_timerPeriod = 0;
    };

    /** convert X event into our platform independent event */
    void ConvertXEvent(XEvent * ev);
protected:
    /* stuff about our window grouped together */
    typedef struct {
        Display *dpy;
        int screen;
        Window win;
        GLXContext ctx;
        XSetWindowAttributes attr;
        bool fs;
#ifdef HAVE_XF86VMODE_H
        XF86VidModeModeInfo deskMode;
#endif
        int x, y;
        unsigned int width, height;
        unsigned int depth;    
    } GLWindow;

    GLWindow m_glwin;
    int m_timerPeriod;
};

/** base GLX platform for unix standalone executable */
class GLXPlatformStandalone : public GLXPlatform
{

public:
    /** construct a glx platform for standalone usage.
     *
     *  This creates and initialises the opengl window, and sets
     *  up the event handler
     */
    GLXPlatformStandalone(int argc, char ** argv);

    /** Start file download (just reads the file from disk...)
     */
    bool startDownloadURL(const std::string & url);

    bool startDownloadURLToFile(const std::string & url);

    const std::string & currentDownloadURL()
    {
        return m_currentURL; 
    }

    std::string currentDownloadMimeType()
    { 
        return "";
    }


    /** make the drawing surface current */
    virtual void glBegin();

    /** required by some platforms (gtkglext) */
    virtual void glEnd() {};

    /** swap the gl buffers */
    virtual void glSwapBuffers();

    /** starts the event loop.
     *  This function will only return if the window is closed
     *  or quit() has been called
     */
    void ProcessEvents();

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
};

#endif
