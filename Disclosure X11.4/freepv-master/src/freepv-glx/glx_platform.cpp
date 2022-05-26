/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *          Mihael.Vrbanec@stud.uni-karlsruhe.de  (glx window creation)
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

#include <sys/poll.h>
#include <sys/time.h>

#include "libfreepv/utils.h"
#include "glx_platform.h"


void GLXPlatform::ConvertXEvent(XEvent * event)
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
            if (m_eventListener) {
                m_eventListener->onKeyEvent(kev);
            }
            break;
        case ButtonPress:
            mev.pos = Point2D(event->xbutton.x, event->xbutton.y);
            mev.modifiers = event->xbutton.state;
            mev.buttonNr = event->xbutton.button;
            mev.down = event->xbutton.type == ButtonPress;
            if (m_eventListener) {
                m_eventListener->onMouseEvent(mev);
            }
            break;
        case ButtonRelease:
            mev.pos = Point2D(event->xbutton.x, event->xbutton.y);
            mev.modifiers = event->xbutton.state;
            mev.buttonNr = event->xbutton.button;
            mev.down = event->xbutton.type == ButtonPress;
            if (m_eventListener) {
                m_eventListener->onMouseEvent(mev);
            }
            break;
        case MotionNotify:
            mev.pos = Point2D(event->xmotion.x, event->xmotion.y);
            mev.modifiers = event->xmotion.state;
            mev.buttonNr = 0;
            mev.down = false;
            if (m_eventListener) {
                m_eventListener->onMouseEvent(mev);
            }
            break;
        case ConfigureNotify:
            if (m_eventListener) {
                m_eventListener->onResize(FPV::Size2D(event->xconfigure.width, event->xconfigure.height));
            }
            break;
        case Expose:
            if (m_eventListener) {
                m_eventListener->onRedraw(event->xexpose.x, event->xexpose.y,
                                          event->xexpose.width, event->xexpose.height, event->xexpose.count);
            }
            break;
        case ClientMessage:    
            if (*XGetAtomName(m_glwin.dpy, event->xclient.message_type)
                 == *"WM_PROTOCOLS")
            {
                fprintf(stderr, "Exiting sanely...\n");
                if (m_eventListener) {
                    m_eventListener->onDestroy();
                }
            }
            break;
        default:
            break;
    }
    std::cerr<<"Type="<<event->type<<std::endl;
};

GLXPlatformStandalone::GLXPlatformStandalone(int argc, char ** argv)
{
    m_running = true;
    m_exitcode = 0;
    /* default to fullscreen */
    m_glwin.fs = false;
    if (!createGLWindow("freepv", 640, 480, 24, m_glwin.fs))
    {
        fprintf(stderr, "creation of GL window failed, exiting\n");
        m_running = false;
        m_exitcode = 1;
    }
}

bool GLXPlatformStandalone::startDownloadURL(const std::string & url)
{
    m_currentURL = url;
    FILE * f;
    f = fopen(url.c_str(),"rb");
    if (!f) 
        return false;
    if (fseek(f, 0, SEEK_END) != 0 )
        return false;
    size_t sz = ftell(f);
    if (fseek(f, 0, SEEK_SET) != 0 )
    rewind(f);
    void * buffer = malloc(sz);
    if (!buffer) {
        return false;
    }
    size_t readsz = fread(buffer, 1, sz, f);
    m_eventListener->onDownloadComplete( buffer, readsz );
    return true;
}


bool GLXPlatformStandalone::startDownloadURLToFile(const std::string & url)
{
    m_currentURL = url;
    m_eventListener->onDownloadComplete( url );
    return true;
}


void GLXPlatformStandalone::quit(int ret)
{
    m_running = false;
    m_exitcode = ret;
}

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
    return (double)(deltaTime / 1000000.0);
}


void GLXPlatformStandalone::ProcessEvents()
{
    XEvent event;
    /* the event loop */
    while (m_running)
    {
        /* handle the events in the queue */
        while (XPending(m_glwin.dpy) > 0)
        {
            XNextEvent(m_glwin.dpy, &event);
            ConvertXEvent(&event);
        }
        // todo: add timer here...
        if (m_timerPeriod) {
            poll(0,0, m_timerPeriod);
            // get absolute time in ms
            m_eventListener->onTimer(getTime());
        } else {
            poll(0,0, 100);
        }

    }
}


