/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Controller.h 150 2008-10-15 14:18:53Z leonox $
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


#ifndef FPV_CONTROLLER_H
#define FPV_CONTROLLER_H

#include "utils.h"
#include "math.h"
#include "FPV_keysyms.h"

namespace FPV
{

/** This is an controller that listens to mouse events and
 *  changes the panorama accordingly
 */
class Controller
{

public:
    Controller(Scene * scene)
    : m_scene(scene), 
      m_yawSpeed(0), m_pitchSpeed(0), m_fovSpeed(0),
      m_setFovOnce(false),
      m_setYaw(false), m_setPitch(false), m_setFov(false),
      m_keyZoomOut(false), m_keyZoomIn(false),
      m_keyLeft(false), m_keyRight(false), m_keyUp(false), m_keyDown(false)
    { }

    virtual ~Controller() {};

    /** called when a mouse event happend 
     *  return true if further redraws (animation) are needed
     */
    virtual bool onMouseEvent(const MouseEvent & mouse)
    {
        bool animate = false;
        // mouse button was pressed down
        if (mouse.buttonNr == 1 && mouse.down) {
            m_mouseDownPoint = mouse.pos;
        }
        // mouse button is down, calcuate new scrolling speed
        if (mouse.modifiers & BUTTON1_MASK) {
            Point2D diff = mouse.pos - m_mouseDownPoint;
            // calculate speed in degree/s
            m_yawSpeed = (float)diff.x/200.0f * m_scene->getCamera()->get_fov(); 
            m_pitchSpeed = (float)diff.y/200.0f * m_scene->getCamera()->get_fov();
            m_setYaw = true;
            m_setPitch = true;
            animate = true;
        }

        // stop updating the yaw/pitch, if the mousebutton was
        // released
		if (mouse.buttonNr == 1 && !mouse.down) {
            m_setYaw = false;
            m_setPitch = false;
        }

        // if mouse wheel was used,
        if (mouse.buttonNr == 4 && mouse.down) {
            // calculate speed in a way that if the movement would take 1/2 second,
            // the visible horizontal length of the image is half of what it is now
            m_fovSpeed = 2.0f*(2.0f*r2d(atan(0.5f*tan(d2r(m_scene->getCamera()->get_fov())/2.0f))) - m_scene->getCamera()->get_fov());
            // reset speed on next update
            m_setFov= true;
            m_setFovOnce = true;
            animate = true;
        }

        if (mouse.buttonNr == 5 && mouse.down) {
//            m_fovSpeed = 50;
            // calculate speed in a way that if the movement would take 1/2 second,
            // the visible horizontal length of the image is double of what it is now
            m_fovSpeed = -2.0f*(2.0f*r2d(atan(0.5f*tan(d2r(m_scene->getCamera()->get_fov())/2.0f))) - m_scene->getCamera()->get_fov());
            // reset speed on next update
            m_setFov= true;
            m_setFovOnce = true;
            animate = true;
        }
        return animate;
    }

    /** called when a key event has happend */
    virtual bool onKeyEvent(const KeyEvent & event)
    {
        bool animate = false;
        // TODO: implement keyboard control
        if ((event.keysym == FPV_Shift_L || event.keysym == FPV_equal) && event.down) {
            m_keyZoomIn = true;
            animate = true;
        };
        if ((event.keysym == FPV_Shift_L || event.keysym == FPV_equal) && !event.down) {
            m_keyZoomIn = false;
            animate = true;
        }

        if ((event.keysym == FPV_Control_L || event.keysym == FPV_minus) && event.down) {
            m_keyZoomOut = true;
            animate = true;
        };
        if ((event.keysym == FPV_Control_L || event.keysym == FPV_minus) && !event.down) {
            m_keyZoomOut = false;
            animate = true;
        }

        if ((event.keysym == FPV_Left) && event.down) {
            m_keyLeft = true;
            animate = true;
        };
        if ((event.keysym == FPV_Left) && !event.down) {
            m_keyLeft = false;
            animate = true;
        };

        if ((event.keysym == FPV_Right) && event.down) {
            m_keyRight = true;
            animate = true;
        };
        if ((event.keysym == FPV_Right) && !event.down) {
            m_keyRight = false;
            animate = true;
        };

        if ((event.keysym == FPV_Up) && event.down) {
            m_keyUp = true;
            animate = true;
        };
        if ((event.keysym == FPV_Up) && !event.down) {
            m_keyUp = false;
            animate = true;
        };

        if ((event.keysym == FPV_Down) && event.down) {
            m_keyDown = true;
            animate = true;
        };
        if ((event.keysym == FPV_Down) && !event.down) {
            m_keyDown = false;
            animate = true;
        };

        return animate;
    }

    /** timer event 
     *
     */
    virtual bool onTimer(double time) {
		if (m_setYaw) {
            m_scene->getCamera()->setYawSpeed(m_yawSpeed, time);
        }
		if (m_setPitch) {
            m_scene->getCamera()->setPitchSpeed(m_pitchSpeed, time);
		}
        if (m_setFov) {
            m_scene->getCamera()->setFovSpeed(m_fovSpeed, time);
        }
        if (m_keyZoomIn) {
            m_fovSpeed = 2.0f*(2.0f*r2d(atan(0.5f*tan(d2r(m_scene->getCamera()->get_fov())/2.0f))) - m_scene->getCamera()->get_fov());
            m_scene->getCamera()->setFovSpeed(m_fovSpeed, time);
        }
        if (m_keyZoomOut) {
            m_fovSpeed = -2.0f*(2.0f*r2d(atan(0.5f*tan(d2r(m_scene->getCamera()->get_fov())/2.0f))) - m_scene->getCamera()->get_fov());
            m_scene->getCamera()->setFovSpeed(m_fovSpeed, time);
        }
        if (m_keyLeft) {
            m_yawSpeed = -0.75f * m_scene->getCamera()->get_fov(); 
            m_scene->getCamera()->setYawSpeed(m_yawSpeed, time);
        }
        if (m_keyRight) {
            m_yawSpeed = 0.75f * m_scene->getCamera()->get_fov(); 
            m_scene->getCamera()->setYawSpeed(m_yawSpeed, time);
        }
        if (m_keyUp) {
            m_pitchSpeed = -0.75f * m_scene->getCamera()->get_fov(); 
            m_scene->getCamera()->setPitchSpeed(m_pitchSpeed, time);
        }
        if (m_keyDown) {
            m_pitchSpeed = 0.75f * m_scene->getCamera()->get_fov(); 
            m_scene->getCamera()->setPitchSpeed(m_pitchSpeed, time);
        }

        bool redrawNeeded = m_scene->getCamera()->onTimer(time);

        if (m_setFovOnce) {
            m_setFov = false;
        }
        return redrawNeeded;
    }

protected:
    Scene * m_scene;
    Point2D m_mouseDownPoint;
    float m_yawSpeed, m_pitchSpeed, m_fovSpeed;
    bool m_setFovOnce;
    bool m_setYaw, m_setPitch, m_setFov;
    bool m_keyZoomOut, m_keyZoomIn;
    bool m_keyLeft, m_keyRight, m_keyUp, m_keyDown;
};

} // namespace

#endif
