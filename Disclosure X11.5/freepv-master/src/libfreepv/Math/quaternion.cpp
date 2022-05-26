/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Leon Moctezuma <densedev_at_gmail_dot_com>
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
#include "quaternion.h"

void quaternion::identity(){
  w=1.0f;
  x=y=z=0.0f;
}

void quaternion::fromEulerAngles(float pan, float tilt, float roll){
  quaternion p, t, r;
  p.RotateAboutY(pan);
  t.RotateAboutX(tilt);
  r.RotateAboutZ(roll);
  *this = r*t*p;
}

void quaternion::toMatrix(Matrix4 &M){
  float *A=M.get();
  A[0]=1-2*(y*y+z*z);
  A[1]=2*(x*y+w*z);
  A[2]=2*(x*z-w*y);
  A[4]=2*(x*y-w*z);
  A[5]=1-2*(x*x+z*z);
  A[6]=2*(y*z+w*x);
  A[8]=2*(x*z+w*y);
  A[9]=2*(y*z-w*x);
  A[10]=1-2*(x*x+y*y);
  A[3]=A[7]=A[11]=A[12]=A[13]=A[14]=0.0f;
  A[15]=1.0f;
}

void quaternion::RotateAboutX(float deg){
  deg = (deg/180)*PI;
  float half_deg = deg/2;
  w = cos(half_deg);
  x = sin(half_deg);
  y = 0;
  z = 0;
}

void quaternion::RotateAboutY(float deg){
  deg = (deg/180)*PI;
  float half_deg = deg/2;
  w = cos(half_deg);
  x = 0;
  y = sin(half_deg);
  z = 0;
}

void quaternion::RotateAboutZ(float deg){
  deg = (deg/180)*PI;
  float half_deg = deg/2;
  w = cos(half_deg);
  x = 0;
  y = 0;
  z = sin(half_deg);
}

void quaternion::RotateAboutAxis(float deg, float c_x, float c_y, float c_z){
  deg = (deg/180)*PI;
  float half_deg = deg/2;
  float mag = sqrt(x*x+y*y+z*z);
  w = cos(half_deg);
  x = c_x/mag;
  y = c_y/mag;
  z = c_z/mag;
}

quaternion& quaternion::operator=(const quaternion& q){
  w=q.w;
  x=q.x;
  y=q.y;
  z=q.z;
  return *this;
}

quaternion quaternion::operator*(const quaternion& q){
  float p_w, p_x, p_y, p_z;
  p_w=w*q.w-x*q.x-y*q.y-z*q.z;
  p_x=w*q.x+x*q.w+z*q.y-y*q.z;
  p_y=w*q.y+y*q.w+x*q.z-z*q.x;
  p_z=w*q.z+z*q.w+y*q.x-x*q.y;
  quaternion p(p_w, p_x, p_y, p_z);
  return p;
}

quaternion& quaternion::operator*=(const quaternion& q){
  *this=*this*q;
  return *this;
}

void quaternion::set(float e_w, float e_x, float e_y, float e_z)
{
  w = e_w;
  x = e_x;
  y = e_y;
  z = e_z;
  normalize();
}

quaternion::quaternion(float e_w, float e_x, float e_y, float e_z)
{
  set(e_w, e_x, e_y, e_z);
}

void quaternion::normalize()
{
	float norm = sqrt(w*w+x*x+y*y+z*z);
	if(norm==1) return;
	w=w/norm;
	x=x/norm;
	y=y/norm;
	z=z/norm;
}

quaternion::quaternion()
{
  identity();
}

quaternion::~quaternion()
{
}
