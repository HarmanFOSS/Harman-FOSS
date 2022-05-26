/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Platform.h 101 2006-12-01 23:42:33Z dangelo $
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

/* This file contains a platform independent interface for the
 *  freepv player
 */

#ifndef FPV_PLATFORM_H
#define FPV_PLATFORM_H

//#include <vector>
#include <string>

#include "utils.h"

namespace FPV
{


enum ModifierType 
{
    SHIFT_MASK    = 1 << 0,
    LOCK_MASK     = 1 << 1,
    CONTROL_MASK  = 1 << 2,
    MOD1_MASK     = 1 << 3,
    MOD2_MASK     = 1 << 4,
    MOD3_MASK     = 1 << 5,
    MOD4_MASK     = 1 << 6,
    MOD5_MASK     = 1 << 7,
    BUTTON1_MASK  = 1 << 8,
    BUTTON2_MASK  = 1 << 9,
    BUTTON3_MASK  = 1 << 10,
    BUTTON4_MASK  = 1 << 11,
    BUTTON5_MASK  = 1 << 12,
};

// to do: add utility functions Button1Pressed() and similar
struct MouseEvent
{
    MouseEvent()
    {
        modifiers = 0;
        buttonNr = 0;
        down = false;
        time = 0;
    }

    /// check if event is a mouse move event
    bool isMove() 
    {
        return buttonNr == 0;
    }
    double time; ///< time in seconds
    Point2D pos;  ///< mouse pointer position relative to widget origin (0,0) top left
    short modifiers;  ///< modifiers (mouse buttons, keys)
    int buttonNr;   ///< button number (1-5) if a specific button was pressed or released, 0 if no change
    bool down;      ///< true: the mouse button was just pressed, false: the mouse button was just released
};

// to do: add utility functions Button1Pressed() and similar
struct KeyEvent{
    KeyEvent()
    { 
        modifiers = 0;
        keysym = 0;
        down = false;
        time = 0;
    }

    double time; ///< time in seconds
    short modifiers; ///< modifiers (mouse buttons, keys)
    unsigned int keysym;    ///< use keysymbols in FPV_keysyms.h
    bool down;              ///< the key was just pressed down
};

/** Interface for GUI events from the platform */
class PlatformEventListener
{
public:
    virtual ~PlatformEventListener() {};

    /** Called whenever the window is resized */
    virtual void onResize(Size2D size)  {};

    /** called when the window (or a part of it) needs to be redrawn 
     *
     *  @param x,y  upper left corner of area that should be redrawn
     *  @param w,h  width and height of area that should be redrawn
     *  @param count number of pending areas requiring a update.
     *               If the function redraws the full screen, it
     *               should just respond to calls with count == 0.
     */
    virtual void onRedraw(int x, int y, int w, int h, int count) {};

    /** called when a mouse event happend */
    virtual void onMouseEvent(const MouseEvent & event) {};

    /** called when a key event has happend */
    virtual void onKeyEvent(const KeyEvent & event) {};

    /** called before the instance is destroyed.
     *
     *  This function should acknowledge the destruction with a call
     *  to Platform::quit()
     */
    virtual void onDestroy() {};

    /** called when a timer event happens.
     *
     *  @param time contains the absolute time in seconds since some
     *                   fixed point in the past.
     */
    virtual void onTimer(double time) {};

    // =================================================
    // download events

    /** notify about the current download progress.
     *
     * @param data pointer to start of available data. Can be 0, even if
     *             @p downloadedBytes is nonzero. Additionally, the pointer
     *             might change on consecutive calls.
     *             This usually happens if the file size is not known
     *             when at download starting time.
     * @param downloadedBytes number of downloaded bytes. might be approximate,
     *                        if @p data is 0.
     *
     * @param size Total size of download. Might be 0, if the size is not known
     *
     */
    virtual void onDownloadProgress(void * data, size_t downloadedBytes, size_t sz) = 0;

    /** notify about a finished download 
     *
     *  Ownership of the \p data buffer is transferred to the
     *  called function. The buffer should be released using free().
     */
    virtual void onDownloadComplete(void * data, size_t downloadedBytes) = 0;

    /** notify about a finished file download.
     *
     *  This file should not be deleted after it has been read.
     */
    virtual void onDownloadComplete(const std::string & filename) = 0;

};

/** This is a singleton object, with functions required for
 *  communication with the outside world.
 *
 *  Every port needs to provide a suitable implementation of
 *  this class.
 */
class Platform
{

public:
    virtual ~Platform() {};

    /** set data supplied to the timer callback */
    void setCBData(void * data);

    /** register listener object for User input */
    void setListener(PlatformEventListener * listener);

    /** quit the program.
     * 
     *  Should be called as response to EventListener::onDestory()
     *  @param ret the return code returned to the shell, if
     *             this is not a browser plugin
     */
    virtual void quit(int ret) {};

    ////////////////////////////////////////////////
    // Timer functions

    /** start the timer, fires in \p delay milliseconds
     *  note that the delay is only a hint and OnTime()
     *  might be called earlier or later, dependent on
     *  the platform and the current machine load.
     *
     *  This is a one shot timer. If a periodic timer
     *  is required, call startTimer() in the timer function.
     */
    virtual void startTimer(int delay) = 0;

    /** stop the timer */
    virtual void stopTimer() = 0;

    ////////////////////////////////////////////////
    // Drawing functions

    /** make the drawing surface current */
    virtual void glBegin() = 0;

    /** required by some platforms (gtkglext) */
    virtual void glEnd() = 0;

    /** swap the gl buffers */
    virtual void glSwapBuffers() = 0;


    ////////////////////////////////////////////////

    /** Download an URL to memory.
     *
     *  This function will call onAllocateDownloadBuffer()
     *  before starting the downloand.
     *  It might report the download progress with
     *  onDownloadProgress().
     *  onDownloadComplete() will be called once the download
     *  has been completed.
     */
    virtual bool startDownloadURL(const std::string & url) = 0;

    /** Download an URL to a file.
     *
     *  It might report the download progress with
     *  onDownloadProgress().
     *  onDownloadComplete() will be called once the download
     *  has been completed.
     *
     *  As a special case, if the file is local, it will just
     *  be delivered by onDownloadComplete().
     *
     *  The file should not be deleted after usage.
     */
    virtual bool startDownloadURLToFile(const std::string & url) = 0;

    /** get the URL that is currently beeing downloaded */
    virtual const std::string & currentDownloadURL() = 0;

    /** get MIME type of the currenly being downloaded URL.
     *
     *  Can be an empty string if no mimetype is known.
     */
    virtual std::string currentDownloadMimeType() = 0;

protected:
    Platform();

    PlatformEventListener * m_eventListener;
};


}

#endif
