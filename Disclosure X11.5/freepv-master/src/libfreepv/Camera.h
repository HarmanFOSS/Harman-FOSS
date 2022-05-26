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

#ifndef FPV_CAMERA_H
#define FPV_CAMERA_H

/** class around the camera */

namespace FPV
{

class Camera
{
public:
    Camera();
    ~Camera();

    void setFovSpeed(float speed, double time);
    void setYawSpeed(float speed, double time);
    void setPitchSpeed(float speed, double time);
    void setPitchLimits(float min, float max);
    void setYawLimits(float min, float max);
    void setFovLimits(float min, float max);
    void setDynamics(float angleDecay, float fovDecay);

    void setMaxYaw(float max);
    void setMinYaw(float min);
    void setMaxPitch(float max);
    void setMinPitch(float min);
    void setMaxFov(float max);
    void setMinFov(float min);
    void setAngleDecay(float angleDecay);
    void setFovDecay(float fovDecay);
    void setFOV(float f);
    void setPitch(float p);
    void setYaw(float y);

    float get_fov(void);
    float get_pitch(void);
    float get_yaw(void);

    Camera& operator=(const Camera &external_camera);

    /** timer function required for the simulation of
     *  inertia
     *  returns true if more animation is needed, false otherwise
     */
    bool onTimer(double time);

protected:
    float m_angleDecay;
    float m_fovDecay;

    float yaw;
    float pitch;
    float fov;

    // limits
    float m_maxYaw;
    float m_minYaw;
    float m_maxPitch;
    float m_minPitch;
    float m_minFov;
    float m_maxFov;

    float yaw_speed;
    float pitch_speed;
    float fov_speed;

    double m_fovStartTime, m_yawStartTime, m_pitchStartTime;
    double m_lastTime;
};

}

#endif
