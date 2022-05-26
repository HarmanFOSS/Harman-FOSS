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

#include "config.h"
#include <npapi.h>

#include <fstream>
#include <sys/time.h>

extern "C" {
#include <GL/gl.h>
#include <GL/glu.h>
}

#include <libfreepv/utils.h>
#include "plugin_unix.h"


#define FPV_MIME_TYPES_HANDLED      "application/freepv-plugin"
#define FPV_PLUGIN_NAME             "FreePV interactive panoramic viewer plugin, QuickTime, SPi-V"
//#define FPV_MIME_TYPES_DESCRIPTION  FPV_MIME_TYPES_HANDLED":mov;png;jpg:"FPV_PLUGIN_NAME
#define FPV_PLUGIN_DESCRIPTION      FPV_PLUGIN_NAME " Opensource software, licensed under LGPL 2.1"

#define FPV_MIME_TYPES_DESCRIPTION "application/freepv-plugin:jpg:FreePV;" \
                                   "application/freepv-plugin:png:FreePV;" \
                                   "video/quicktime:mov:Quicktime;" \
                                   "video/x-quicktime:mov:Quicktime;" \
                                   "image/x-quicktime:mov:Quicktime;" \
                                   "application/x-quicktime:mov:Quicktime;" \
                                   "application/quicktime:mov:Quicktime;" \
                                   "application/x-quicktimeplayer:mov:Quicktime;" \
                                   "graphics/pangeavr:mov,qtvr,*:PangeaVR;" \
                                   "graphics/pangeavr2:mov,qtvr,*:PangeaVR 2;" \
                                   "application/glpanoview:mov,qtvr,*:glpanoview;"\
                                   "application/x-director:dcr:Schockwave"

static Display *gxDisplay = NULL;

static int attributeList_noFSAA[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
static int attributeList_FSAA[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 1, GLX_SAMPLE_BUFFERS_ARB, 1,GLX_SAMPLES_ARB, 1, None };

using namespace FPV;

//////////////////////////////////////
//
// general initialization and shutdown

NPError NS_PluginInitialize()
{
    DEBUG_TRACE("");

    gxDisplay = XOpenDisplay(NULL);
    if (gxDisplay) {
        DEBUG_DEBUG("Opened connection to X11 server" );
    } else {
        DEBUG_ERROR("Couldn't open a connection to the X11 server!" );
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    }

    return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
    DEBUG_TRACE("");
    if (gxDisplay) {
        XCloseDisplay(gxDisplay);
        gxDisplay = NULL;
        DEBUG_DEBUG("Closed connection to X11 server" );
    }

}

// get values per plugin
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
    DEBUG_TRACE("");
    NPError err = NPERR_NO_ERROR;
    switch (aVariable) {
        case NPPVpluginNameString:
            *((char **)aValue) = FPV_PLUGIN_NAME;
            break;
        case NPPVpluginDescriptionString:
            *((char **)aValue) = FPV_PLUGIN_DESCRIPTION;
            break;
            /*
        case NPPVpluginNeedsXEmbed:
            *((PRBool *)aValue) = PR_TRUE;
            break;
            */
        default:
            err = NPERR_INVALID_PARAM;
            break;
    }
    return err;
}

char* NPP_GetMIMEDescription(void)
{
    DEBUG_TRACE("");
    return(FPV_MIME_TYPES_DESCRIPTION);
}


/////////////////////////////////////////////////////////////
//
// utility functions
//

static double getTime()
{
    long long unsigned currTime;
    long long unsigned deltaTime;
    static long long unsigned oldTime = 0;

    struct timeval tv;

    gettimeofday(&tv, 0);

    currTime = tv.tv_sec * 1000000LL + tv.tv_usec;

    if (oldTime == 0) {
        oldTime = currTime;
    }
    deltaTime = currTime - oldTime;
    //DEBUG_DEBUG("curr: " << currTime << "  old: " << oldTime << "  delta: " << deltaTime);
    double dt = (((double)deltaTime) / 1000000.0);
    //DEBUG_DEBUG("delta in seconds: " << dt);
    return dt;
}


/////////////////////////////////////////////////////////////
//
// construction and destruction of our plugin instance object
//

nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
    DEBUG_TRACE("");

    if(!aCreateDataStruct)
        return NULL;

    nsPluginInstanceUnix * plugin = new nsPluginInstanceUnix(aCreateDataStruct);
    return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
    DEBUG_TRACE("");

    if(aPlugin)
        delete (nsPluginInstanceUnix *)aPlugin;
}

