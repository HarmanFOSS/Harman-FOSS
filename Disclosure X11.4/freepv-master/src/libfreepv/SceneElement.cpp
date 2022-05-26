/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Authors: Pablo d'Angelo <pablo.dangelo@web.de>
 *           Leon Moctezuma <densedev_at_gmail_dot_com>
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
#include "SceneElement.h"

using namespace FPV;

///////////////////////////////
//       SceneElement
///////////////////////////////

SceneElement::SceneElement(){
   m_type=GROUP;
   m_renderData=NULL;
   m_visible=true;
}

void SceneElement::Invalid()
{
   m_type=INVALID;
}

SceneElement::~SceneElement(){
}

quaternion SceneElement::getQuaternion(){
  return m_quaternion;
}

void SceneElement::setVisible(bool _value){
  m_visible=_value;
}

bool SceneElement::isVisible(){
  return m_visible;
}

///////////////////////////////
//       CubicPano
///////////////////////////////

CubicPano::CubicPano(Size2D sz)
{
  m_type = PANO_CUBIC;
  m_size = sz;
  for (int i=0; i < 6; i++) {
    m_images[i] = 0;
  }
}

CubicPano::CubicPano()
{
  m_type = PANO_CUBIC;
  m_size = Size2D(0,0);
  for (int i=0; i < 6; i++) {
    m_images[i] = 0;
  }
}

CubicPano::~CubicPano()
{
  for (int i=0; i < 6; i++) {
    if (m_images[i]) {
      delete m_images[i];
    }
  }
}

void CubicPano::setCubeFaces(Image * imgs[6]){
  for (int face=0; face < 6; face++) {
    if (m_images[face]) {
      delete m_images[face];
    }
    m_images[face] = imgs[face];
  }
        m_size = imgs[0]->size();
}

void CubicPano::setCubeFace(FaceID face, Image * img)
{
  assert(face < 6);
  if (m_images[face]) {
    delete m_images[face];
  }
  m_images[face] = img;
}

Image* CubicPano::getCubeFace(FaceID face){
  assert(face < 6);
  return m_images[face];
}

///////////////////////////////
//       CylindricalPano
///////////////////////////////

CylindricalPano::CylindricalPano(Size2D sz, float HFOV){
  m_type = PANO_CYLINDRICAL;
  m_image = 0;
  m_hfov = HFOV;
}

CylindricalPano::CylindricalPano(){
  m_type = PANO_CYLINDRICAL;
  m_image = 0;
  m_hfov = 0;
}

CylindricalPano::~CylindricalPano(){
  if (m_image) {
    delete m_image;
  }
}

void CylindricalPano::setImage(Image * img)
{
  if (m_image) {
    delete m_image;
  }
  m_image = img;
}
///////////////////////////////
//       FlatPano
///////////////////////////////

FlatPano::FlatPano(float HFOV){
   m_type = PANO_FLAT;
   m_image = 0;
   m_hfov = HFOV;
}

FlatPano::~FlatPano()
{
   if (m_image) {
      delete m_image;
   }
}

void FlatPano::setImage(Image * img)
{
   if (m_image) {
      delete m_image;
   }
   m_image = img;
}

///////////////////////////////
//        PlaceHolder
///////////////////////////////

PlaceHolder::PlaceHolder(){
   m_type = PLACE_HOLDER;
   m_image = 0;
}

PlaceHolder::~PlaceHolder()
{
}

void PlaceHolder::setImage(Image * img)
{
   if (m_image) {
      delete m_image;
   }
   m_image = img;
}

Image* PlaceHolder::getImage()
{
   return m_image;
}

///////////////////////////////
//       SphericalPano
///////////////////////////////

SphericalPano::SphericalPano(Size2D sz, float HFOV){
  m_type = PANO_SPHERICAL;
  m_image = 0;
  m_hfov = HFOV;
}

SphericalPano::SphericalPano(){
  m_type = PANO_SPHERICAL;
  m_image = 0;
  m_hfov = 0;
}

SphericalPano::~SphericalPano(){
  if (m_image) {
    delete m_image;
  }
}

void SphericalPano::setImage(Image * img)
{
  if (m_image) {
    delete m_image;
  }
  m_image = img;
}
