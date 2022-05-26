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

#ifndef FPV_PLUGIN_WINDOWS_H
#define FPV_PLUGIN_WINDOWS_H


#include <prthread.h>

//#include <libfreepv/glutfont/freeglut_font_copy.h>

#include "plugin.h"

namespace FPV
{

class PanoViewer;

class nsPluginInstanceWin32 : public nsPluginInstance
{

public:
    // modified constructor to receive the plugin creation data
    nsPluginInstanceWin32(nsPluginCreateData *pcd);

  // these three methods must be implemented in the derived
  // class platform specific way
  virtual NPBool init(NPWindow* aWindow);
  virtual void shut();

  // implement all or part of those methods in the derived
  // class as needed
  virtual NPError SetWindow(NPWindow* pNPWindow);
  virtual void    Print(NPPrint* printInfo)                         { return; }
//  virtual uint16  HandleEvent(void* event);
  /*
  virtual void    URLNotify(const char* url, NPReason reason,
                            void* notifyData)                       { return; }
  virtual NPError GetValue(NPPVariable variable, void *value)       { return NPERR_NO_ERROR; }
  virtual NPError SetValue(NPNVariable variable, void *value)       { return NPERR_NO_ERROR; }
  */


    // window event callbacks..
    static LRESULT CALLBACK PluginWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    /*
    // handles the WM_LBUTTONDOWN message
	void OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);

	// handles the WM_LBUTTONUP message
	void OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);

    void Repaint(void);
    */
protected:

    int mX, mY;
    int mWidth, mHeight;

    // platform dependent variables and functions
public:
    HWND mhWnd;
protected:
    HDC  m_hDC;
    HGLRC  m_hRC;

    int m_wheelDelta;
    int m_modifiers;

    //void startTimerThread();
    //void stopTimerThread();
    UINT_PTR m_timerPtr;
public:
    // hack, need to be accessed from the thread main function
    PRThread * m_thread;
    bool m_threadQuit;
    HANDLE m_threadWin32;
    
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