nsPluginInstanceUnix::nsPluginInstanceUnix(nsPluginCreateData *pcd)
    : nsPluginInstance(pcd),
      mXtwidget(0), mWindow(0), mX(0), mY(0), mWidth(0), mHeight(0),
      mVisual(0), mColormap(0), mDepth(0), mFontInfo(0), mGC(0),
      m_appContext(0), m_timerID(0), m_timerActive(false),
      m_glxContext(0), m_glInitialized(0),
      m_viewer(0)
{
    DEBUG_TRACE("instance: " << mInstance);
    const char * home = getenv("HOME");
    if (home != 0) {
        std::string filename(home);
        filename.append("/.freepv");
        DEBUG_DEBUG("prefs file: " << filename);
        std::ifstream pstream(filename.c_str());
        if (pstream.is_open()) {
            while(pstream.good() && (!pstream.eof())) {
                std::string line;
                std::getline(pstream, line);
                if (line.length() == 0) 
                    continue;
                if (line[0] == '#')
                    continue;
                m_fpvParam->parse(line.c_str());
            }
        }
    }

    m_viewer = PanoViewer::Instance();
    m_viewer->init( *this, *m_fpvParam);
}

// these three methods must be implemented in the derived
// class platform specific way
NPBool nsPluginInstanceUnix::init(NPWindow* aWindow)
{
    DEBUG_TRACE("");

    if(aWindow == NULL)
        return FALSE;
    
    DEBUG_DEBUG("window: " << aWindow->window << ", pos, size: " << aWindow->x << "," << aWindow->y <<
                "  " << aWindow->width << "," << aWindow->height);
    
    if (SetWindow(aWindow))
        mInitialized = TRUE;

#if 0
    mX = aWindow->x;
    mY = aWindow->y;
    mWidth = aWindow->width;
    mHeight = aWindow->height;
    mWindow = (Window) aWindow->window;

    m_gtkplug = gtk_plug_new(mWindow);
    m_drawingArea = createGTKWindow(0, m_gtkplug, mWidth, mHeight, m_viewer, this);
    /*
    GdkGLConfig * glcfg = configureGL();
    if (! glcfg) {
        DEBUG_ERROR("OpenGL configuration failed");
        return FALSE;
    }
    m_drawingArea = createGTKWindow(glcfg, m_gtkplug, mWidth, mHeight, m_viewer, this);
    */
#endif
    mInitialized = TRUE;
    return TRUE;
}

void nsPluginInstanceUnix::shut()
{
    DEBUG_TRACE("");

    //delete (m_viewer);
    destroyGLXContext();
    mInitialized = FALSE;
}