/* this function creates our window and sets it up properly */
/* FIXME: bits is currently unused */
bool GLXPlatformStandalone::createGLWindow(char* title,
                                           int width, int height, int bits,
                                           bool fullscreenflag)
{

    /* attributes for a single buffered visual in RGBA format with at least
     * 4 bits per color and a 16 bit depth buffer */
    static int attrListSgl[] = {GLX_RGBA, GLX_RED_SIZE, 4,
        GLX_GREEN_SIZE, 4,
        GLX_BLUE_SIZE, 4,
        GLX_DEPTH_SIZE, 16,
        None};

    /* attributes for a double buffered visual in RGBA format with at least
     * 4 bits per color and a 16 bit depth buffer */
    static int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 
            GLX_RED_SIZE, 4, 
            GLX_GREEN_SIZE, 4, 
            GLX_BLUE_SIZE, 4, 
            GLX_DEPTH_SIZE, 16,
            None };
    XVisualInfo *vi;
    Colormap cmap;
    int dpyWidth, dpyHeight;
    int i;
    int glxMajorVersion, glxMinorVersion;
#ifdef HAVE_XF86VMODE_H
    int vidModeMajorVersion, vidModeMinorVersion;
    XF86VidModeModeInfo **modes;
#endif
    int modeNum;
    int bestMode;
    Atom wmDelete;
    Window winDummy;
    unsigned int borderDummy;

    m_glwin.fs = fullscreenflag;
    /* set best mode to current */
    bestMode = 0;
    /* get a connection */
    m_glwin.dpy = XOpenDisplay(0);
    m_glwin.screen = DefaultScreen(m_glwin.dpy);
#ifdef HAVE_XF86VMODE_H
    XF86VidModeQueryVersion(m_glwin.dpy, &vidModeMajorVersion,
                            &vidModeMinorVersion);
    printf("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion,
           vidModeMinorVersion);
    XF86VidModeGetAllModeLines(m_glwin.dpy, m_glwin.screen, &modeNum, &modes);
    /* save desktop-resolution before switching modes */
    m_glwin.deskMode = *modes[0];
    /* look for mode with requested resolution */
    for (i = 0; i < modeNum; i++)
    {
        if ((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height))
        {
            bestMode = i;
        }
    }
#endif
    /* get an appropriate visual */
    vi = glXChooseVisual(m_glwin.dpy, m_glwin.screen, attrListDbl);
    if (vi == NULL)
    {
        vi = glXChooseVisual(m_glwin.dpy, m_glwin.screen, attrListSgl);
        printf("Only Singlebuffered Visual!\n");
    }
    else
    {
        printf("Got Doublebuffered Visual!\n");
    }
    glXQueryVersion(m_glwin.dpy, &glxMajorVersion, &glxMinorVersion);
    printf("glX-Version %d.%d\n", glxMajorVersion, glxMinorVersion);
    /* create a GLX context */
    m_glwin.ctx = glXCreateContext(m_glwin.dpy, vi, 0, GL_TRUE);
    /* create a color map */
    cmap = XCreateColormap(m_glwin.dpy, RootWindow(m_glwin.dpy, vi->screen),
                           vi->visual, AllocNone);
    m_glwin.attr.colormap = cmap;
    m_glwin.attr.border_pixel = 0;

