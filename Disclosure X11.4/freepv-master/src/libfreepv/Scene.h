/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Authors: Pablo d'Angelo <pablo.dangelo@web.de>
 *           Leon Moctezuma <densedev_at_gmail_dot_com>
 *
 *  $Id: Scene.h 150 2008-10-15 14:18:53Z leonox $
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

#ifndef FPV_PANORAMA_H
#define FPV_PANORAMA_H

#include <assert.h>

#include "Platform.h"
#include "NodeElement.h"
#include "Camera.h"
#include <map>
#include <string>

namespace FPV
{


/** This holds the scene.
 *
 *  A scene consists of a panoramic SceneElement, one camera and one
 *  user interface element (like a status text line, or a controller)
 *
 *  @TODO: support multiple pano and ui elements
 */
class Scene
{

public:
    Scene();

    virtual ~Scene();

    void setStatusText(const std::string & text);

    void setSceneElement(SceneElement * elem);
    void setSceneElement(SceneElementNode * elem);
    SceneElementNode * getSceneElementRoot();

//    void setUIElement(SceneElement * elem);
    SceneElement * getUIElement();
    Camera * getCamera();
    SceneElementNode *m_SceneElementRoot;
    void addBehavior(const char* _id, Behavior &_behavior);
    Behavior* getBehavior(const char* _id);
    std::map<std::string,Behavior>* getBehaviorMap();
private:
    /// our single scene element (probably only a pano...)
    SceneElement * m_pano;
    TextElement * m_ui;
    std::map<std::string,Behavior> m_behaviors;
    
    /// the camera
    Camera m_camera;
};


}
#endif
