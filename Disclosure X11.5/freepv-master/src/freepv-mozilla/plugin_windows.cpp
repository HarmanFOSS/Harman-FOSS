/*

Windows-specific code for the nsPluginInstance class

*/



/* try to include windows.h in a sensible way */
#define  WIN32_LEAN_AND_MEAN
#define WIN32_NOMINMAX
// for WM_MOUSEWHEEL
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <windowsx.h>

#include <mmsystem.h>

/* gl stuff */
#include <GL/gl.h>
#include <GL/glu.h>

#include "plugin_windows.h"
#include <libfreepv/PanoViewer.cpp>

static LRESULT CALLBACK PluginWinProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC lpOldProc = NULL;

#define FPV_WM_TIME (WM_USER+1)

/////////////////////////////////////////////////////////////////////////////////
// misc platform utility functions
//

// using netscape threads
extern "C" {
static void timerMain(void * data)
{
    nsPluginInstanceWin32 * thisp = (nsPluginInstanceWin32 *) data;
    while (!thisp->m_threadQuit) {
        // send our custom time event to our window..
        SendMessage(thisp->mhWnd, FPV_WM_TIME,0,0);
        // try to make the system more responsive. But this adds a lot
        // of jerkyness..
        Sleep(0);
    }
    printf("Exiting timerMain() thread");
}
}

// using windows threads
DWORD WINAPI timerMainWin32(LPVOID iValue)
{
    nsPluginInstanceWin32 * thisp = (nsPluginInstanceWin32 *) iValue;
    while (!thisp->m_threadQuit) {
        // send our custom time event to our window..
        SendMessage(thisp->mhWnd, FPV_WM_TIME,0,0);
        // try to make the system more responsive. But this adds a lot
        // of jerkyness..
        Sleep(0);
    }
    printf("Exiting timerMain() thread");
    return 0;
}


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


//////////////////////////////////////
//
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

/////////////////////////////////////////////////////////////
//
// construction and destruction of our plugin instance object
//
nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstanceWin32(aCreateDataStruct);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstanceWin32 *)aPlugin;
}


nsPluginInstanceWin32::nsPluginInstanceWin32(nsPluginCreateData *pcd)
    : nsPluginInstance(pcd),
      mX(0), mY(0), mWidth(0), mHeight(0),
      m_wheelDelta(0), m_modifiers(0),
      m_timerPtr(0),
      m_thread(0), m_threadQuit(false),
      m_viewer(0)
{
    DEBUG_TRACE("");
    m_viewer = new PanoViewer();
    m_viewer->init( *this, *m_fpvParam);
}

NPBool nsPluginInstanceWin32::init(NPWindow* aWindow)
{
    if(aWindow == NULL)
        return FALSE;

    mhWnd = (HWND)aWindow->window;
    if(mhWnd == NULL) {
        DEBUG_ERROR("aWindow->window contains a null pointer");
        return FALSE;
    }

    // subclass window so we can intercept window messages and
    // do our drawing to it
    lpOldProc = SubclassWindow(mhWnd, (WNDPROC)PluginWinProc);

    // associate window with our nsPluginInstance object so we can access 
    // it in the window procedure
    SetWindowLongPtr(mhWnd, GWL_USERDATA, (LONG_PTR)this);

    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;

    // get the device context (DC)
    m_hDC = GetDC( mhWnd );
    if (! m_hDC) {
        DEBUG_ERROR("GetDC() failed");
        return FALSE;
    }

    // set the pixel format for the DC
    ZeroMemory( &pfd, sizeof( pfd ) );
    pfd.nSize = sizeof( pfd );
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                  PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    iFormat = ChoosePixelFormat( m_hDC, &pfd );
    if (iFormat == 0) {
        DEBUG_ERROR("ChoosePixelFormat failed: " << GetLastError());
        return FALSE;
    }
    if (!SetPixelFormat( m_hDC, iFormat, &pfd )){
        DEBUG_ERROR("ChoosePixelFormat failed: " << GetLastError());
        return FALSE;
    }

    // create and enable the render context (RC)
    m_hRC = wglCreateContext( m_hDC );
    if (! m_hRC) {
      DEBUG_ERROR("wglCreateContext failed: " << GetLastError());
      return FALSE;
    }
//    ReleaseDC(mhWnd, m_hDC);
//    m_hDC = 0;

    mInitialized = TRUE;

    mX = aWindow->x;
    mY = aWindow->y;
    mWidth = aWindow->width;
    mHeight = aWindow->height;
    wglMakeCurrent( m_hDC, m_hRC );
    m_viewer->onResize(Size2D(mWidth, mHeight));

    m_viewer->start();

    //startTimerThread();
    return TRUE;
}