NPError nsPluginInstanceUnix::SetWindow(NPWindow* aWindow)
{
    DEBUG_TRACE("");
    if ((aWindow == NULL) || (aWindow->window == NULL)) {
        return NPERR_NO_ERROR;
    }
    if (aWindow->x == mX && aWindow->y == mY
        && (int) aWindow->width == mWidth
        && (int) aWindow->height == mHeight
        && (unsigned long)(aWindow->window) == mWindow) 
    {
        DEBUG_DEBUG("called with same window as before");
        return NPERR_NO_ERROR;
    }
    mX = aWindow->x;
    mY = aWindow->y;
    mWidth = aWindow->width;
    mHeight = aWindow->height;
    if (mWindow == (Window) aWindow->window) {
        // The page with the plugin is being resized.
        // Save any UI information because the next time
        // around expect a SetWindow with a new window id.
        DEBUG_DEBUG("same window, but different size or position");
    } else {
        DEBUG_DEBUG("creating new window for plugin instance: "<< mInstance);
        // do something?
        mWindow = (Window) aWindow->window;
        NPSetWindowCallbackStruct *ws_info =
                (NPSetWindowCallbackStruct *)aWindow->ws_info;
        mVisual = ws_info->visual;
        mDepth = ws_info->depth;
        mColormap = ws_info->colormap;
        mDisplay = ws_info->display;

        /*
        if (!mFontInfo) {
            if (!(mFontInfo = XLoadQueryFont(gxDisplay, "9x15"))) {
                DEBUG_DEBUG("ERROR: Cannot open 9X15 font!" );
            }
        }
        */
        
        // add xt event handler
        Widget xtwidget = XtWindowToWidget(mDisplay, mWindow);
        if (xtwidget && mXtwidget != xtwidget) {
            mXtwidget = xtwidget;
            long event_mask = ExposureMask | KeyPressMask | 
                    KeyReleaseMask | ButtonPressMask |
                    ButtonReleaseMask | ButtonMotionMask |
                    StructureNotifyMask| PointerMotionMask;
            XSelectInput(mDisplay, mWindow, event_mask);
            XtAddEventHandler(xtwidget, event_mask, False, (XtEventHandler)nsPluginInstanceUnix::xtEventHandler, this);
        }

#if 0
        // add xt event handler
        DEBUG_DEBUG("before XtWindowToWidget: display" << gxDisplay << "  window: " << mWindow);
        Widget xtwidget = XtWindowToWidget(gxDisplay, mWindow);
        DEBUG_DEBUG("xtwidget: " << xtwidget);
        if (xtwidget && mXtwidget != xtwidget) {
            mXtwidget = xtwidget;
            long event_mask = ExposureMask | KeyPressMask | 
                    KeyReleaseMask | ButtonPressMask |
                    ButtonReleaseMask | ButtonMotionMask |
                    StructureNotifyMask;
            DEBUG_DEBUG("");
            XSelectInput(gxDisplay, mWindow, event_mask);
            DEBUG_DEBUG("XSelectInput ok");
            XtAddEventHandler(xtwidget, event_mask, False, (XtEventHandler)nsPluginInstanceUnix::xtEventHandler, this);
            DEBUG_DEBUG("XtAddEventHandler ok");

        }
#endif

        XVisualInfo *vi = glXChooseVisual(gxDisplay, DefaultScreen(gxDisplay),
                                          attributeList_FSAA);
        if (vi == NULL) {
            vi = glXChooseVisual(gxDisplay, DefaultScreen(gxDisplay), attributeList_noFSAA);
        } else {
            vi->visual = mVisual;
        }
        m_glxContext = glXCreateContext(gxDisplay, vi, 0, GL_TRUE);
        if (m_glxContext) {
            DEBUG_DEBUG("new glxContext: " << (void *)m_glxContext );
            setGL();
            m_glInitialized = TRUE;

            // hope this does not happen too often.. start viewer
            m_viewer->start();

            XtAppContext context;

            if (NPN_GetValue(mInstance, NPNVxtAppContext, (void*)context) != NPERR_NO_ERROR ) {
                DEBUG_ERROR("Could not get XtAppContext from mozilla, trying XtDisplayToApplicationContext");
                // try out alternative way to retrieve the XtAppContext
                context = XtDisplayToApplicationContext(mDisplay);
            } else {
                m_appContext = context;
            }
            m_appContext = context;

        } else {
            DEBUG_ERROR("ERROR: Couldn't get new glxContext!" );
            m_glInitialized = FALSE;
            m_glxContext = 0;
        }
    }

    // resize window
    if (m_viewer) {
        m_viewer->onResize(Size2D(mWidth, mHeight));
    }

    //draw();
    return NPERR_NO_ERROR;
}


#if 0
void nsPluginInstanceUnix::draw()
{
    DEBUG_TRACE("");
#if 0
    // draw a simple scene
    glMakeCurrent();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLfloat)m_glwin.width , 0.0, (GLfloat)m_glwin.height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(20,20);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (GLubyte *)"Hello World");

    glSwapBuffers();

#endif
    /*
    unsigned int h = mHeight/2;
    unsigned int w = 3 * mWidth/4;
    int x = (mWidth - w)/2; // center
    int y = h/2;
    if (x >= 0 && y >= 0) {
        GC gc = XCreateGC(mDisplay, mWindow, 0, NULL);
        if (!gc)
            return;
        XDrawRectangle(mDisplay, mWindow, gc, x, y, w, h);
        const char *string = getVersion();
        if (string && *string) {
            int l = strlen(string);
            int fmba = mFontInfo->max_bounds.ascent;
            int fmbd = mFontInfo->max_bounds.descent;
            int fh = fmba + fmbd;
            y += fh;
            x += 32;
            XDrawString(mDisplay, mWindow, gc, x, y, string, l);
        }
        XFreeGC(mDisplay, gc);
    }
    */
}

#endif

::uint16  nsPluginInstanceUnix::HandleEvent(void* event)
{
    DEBUG_TRACE("");

    return 0;
}

