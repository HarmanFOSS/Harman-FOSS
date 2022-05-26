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

#ifndef FPV_SCENE_ELEMENT_H
#define FPV_SCENE_ELEMENT_H

#include "Image.h"
#include "Platform.h"
#include "Math/quaternion.h"
#include "Subject.h"

namespace FPV
{

  class RenderData{
  public:
    virtual ~RenderData() {};
  };

  /*!SceneElement class 
    This is the model that contains the panoramic image or
    any other scene elment that should be drawn on screen
  */

  class SceneElement : public Subject{
  public:
    /*! List of all scene element types */
    enum Type { INVALID, PANO_CUBIC, PANO_SPHERICAL, PANO_CYLINDRICAL, PANO_FLAT, GROUP, PLACE_HOLDER, TEXT};

    //!This quaternion keeps the angular displacement
    quaternion p;

    //!Contructor
    SceneElement();
    //!Destructor
    virtual ~SceneElement();

    /*! the render data is used by the renderer to store
      render-specific information about this element
      eg. OpenGL textures or similar
    */
    
    //! Variable to store specific renderer data
    RenderData * m_renderData;
    
    //!\return quaternion with the orientation of the element
    quaternion getQuaternion();

    //!\return type of element
    Type getType()
    {
        return m_type;
    }
    
    //!Invalidates the element
    void Invalid();

    quaternion m_quaternion;

    //!This method makes an element visible or invisible
    //!\param true for visible, false for invisible
    void setVisible(bool _value);
    
    //\return true if the element is visible
    bool isVisible();

 protected:
    Type m_type;
    bool m_visible;

};

  /*!CubicPano class stores the cubic panoramas*/

  class CubicPano : public SceneElement{
  public:
    
    enum FaceID { FACE_FRONT=0, FACE_RIGHT, FACE_BACK, FACE_LEFT, FACE_TOP, FACE_BOTTOM };

    //!Constructor
    //!\param creates 6 cubefaces with the given size
    CubicPano(Size2D sz);
    //!Constructor
    CubicPano();
    //!Destructor
    ~CubicPano();

    /*! Set all cubefaces. O
      Ownership of Images pointed to by 
      imgs is transferred to the CubicPanorama object.
      \note The cube face order is: front, right, back, left, top, bottom
    */
    void setCubeFaces(Image * imgs[6]);

    /*! Set a single cube face.
      Ownership of img is transferred to the CubicPanorama object.
    */
    void setCubeFace(FaceID face, Image * img);

    /*! get a single face */
    Image * getCubeFace(FaceID face);

    // cube face order: 'front', 'right', 'back', 'left', 'top', 'bottom'
    Image * m_images[6];

    // size of one face
    Size2D m_size;
};

/*!CylindricalPano class stores the cylindrical panoramas*/
  class CylindricalPano : public SceneElement{
  public:
    /*! construct a cylindrical panorama.
     */
    CylindricalPano(Size2D sz, float HFOV);

    CylindricalPano();

    ~CylindricalPano();

    /*! Set all cubefaces
      Ownership of Image pointed to by \p img is transferred to the CylindricalPano
      object.
    */
    void setImage(Image * img);

    Image * m_image;

    // hfov of pano
    double m_hfov;
};

/*!FlatPano class stores the flat panoramas*/
  class FlatPano : public SceneElement{
  public:
    
    FlatPano(float HFOV=90);
    ~FlatPano();
    void setImage(Image * img);
    Image * m_image;
    float hfov(){return m_hfov;}
    
  private:
    
    float m_hfov;
    float m_hoffset;
    float m_voffset;
  };
  
/*!PlaceHolder class keeps the place for a panorama*/
class PlaceHolder : public SceneElement
{
public:

    PlaceHolder();
    ~PlaceHolder();
    void setImage(Image * img);
    Image* getImage();

private:
    Image * m_image;
};

/*!SphericalPano class stores the spherical panoramas*/
 class SphericalPano : public SceneElement{
 public:
   /*! construct a spherical panorama.
    */
   SphericalPano(Size2D sz, float HFOV);
   SphericalPano();
   ~SphericalPano();

   /*! Set panorama image
     Ownership of Image pointed to by \p img is transferred to the CylindricalPano
     object.
   */
   void setImage(Image * img);

   //!Image of the panorama
   Image * m_image;
   
   //! hfov of pano
   double m_hfov;
};

///////////////////////////////
//       Text Msg
///////////////////////////////
class TextElement : public SceneElement
{
public:
    TextElement(const std::string & text)
    {
        m_type = TEXT;
        m_text = text;
    }
    std::string m_text;
};

}

#endif