void nsPluginInstanceWin32::shut()
{
    // TODO: stop events
    stopTimer();
    delete (m_viewer);

    wglDeleteContext( m_hRC );
    ReleaseDC( mhWnd, m_hDC );

    // subclass it back
    SubclassWindow(mhWnd, lpOldProc);
    mhWnd = NULL;
    mInitialized = FALSE;
}

NPError nsPluginInstanceWin32::SetWindow(NPWindow* aWindow)
{
    DEBUG_TRACE("");
    if ((aWindow == NULL) || (aWindow->window == NULL)) {
        DEBUG_DEBUG("no valid window");
        return NPERR_NO_ERROR;
    }
    if (aWindow->x == mX && aWindow->y == mY
        && aWindow->width == mWidth
        && aWindow->height == mHeight
        && (HWND)(aWindow->window) == mhWnd) 
    {
        DEBUG_DEBUG("called with same window as before");
        return NPERR_NO_ERROR;
    } else {
        DEBUG_DEBUG("called with new window");
    }

    mX = aWindow->x;
    mY = aWindow->y;
    mWidth = aWindow->width;
    mHeight = aWindow->height;
    return NPERR_NO_ERROR;
}

void nsPluginInstanceWin32::startTimer(int delay)
{
    PostMessage(mhWnd, FPV_WM_TIME, 0, 0);

#if 0
    if (m_timerPtr == 0) {
        m_timerPtr = SetTimer( mhWnd, 1, 1000.0/80, NULL);
    }
    if (m_timerPtr == 0) {
        DEBUG_ERROR("Could not start timer");
    }

    DWORD dwGenericThread;
    m_threadWin32 = CreateThread(NULL,0,timerMainWin32,this,0,&dwGenericThread);
    if(m_threadWin32 == NULL)
    {
        DWORD dwError = GetLastError();
        DEBUG_ERROR("SCM:Error in Creating thread"<<dwError);
    }
    // start our timer thread
    if (m_thread == 0) {
        DEBUG_TRACE("starting timer thread");
        m_thread = PR_CreateThread(PR_USER_THREAD,
                                (void (__cdecl *)(void *))timerMain,
                                this,
                                PR_PRIORITY_NORMAL,
                                PR_GLOBAL_THREAD,
                                PR_JOINABLE_THREAD,
                                0);
        if (m_thread == 0) {
            DEBUG_ERROR("Could not start timer thread");
        }
    }
#endif
}

void nsPluginInstanceWin32::stopTimer()
{
#if 0
    if (m_timerPtr) {
        KillTimer(mhWnd, m_timerPtr);
        m_timerPtr = 0;
    }
    if (m_threadWin32) 
    {
        DEBUG_TRACE("setting thread quite flag");
        m_threadQuit = true;
        //WaitForSingleObject(m_threadWin32, INFINITE);
        DEBUG_TRACE("thread stopped");
    }

    if (m_thread) {
        DEBUG_TRACE("stopping thread");
        m_threadQuit = true;
        PR_JoinThread(m_thread);
        DEBUG_TRACE("thread stopped");
    }
    m_thread = 0;
#endif
}


/** make the drawing surface current */
void nsPluginInstanceWin32::glBegin()
{
    // get the device context (DC)
//    m_hDC = GetDC( mhWnd );
//    wglMakeCurrent( m_hDC, m_hRC );
}

void nsPluginInstanceWin32::glEnd()
{
//    ReleaseDC(mhWnd, m_hDC);
//    m_hDC = 0;
}

/** swap the gl buffers */
void nsPluginInstanceWin32::glSwapBuffers()
{
    SwapBuffers( m_hDC );

    GLenum errCode;
    const GLubyte * errString;
    errCode = glGetError();
    errString = gluErrorString(errCode);
    if (errCode != GL_NO_ERROR) {
        DEBUG_ERROR("OpenGL Error: " << errCode << ", " <<  errString);
    }
}


// translate modifiers send while receiving mouse events
short convertModifiers(WPARAM wParam)
{
    short mod = 0;
    if (wParam & MK_CONTROL)
        mod |= CONTROL_MASK;
    if (wParam & MK_SHIFT)
        mod |= SHIFT_MASK;
    if (wParam & MK_LBUTTON)
        mod |= BUTTON1_MASK;
    if (wParam & MK_MBUTTON)
        mod |= BUTTON2_MASK;
    if (wParam & MK_RBUTTON)
        mod |= BUTTON3_MASK;
    return mod;
}

