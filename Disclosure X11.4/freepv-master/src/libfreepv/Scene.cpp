/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Authors: Pablo d'Angelo <pablo.dangelo@web.de>
 *           Leon Moctezuma <densedev_at_gmail_dot_com>
 *
 *  $Id: Scene.cpp 150 2008-10-15 14:18:53Z leonox $
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

#include "Scene.h"
#include "utils.h"

using namespace FPV;

Scene::Scene()
    : m_pano(0)
{
    m_SceneElementRoot = NULL;
    m_ui = new TextElement("");
    std::cerr<<"a scene was created"<<std::endl;
}

Scene::~Scene()
{
    if (m_SceneElementRoot) {
        delete(m_SceneElementRoot);
    }
    if (m_ui) {
        delete m_ui;
    }
}

void Scene::setSceneElement(SceneElement * elem)
{
    if (m_SceneElementRoot) {
        delete(m_SceneElementRoot);
    }
    m_SceneElementRoot = new SceneElementNode(elem);
}

void Scene::setSceneElement(SceneElementNode * elem)
{
    if (m_SceneElementRoot) {
        delete(m_SceneElementRoot);
    }
    m_SceneElementRoot = elem;
}

SceneElementNode * Scene::getSceneElementRoot()
{
       return m_SceneElementRoot;
}

void Scene::setStatusText(const std::string & text)
{
    DEBUG_TRACE(text);
    m_ui->m_text = text;
}

SceneElement * Scene::getUIElement()
{
    return m_ui;
}

Camera * Scene::getCamera()
{
    return &m_camera;
}

std::map<std::string,Behavior>* Scene::getBehaviorMap(){
    return &m_behaviors;
}

Behavior* Scene::getBehavior(const char* _id){
    if(!_id)
	return (NULL);
    std::string s_id = _id;
    std::map<std::string,Behavior>::iterator iter = m_behaviors.find(s_id);
    if(iter!=m_behaviors.end())
	return &(iter->second);
    else
	return NULL;
}

void Scene::addBehavior(const char* _id, Behavior &_behavior){
    if(!_id)
	return;
    std::string s_id = _id;
    std::map<std::string,Behavior>::iterator iter = m_behaviors.find(s_id);
    if(iter!=m_behaviors.end())
	m_behaviors.erase(iter);
    m_behaviors.insert(m_behaviors.begin(),make_pair(s_id,_behavior));
}



