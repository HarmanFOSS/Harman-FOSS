/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Thomas Rauscher <t.rauscher@sinnfrei.at>
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: glut_platform.cpp 155 2009-02-22 10:56:20Z brunopostle $
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

#include <libfreepv/FPV_keysyms.h>

#ifdef _WIN32
#define FREEGLUT_STATIC
#endif

#ifdef HAVE_OSXGLUT_H
#include <GLUT/glut.h>
#elif defined HAVE_FREEGLUT_H
#include <GL/freeglut.h>
#elif defined HAVE_GLUT_H
#include <GL/glut.h>
#else
#error "GLUT automake/cmake check failed"
#endif

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <mmsystem.h>
#endif

#include "glut_platform.h"

// prototype for timer callback function
void timerCallback(int id);

extern GLUTPlatformStandalone * platformptr;


void idleCallback() {
	platformptr->glutIdleCallback();
}

static int convertModifiers(int glutMod)
{
    int ret = 0;
    if (glutMod & GLUT_ACTIVE_SHIFT) {
        ret |= SHIFT_MASK;
    }
    if (glutMod & GLUT_ACTIVE_CTRL) {
        ret |= CONTROL_MASK;
    }
    if (glutMod & GLUT_ACTIVE_ALT) {
        ret |= MOD1_MASK;
    }
    return ret;
}

#ifdef _WIN32

static double 
getTimeInaccurate()
{
    static unsigned long oldTime= 0;
    unsigned long currTime;
    unsigned long deltaTime;
    currTime = timeGetTime();
    if (oldTime == 0) {
        oldTime = currTime;
    }
    deltaTime = currTime-oldTime;
    return deltaTime/ 1000.0;
}

// return the elapsed time since the first call in seconds.
static double 
getTime()
{
    LARGE_INTEGER currTime;
    LARGE_INTEGER deltaTime;
    static LARGE_INTEGER oldTime = {0,0};
    static LARGE_INTEGER ticksPerSecond = {0,0};

    if (ticksPerSecond.QuadPart == 0) {
        if (!QueryPerformanceFrequency(&ticksPerSecond)) 
        {
            // no performance counter, use low resolution system clock
            ticksPerSecond.QuadPart = 0;
            return getTimeInaccurate();
        }
    }

    QueryPerformanceCounter(& currTime);

    if (oldTime.QuadPart == 0) {
        oldTime.QuadPart = currTime.QuadPart;
    }

    deltaTime.QuadPart = currTime.QuadPart - oldTime.QuadPart;
    if (deltaTime.QuadPart == 0) return 0;
    return (deltaTime.QuadPart / ((double)ticksPerSecond.QuadPart));
} 

#endif

#ifdef HAVE_GETTIMEOFDAY
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
#endif

GLUTPlatformStandalone::GLUTPlatformStandalone(int argc, char ** argv)
{
    m_running = true;
    m_exitcode = 0;
    /* default to fullscreen */
    m_glwin.fs = false;

    m_currentGlutMod = 0;

	glutInit(&argc, argv);

	if (!createGLWindow("freepv", 640, 480, 24, m_glwin.fs))
    {
        fprintf(stderr, "creation of GL window failed, exiting\n");
        m_running = false;
        m_exitcode = 1;
    }
}

bool GLUTPlatformStandalone::startDownloadURL(const std::string & url)
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
	void * buffer=malloc(sz);
//    void * buffer = m_eventListener->onAllocateDownloadBuffer(sz);
    size_t readsz = fread(buffer, 1, sz, f);
    m_eventListener->onDownloadComplete( buffer, readsz );
    return true;
}

const std::string & GLUTPlatformStandalone::currentDownloadURL() {
	return m_currentURL;
}

bool GLUTPlatformStandalone::startDownloadURLToFile(const std::string & url)
{
    m_currentURL = url;
    m_eventListener->onDownloadComplete( url );
    return true;
}


void GLUTPlatformStandalone::quit(int ret)
{
    m_running = false;
    m_exitcode = ret;
}


void GLUTPlatform::glutDisplayCallback(void) {
    // processing a timer event always enforces a redraw
    double t = getTime();
	m_eventListener->onTimer(t);
}

void GLUTPlatform::glutReshapeCallback(int width, int height)
{
    m_eventListener->onResize(FPV::Size2D(width, height));
}