void nsPluginInstanceUnix::startTimer(int delay)
{
    if (! m_timerActive) {
        // add timer
        m_timerID = XtAppAddTimeOut(m_appContext, 1, (XtTimerCallbackProc) nsPluginInstanceUnix::xtTimeOutProc, this);
        m_timerActive = true;
        DEBUG_DEBUG("added timeout proc, id: " << m_timerID);
    }
}

void nsPluginInstanceUnix::stopTimer()
{
    // TODO: stop events
    DEBUG_DEBUG("stop timer");
    if (m_timerActive) {
        XtRemoveTimeOut(m_timerID);
    }
    m_timerActive = false;
}


void nsPluginInstanceUnix::setGL() {
//    DEBUG_TRACE("setGL");
    if (gxDisplay && m_glxContext && mWindow) {
        glXMakeCurrent(gxDisplay, mWindow, m_glxContext);
        XSync(gxDisplay, False);
    }
}

/// \brief Shutdown OpenGL
void
nsPluginInstanceUnix::destroyGLXContext()
{
    DEBUG_TRACE("");

    if (!m_glInitialized) {
        DEBUG_DEBUG(__FUNCTION__ << ": OpenGL already killed..." );
    return;
    }

    if (gxDisplay && m_glxContext) {
        glXDestroyContext(gxDisplay, m_glxContext);
        m_glxContext = NULL;
    }

    m_glInitialized = FALSE;
}

/** make the drawing surface current */
void nsPluginInstanceUnix::glBegin()
{
    setGL();
}

void nsPluginInstanceUnix::glEnd()
{
}

/** swap the gl buffers */
void nsPluginInstanceUnix::glSwapBuffers()
{
    if (gxDisplay && mWindow) {
//             glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//             glFlush();
        glXSwapBuffers(gxDisplay, mWindow);
    } else {
        DEBUG_ERROR("could not swap buffers, display: " << gxDisplay << "  window: " << mWindow);
    }
    GLenum errCode;
    const GLubyte * errString;
    errCode = glGetError();
    errString = gluErrorString(errCode);
    if (errCode != GL_NO_ERROR) {
        DEBUG_ERROR("OpenGL Error: " << errCode << ", " <<  errString);
    }
}

void nsPluginInstanceUnix::xtTimeOutProc(nsPluginInstanceUnix * thisp, XtIntervalId *id)
{
    double t = getTime();
//    DEBUG_TRACE("time, " << t << "  id: " << id);
    if (thisp->m_eventListener) {
        thisp->m_eventListener->onTimer(t);
    }
    if (thisp->m_timerActive) {
        thisp->m_timerID = XtAppAddTimeOut(thisp->m_appContext, 1, (XtTimerCallbackProc) nsPluginInstanceUnix::xtTimeOutProc, thisp);
    }
}


void nsPluginInstanceUnix::xtEventHandler(Widget xtwidget, nsPluginInstanceUnix *thisp, XEvent *event, Boolean *b)
{
    //KeySym key;
    KeyEvent kev;
    MouseEvent mev;
    switch (event->type)
    {
        case KeyPress:
        case KeyRelease:
            kev.keysym = XLookupKeysym(&(event->xkey), 0);;
            kev.modifiers = event->xkey.state;
            kev.down = event->xkey.type == KeyPress;
            if (thisp->m_eventListener) {
                thisp->m_eventListener->onKeyEvent(kev);
            }
            break;
        case ButtonPress:
        case ButtonRelease:
            mev.pos = Point2D(event->xbutton.x, event->xbutton.y);
            mev.modifiers = event->xbutton.state;
            mev.buttonNr = event->xbutton.button;
            mev.down = event->xbutton.type == ButtonPress;
            if (thisp->m_eventListener) {
                thisp->m_eventListener->onMouseEvent(mev);
            }
            break;
        case MotionNotify:
            mev.pos = Point2D(event->xmotion.x, event->xmotion.y);
            mev.modifiers = event->xmotion.state;
            mev.buttonNr = 0;
            mev.down = false;
            if (thisp->m_eventListener) {
                thisp->m_eventListener->onMouseEvent(mev);
            }
            break;
        case ConfigureNotify:
            if (thisp->m_eventListener) {
                thisp->m_eventListener->onResize(FPV::Size2D(event->xconfigure.width, event->xconfigure.height));
            }
            break;
        case Expose:
            if (thisp->m_eventListener) {
                thisp->m_eventListener->onRedraw(event->xexpose.x, event->xexpose.y,
                                          event->xexpose.width, event->xexpose.height, event->xexpose.count);
            }
            break;
        default:
            break;
    }
};