LRESULT CALLBACK nsPluginInstanceWin32::PluginWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    nsPluginInstanceWin32 * thisp = (nsPluginInstanceWin32 *)GetWindowLong(hWnd, GWL_USERDATA);
    if (!thisp) {
        DEBUG_ERROR("no valid this pointer given");
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_SIZE:								// Resize The OpenGL Window
		{
            DEBUG_TRACE("WM_SIZE");
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onResize(FPV::Size2D(LOWORD(lParam),HIWORD(lParam)));
            }

            switch (wParam) 
            { 
                case SIZE_MINIMIZED:  
                    DEBUG_TRACE("Window minimized");
                    //thisp->stopTimerThread();
                    break; 
 
                case SIZE_RESTORED: 
                case SIZE_MAXIMIZED:
                    DEBUG_TRACE("Window maximised");
                    //thisp->startTimerThread();
            } 
            return 0L; 

		}
        break;
    case WM_PAINT:
        {
            DEBUG_TRACE("WM_PAINT");
            PAINTSTRUCT ps; 
            BeginPaint(thisp->mhWnd, &ps); 
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onRedraw(0,0,0,0, 0);
            }
            EndPaint(thisp->mhWnd, &ps); 
        }
        break;
    case WM_TIMER:
    case FPV_WM_TIME:
        {
            static oldt = 0;
            double t = getTime();
            DEBUG_TRACE("WM_TIMER: time: " << t << "s  delta:" << (t-oldt)*1000 << " ms");
            oldt = t;
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onTimer(t);
            }            
        }
        break;
        // convert button press
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
		{
            //DEBUG_TRACE("mouse button event");
            thisp->m_modifiers = convertModifiers(wParam);
            MouseEvent mev;
            mev.pos.x = GET_X_LPARAM(lParam); 
            mev.pos.y = GET_Y_LPARAM(lParam); 
            mev.modifiers = thisp->m_modifiers;
            if (msg == WM_LBUTTONDOWN ||msg == WM_LBUTTONUP) {
                mev.down = (msg == WM_LBUTTONDOWN);
                mev.buttonNr=1;
            } else if (msg == WM_MBUTTONDOWN ||msg == WM_MBUTTONUP) {
                mev.down = (msg == WM_MBUTTONDOWN);
                mev.buttonNr=2;
            } else {
                mev.down = (msg == WM_RBUTTONDOWN);
                mev.buttonNr=3;
            }
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onMouseEvent(mev);
            }
            // repaint
            double t = getTime();
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onTimer(t);
            }            

    	}
		break;
    case WM_MOUSEWHEEL:
        {
            // TODO: this doesn't work. No WM_MOUSEWHEEL event arrive...
            thisp->m_wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
            DEBUG_DEBUG("wheelDelta: " << thisp->m_wheelDelta);
            MouseEvent mev;
            mev.pos.x = GET_X_LPARAM(lParam); 
            mev.pos.y = GET_Y_LPARAM(lParam); 
            mev.modifiers = thisp->m_modifiers;
            // handle possibly smooth mouse wheels
            if (thisp->m_wheelDelta > WHEEL_DELTA) {
                thisp->m_wheelDelta = 0;
                mev.buttonNr = 4;
            } else if (thisp->m_wheelDelta < -WHEEL_DELTA) {
                thisp->m_wheelDelta = 0;
                mev.buttonNr = 5;
            }
            mev.down = true;
        }
        break;
	case WM_MOUSEMOVE:
		{
            DEBUG_TRACE("WM_MOUSEMOVE: " << getTime());
            thisp->m_modifiers = convertModifiers(wParam);
            MouseEvent mev;
            mev.pos.x = GET_X_LPARAM(lParam); 
            mev.pos.y = GET_Y_LPARAM(lParam); 
            mev.down = false;
            mev.modifiers = thisp->m_modifiers;
            mev.buttonNr=0;
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onMouseEvent(mev);
            }
            // repaint
            double t = getTime();
            if (thisp->m_eventListener) {
                wglMakeCurrent( thisp->m_hDC, thisp->m_hRC );
                thisp->m_eventListener->onTimer(t);
            }            
    	}
		break;

		case WM_ACTIVATE:						// Watch For Window Activate Message
		{
            if (!HIWORD(wParam)) {					// Check Minimization State
                DEBUG_TRACE("Window activated");
                //thisp->startTimer();
            } else {
                DEBUG_TRACE("Window deactivated");
                //thisp->stopTimer();
            }

		}
        break;
        
        case WM_KEYDOWN:	
		{
            DEBUG_TRACE("keydown");
		}
        break;
        case WM_KEYUP:
		{
            DEBUG_TRACE("keyup");
		}
        break;
    default:
        //DEBUG_TRACE("Unknown event: " << msg);
        // pass all other events to the previous window
        return DefWindowProc(hWnd, msg, wParam, lParam);
        break;
    }
    return 0;
}

#if 0
// requires a repaint of the plugin
void nsPluginInstanceWin32::Repaint(void)
{
	InvalidateRect( mhWnd, NULL, TRUE );
	UpdateWindow( mhWnd );
}

// handles the WM_LBUTTONDOWN message
void nsPluginInstanceWin32::OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	OutputDebugString( "LButtonDown\n" );
}


// handles the WM_LBUTTONDOWN message
void nsPluginInstanceWin32::OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	OutputDebugString( "LButtonUp\n" );
			
}
#endif