/* TODO: implement special keys (using glutSpecialFunc callback) */
void GLUTPlatform::glutKeyboardSpecialCallback(int key,int x, int y, bool down) {
    KeyEvent kev;
    kev.keysym = 0;
    DEBUG_DEBUG("key: " << key << " x:" << x << " y:" << y);
    // TODO: need to translate to keysyms here
    switch (key) {
        case GLUT_KEY_UP:
            kev.keysym = FPV_Up;
            break;
        case GLUT_KEY_DOWN:
            kev.keysym = FPV_Down;
            break;
        default:
            break;
    }

    if (kev.keysym) {
        kev.down = down;
        // TODO: add mouse buttons and modifier keys here (shift/ctrl/alt/meta etc)
        kev.modifiers = convertModifiers(glutGetModifiers()); 
	    m_eventListener->onKeyEvent(kev);
    }
}


void GLUTPlatform::glutKeyboardCallback(unsigned char key,int x, int y, bool down) 
{
    DEBUG_DEBUG("keycode conversion not implemented yet");

    KeyEvent kev;

    kev.modifiers = convertModifiers(glutGetModifiers());
    // TODO: need to translate to keysyms here
	kev.keysym=key;
    kev.down = down;
    // TODO: add mouse buttons and modifier keys here (shift/ctrl/alt/meta etc)
    kev.modifiers = 0; 
    kev.time = getTime();
	//m_eventListener->onKeyEvent(kev);
}

void GLUTPlatform::glutMouseCallback(int button, int state,int x, int y) {
	MouseEvent mev;
    printf("glutMouseCallback, button %d, state %d, pos: %d, %d\n", button, state, x, y);
    mev.modifiers = convertModifiers(glutGetModifiers());

	mev.pos.x=x;
	mev.pos.y=y;
    int mask;
	if (button==GLUT_LEFT_BUTTON)  {
		mev.buttonNr=1;
        mask = BUTTON1_MASK;
	} else if (button==GLUT_RIGHT_BUTTON) {
		mev.buttonNr=3;
        mask = BUTTON3_MASK;
    } else if (button==GLUT_MIDDLE_BUTTON) {
		mev.buttonNr=2;
        mask = BUTTON2_MASK;
    } else {
        // unknown button, ignore
        return;
    }
    if (state == GLUT_DOWN) {
		mev.down=true;
        m_modifiers |= mask;
    } else {
		mev.down=false;
        m_modifiers &= ~mask;
    }

    mev.modifiers = m_modifiers;
    mev.time = getTime();

	m_eventListener->onMouseEvent(mev);
}


void GLUTPlatform::glutScrollWheelCallback(int button, int state,int x, int y)
{
	MouseEvent mev;
    //printf("glutScrollWheelCallback, button %d, state %d, pos: %d, %d\n", button, state, x, y);
	mev.pos.x=x;
	mev.pos.y=y;
    mev.modifiers = m_modifiers;
	mev.down=true;
    mev.time = getTime();

    if (state == 1) {
        // up
        mev.buttonNr = 4;
    } else {
        // down
        mev.buttonNr = 5;
    }
	m_eventListener->onMouseEvent(mev);
}

void GLUTPlatform::glutMouseMotionCallback(int x, int y) {
	MouseEvent mev;
    printf("glutMouseMotion, pos: %d, %d\n", x, y);
	mev.buttonNr=0;
	mev.down=false;
	mev.modifiers=m_modifiers;
	mev.pos.x=x;
	mev.pos.y=y;
	mev.down=false;
    mev.time = getTime();
	m_eventListener->onMouseEvent(mev);
}

void GLUTPlatform::glutIdleCallback(void) {
    // force glut rendering
	glutPostRedisplay();
}

// currently unused, render as fast as possible
void GLUTPlatform::glutOnTimerCallback(void) {
    // call timer function, which does the animation and other stuff.
    double t = getTime();
//    printf("glutOnTimerCallback, t= %d\n", t);
	m_eventListener->onTimer(t);
    // restart timer
   	glutTimerFunc(10,timerCallback,0);
}

void GLUTPlatform::ProcessEvents(void)
{
    /* the event loop */
//	glutDisplayFunc(renderScene);
	glutMainLoop();
}

/** swap the gl buffers */
void GLUTPlatform::glSwapBuffers()
{
//    glutSwapBuffers(m_glwin.dpy, m_glwin.win);
	glutSwapBuffers();
}

/* this function creates our window and sets it up properly */
/* FIXME: bits is currently unused */
bool GLUTPlatformStandalone::createGLWindow(char* title,
                                           int width, int height, int bits,
                                           bool fullscreenflag)
{
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_ALPHA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(width,height);
	glutCreateWindow(title);
	return true;    
}

void GLUTPlatform::startTimer(int delay)
{
    //DEBUG_DEBUG("Timer start");
	glutIdleFunc (idleCallback);
};

void GLUTPlatform::stopTimer()
{
    DEBUG_DEBUG("Timer stop");
    glutIdleFunc(0);
};

/* function to release/destroy our resources and restoring the old desktop */
void GLUTPlatformStandalone::killGLWindow()
{
}