#ifdef HAVE_XF86VMODE_H
    if (m_glwin.fs)
    {
        XF86VidModeSwitchToMode(m_glwin.dpy, m_glwin.screen, modes[bestMode]);
        XF86VidModeSetViewPort(m_glwin.dpy, m_glwin.screen, 0, 0);
        dpyWidth = modes[bestMode]->hdisplay;
        dpyHeight = modes[bestMode]->vdisplay;
        printf("Resolution %dx%d\n", dpyWidth, dpyHeight);
        XFree(modes);
    
        /* create a fullscreen window */
        m_glwin.attr.override_redirect = True;
        m_glwin.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
                StructureNotifyMask;
        m_glwin.win = XCreateWindow(m_glwin.dpy, RootWindow(m_glwin.dpy, vi->screen),
                                  0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
                                  CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
                                  &m_glwin.attr);
        XWarpPointer(m_glwin.dpy, None, m_glwin.win, 0, 0, 0, 0, 0, 0);
        XMapRaised(m_glwin.dpy, m_glwin.win);
        XGrabKeyboard(m_glwin.dpy, m_glwin.win, True, GrabModeAsync,
                      GrabModeAsync, CurrentTime);
        XGrabPointer(m_glwin.dpy, m_glwin.win, True, ButtonPressMask,
                     GrabModeAsync, GrabModeAsync, m_glwin.win, None, CurrentTime);
    }
    else
    {
#endif
        /* create a window in window mode*/
        m_glwin.attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
                                  ButtonReleaseMask | ButtonMotionMask | StructureNotifyMask | PointerMotionMask;
        m_glwin.win = XCreateWindow(m_glwin.dpy, RootWindow(m_glwin.dpy, vi->screen),
                                  0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
                                  CWBorderPixel | CWColormap | CWEventMask, &m_glwin.attr);
        /* only set window title and handle wm_delete_events if in windowed mode */
        wmDelete = XInternAtom(m_glwin.dpy, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(m_glwin.dpy, m_glwin.win, &wmDelete, 1);
        XSetStandardProperties(m_glwin.dpy, m_glwin.win, title,
                               title, None, NULL, 0, NULL);
        XMapRaised(m_glwin.dpy, m_glwin.win);
#ifdef HAVE_XF86VMODE_H
    }       
#endif

    /* connect the glx-context to the window */
    glXMakeCurrent(m_glwin.dpy, m_glwin.win, m_glwin.ctx);
    XGetGeometry(m_glwin.dpy, m_glwin.win, &winDummy, &m_glwin.x, &m_glwin.y,
                 &m_glwin.width, &m_glwin.height, &borderDummy, &m_glwin.depth);
    printf("Depth %d\n", m_glwin.depth);
    if (glXIsDirect(m_glwin.dpy, m_glwin.ctx)) 
        printf("Congrats, you have Direct Rendering!\n");
    else
        printf("Sorry, no Direct Rendering possible!\n");
//    if (!initGL())
//    {
//        printf("Could not initialize OpenGL.\nAborting...\n");
//        return False;
//    }        
    return true;    
}

/** make the drawing surface current */
void GLXPlatformStandalone::glBegin()
{
    glXMakeCurrent(m_glwin.dpy, m_glwin.win, m_glwin.ctx);
}

/** swap the gl buffers */
void GLXPlatformStandalone::glSwapBuffers()
{
    glXSwapBuffers(m_glwin.dpy, m_glwin.win);
    // check of opengl errors
    GLenum errCode;
    const GLubyte * errString;
    errCode = glGetError();
    errString = gluErrorString(errCode);
    if (errCode != GL_NO_ERROR) {
        DEBUG_ERROR("OpenGL Error: " << errCode << ", " <<  errString);
    }
}


/* function to release/destroy our resources and restoring the old desktop */
void GLXPlatformStandalone::killGLWindow()
{
    if (m_glwin.ctx)
    {
        if (!glXMakeCurrent(m_glwin.dpy, None, NULL))
        {
            printf("Could not release drawing context.\n");
        }
        glXDestroyContext(m_glwin.dpy, m_glwin.ctx);
        m_glwin.ctx = NULL;
    }
    /* switch back to original desktop resolution if we were in fs */
#ifdef HAVE_XF86VMODE_H
    if (m_glwin.fs)
    {
        XF86VidModeSwitchToMode(m_glwin.dpy, m_glwin.screen, &m_glwin.deskMode);
        XF86VidModeSetViewPort(m_glwin.dpy, m_glwin.screen, 0, 0);
    }
#endif
    XCloseDisplay(m_glwin.dpy);
}

