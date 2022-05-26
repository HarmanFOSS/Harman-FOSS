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

#ifndef FPV_QUATERNION_H
#define FPV_QUATERNION_H

#include <math.h>
#include "Matrix4.h"

const float PI = 3.1416f;

/*!Class used to store the rotatio in a quaternion representation*/
class quaternion
{
 private:
  float w, x, y, z;
 public:
  //!Constructor
  quaternion(float w, float x, float y, float z);
  //!Constructor
  quaternion();
  //!Destructor
  ~quaternion();
  void identity();
  //!Transforms from euler representation to quaternion
  void fromEulerAngles(float tilt, float pan, float fov);
  //!Transforms from quaternion to a Matrix representation
  void toMatrix(Matrix4 &M);
  /*!Rotate about axis X n degrees*/
  void RotateAboutX(float deg);
  /*!Rotate about axis Y n degrees*/
  void RotateAboutY(float deg);
  /*!Rotate about axis Z n degrees*/
  void RotateAboutZ(float deg);
  /*!Rotate about arbitrary axis n degrees*/
  void RotateAboutAxis(float deg, float x, float y, float z);
  void set(float e_w, float e_x, float e_y, float e_z);
  quaternion& operator=(const quaternion& q);
  quaternion& operator*=(const quaternion& q);
  quaternion operator*(const quaternion& q);
  void normalize();
};

#endif
