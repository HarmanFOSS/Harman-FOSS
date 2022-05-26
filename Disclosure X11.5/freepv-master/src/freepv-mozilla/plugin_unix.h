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

#ifndef FPV_PLUGIN_UNIX_H
#define FPV_PLUGIN_UNIX_H

/* Xlib/Xt stuff */
extern "C" {
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
}

#include "plugin.h"

#include <libfreepv/PanoViewer.cpp>

namespace FPV
{

class nsPluginInstanceUnix : public nsPluginInstance
{

public:
    // modified constructor to receive the plugin creation data
    nsPluginInstanceUnix(nsPluginCreateData *pcd);

  // these three methods must be implemented in the derived
  // class platform specific way
  virtual NPBool init(NPWindow* aWindow);
  virtual void shut();

  // implement all or part of those methods in the derived
  // class as needed
  virtual NPError SetWindow(NPWindow* pNPWindow);
  virtual void    Print(NPPrint* printInfo)                         { return; }
  virtual uint16  HandleEvent(void* event);
  /*
  virtual void    URLNotify(const char* url, NPReason reason,
                            void* notifyData)                       { return; }
  virtual NPError GetValue(NPPVariable variable, void *value)       { return NPERR_NO_ERROR; }
  virtual NPError SetValue(NPNVariable variable, void *value)       { return NPERR_NO_ERROR; }
  */
  
  //void draw();
  
protected:

    // internal utility functions for opengl handling
    void initGLX();
    int resizeWindow(int width,int height);
    void destroyGLXContext();

    // Set the current GL context
    void setGL();

    // Xt event stuff
    static void xtEventHandler(Widget xtwidget, nsPluginInstanceUnix *thisp, XEvent *event, Boolean *b);
    //static Boolean xtWorkProc(nsPluginInstanceUnix * thisp);
    static void xtTimeOutProc(nsPluginInstanceUnix * thisp, XtIntervalId *id);


    Widget mXtwidget;
    Window mWindow;
    Display *mDisplay;
    int mX, mY;
    int mWidth, mHeight;
    Visual* mVisual;
    Colormap mColormap;
    unsigned int mDepth;
    XFontStruct *mFontInfo;
    GC mGC;
    XtAppContext m_appContext;
    XtIntervalId m_timerID;
    bool m_timerActive;

    GLXContext m_glxContext;
    NPBool m_glInitialized;


public:
    
    // FPV::Platform interface

    virtual void quit(int ret) {};

    ////////////////////////////////////////////////
    // Timer functions

    /** start the timer, fires each \p delay milliseconds */
    virtual void startTimer(int delay);

    /** stop the timer */
    virtual void stopTimer();

    ////////////////////////////////////////////////
    // Drawing functions

    /** make the drawing surface current */
    virtual void glBegin();
    /** required by some platforms (gtkglext) */
    virtual void glEnd();

    /** swap the gl buffers */
    virtual void glSwapBuffers();

protected:

    PanoViewer * m_viewer;

};

} // namespace

#endif
