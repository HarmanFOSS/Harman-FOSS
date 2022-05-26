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

#include "Camera.h"
#include <math.h>
#include <float.h>

namespace FPV
{

Camera::Camera()
{
    yaw = pitch = 0;
    fov = 70;
    yaw_speed = 0;
    pitch_speed = 0;
    fov_speed = 0;

    m_yawStartTime = DBL_MAX;
    m_pitchStartTime = DBL_MAX;
    m_fovStartTime = DBL_MAX;

    m_minPitch = -90;
    m_maxPitch = 90;
    m_maxYaw = 180;
    m_minYaw = -180;

    m_minFov = 1;
    m_maxFov = 160;

    m_angleDecay = 3.0f;
    m_fovDecay = 5.0f;

    m_lastTime = 0;
}

Camera::~Camera()
{

}

void Camera::setMaxYaw(float max){
    if(max>180)
	max=180;
    else if(max<-180)
	max=-180;
    m_maxYaw=max;
}

void Camera::setMinYaw(float min){
    if(min>180)
	min=180;
    else if(min<-180)
	min=-180;
    m_minYaw=min;
}

void Camera::setMaxPitch(float max){
    if(max>90)
	max=90;
    else if(max<-90)
	max=-90;
    m_maxPitch=max;
}

void Camera::setMinPitch(float min){
    if(min>90)
	min=90;
    else if(min<-90)
	min=-90;
    m_minPitch=min;
}

void Camera::setMaxFov(float max){
    if(max>170)
	max=170;
    else if(max<0.1f)
	max=0.1f;
    m_maxFov=max;
}

void Camera::setMinFov(float min){
    if(min>170)
	min=90;
    else if(min<0.1f)
	min=0.1f;
    m_minFov=min;
}

void Camera::setAngleDecay(float angleDecay){
    m_angleDecay=angleDecay;
}

void Camera::setFovDecay(float fovDecay){
    m_fovDecay=fovDecay;
}

void Camera::setFOV(float f)
{
    if(f>170)
	f=170;
    else if(f<0.1f)
	f=0.1f;
    fov=f;
}

void Camera::setPitch(float p)
{
    if(p>90)
        p=90;
    else if(p<-90)
        p=-90;
    pitch=p;
}

void Camera::setYaw(float y)
{
    if(y>180)
	y=180;
    else if(y<-180)
	y=-180;
    yaw=y;
}

float Camera::get_fov(){return fov;}
float Camera::get_pitch(){return pitch;}
float Camera::get_yaw(){return yaw;}

Camera& Camera::operator=(const Camera &external_camera)
{
    yaw = external_camera.yaw; 
    pitch = external_camera.pitch;
    fov = external_camera.fov;
    m_minPitch = external_camera.m_minPitch;
    m_maxPitch = external_camera.m_maxPitch;
    m_maxYaw = external_camera.m_maxYaw;
    m_minYaw = external_camera.m_minYaw;

    m_minFov = external_camera.m_minFov;
    m_maxFov = external_camera.m_maxFov;

    m_angleDecay = external_camera.m_angleDecay;
    m_fovDecay = external_camera.m_fovDecay;

	return *this;
}


void Camera::setFovSpeed(float speed, double time)
{
    fov_speed = speed;
    m_fovStartTime = time;
}

void Camera::setYawSpeed(float speed, double time)
{
    yaw_speed = speed;
    m_yawStartTime = time;
}

void Camera::setPitchSpeed(float speed, double time)
{
    pitch_speed = speed;
    m_pitchStartTime = time;
}


bool Camera::onTimer(double time)
{
    // delta time might not be accurate if the timer has just been restarted..
    double dt = time - m_lastTime;
    if (m_lastTime == 0) {
        dt = 0;
    }

    double yspeed = yaw_speed;
    // calculate decay. This time it is independent from the update rate
    if ( m_yawStartTime < m_lastTime) {
        double decayTime = time - m_yawStartTime; 
        yspeed = yaw_speed * exp(-m_angleDecay* decayTime);
    }
    double pspeed = pitch_speed;
    if ( m_pitchStartTime < m_lastTime) {
        double decayTime = time - m_pitchStartTime; 
        pspeed = pitch_speed * exp(-m_angleDecay* decayTime);
    }
    double fspeed = fov_speed;
    if ( m_fovStartTime < m_lastTime) {
        double decayTime = time - m_fovStartTime; 
        fspeed = fov_speed * exp(-m_fovDecay* decayTime);
    }

    m_lastTime = time;


    //DEBUG_DEBUG("time step: " << dt << " sec.");
    yaw += (float)(yspeed * dt);
    pitch += (float)(pspeed * dt);
    fov += (float)(fspeed * dt);
    
    /*
    DEBUG_DEBUG("time:  " << m_yawStartTime << "   " << m_pitchStartTime << "   " << m_fovStartTime << "  " << time);
    DEBUG_DEBUG("pos:   " << yaw << "   " << pitch << "   " << fov);
    DEBUG_DEBUG("speed: " << yspeed << "   " << pspeed << "   " << fspeed);
    */

    if (fov > m_maxFov)
        fov = m_maxFov;
    if (fov < m_minFov)
        fov = m_minFov;

    // ensure yaw stays within -180 .. 180
    while (yaw > 180) yaw -= 360;
    while (yaw < -180) yaw +=360;

    if (pitch > m_maxPitch) pitch = m_maxPitch;
    if (pitch < m_minPitch) pitch = m_minPitch;

    if (yaw > m_maxYaw) yaw = m_maxYaw;
    if (yaw < m_minYaw) yaw = m_minYaw;

    // check if the velocities is still high enought to warrant animation
    bool animate = ( fabs(yspeed) > 0.01 * fov) || ( fabs(pspeed) > 0.01 * fov) || ( fabs(fspeed) > 0.01 * fov);
    if (!animate) {
        // set to 0, indicate that the next delta is not valid
        m_lastTime = 0;
    }
    return animate;
}
}
