/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Renderer.h 150 2008-10-15 14:18:53Z leonox $
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

#ifndef FPV_RENDERER_H
#define FPV_RENDERER_H

#include "Scene.h"
#include "Parameters.h"

namespace FPV
{

/** Abstract renderer, which is used to render the various
 *  Panoelements onto the screen
 */
class Renderer
{
public:

    virtual ~Renderer() {};

    /** Prepare render for rendering
     *
     *  This should be called if all resources required by
     *  the renderer are available (for example: opengl context)
     */
    virtual void init() = 0;

    /** Prepare for rendering this pano element.
     *
     *  This function is called whenever the SceneElement
     *  has changed (for example, after a new cube face is added)
     *  and should be used for initialisation of the rendering,
     *  creation of opengl textures and so on.
     */
    virtual void initElement(SceneElement & pano) = 0;

    /** Render the whole scene.*/
    virtual void render(Scene & scene) = 0;
    virtual std::list<Subject*>* getPointedSubjects(Scene &scene, Point2D point)=0;

    /** The window has been resized */
    virtual void resize(Size2D sz) = 0;

    /** Max Depth */
    virtual void max_depth(float)=0;
    
    /** Min Depth */
    virtual void min_depth(float)=0;
};

}
#endif
