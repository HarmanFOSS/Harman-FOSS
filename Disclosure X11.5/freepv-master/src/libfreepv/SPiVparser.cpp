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

#include "SPiVparser.h"
#include <iostream>
#include <assert.h>
#include "Error.h"
#include <cstring>

using namespace FPV;

SPiVparser::SPiVparser()
{
    m_doc = NULL;
    m_root = NULL;
    m_global = NULL;
    m_current_scene = NULL;
    m_global_camera = NULL;
}

SPiVparser::~SPiVparser()
{
    xmlFreeDoc(m_doc);
}

bool SPiVparser::loadTour()
{
    Camera *p_camera;

    if (m_doc != NULL)
    {
        m_root = xmlDocGetRootElement(m_doc);

        if (xmlStrcmp(m_root->name, (const xmlChar *)"tour"))
        {
            std::cerr << "This file has a wrong root node" << std::endl;
            m_root = NULL;
        }
    }
    return false;
}

bool SPiVparser::loadScene(Scene &sceneToLoad, const char *id)
{
    std::string status;
    xmlNodePtr p_aux = NULL;
    xmlNodePtr p_meta = NULL;
    m_scene = &sceneToLoad;

    imgList.clear();

    if (m_root != NULL)
    {
        if (id == NULL && m_global == NULL)
            loadGlobal();
        //If the id is a null pointer we try to load the
        //default scene, first by looking in the
        //global node.If not, we load the first one
        //specified in the tour.

        if (id == NULL)
        {
            p_aux = NULL;
            //In case of having a global node, we try to find
            //the meta node to check if the default scene is
            //specified.
            if (m_global != NULL)
            {
                p_aux = FindElement(m_global, "defaultview", NULL, NULL, DEPTH);
                if (p_aux != NULL)
                    p_aux = FindElement(m_root, "scene", "id", (const char *)xmlGetProp(p_aux, (const xmlChar *)"scene"), WIDE);
            }
            if (p_aux == NULL)
                p_aux = FindElement(m_root, "scene", NULL, NULL, WIDE);
            if (p_aux != NULL)
            {
                m_current_scene = p_aux;
                p_aux = FindElement(m_root, "meta", NULL, NULL, WIDE);
                setupCamera(p_aux, sceneToLoad.getCamera());
                status.append("The current scene is ");
                //use auto_ptr
                const char *id = (const char *)xmlGetProp(m_current_scene, (const xmlChar *)"id");
                if (id != NULL)
                {
                    status.append(id);
                    sceneToLoad.setStatusText(status);
                    loadSceneBehaviors(m_current_scene, sceneToLoad);
                    sceneToLoad.setSceneElement(setupScene(m_current_scene));
                    return true;
                }
                else
                {
                    status = "SPiVparser: Error-> Scene without id";
                    sceneToLoad.setStatusText(status);
                    return false;
                }
            }
            else
            {
                status.append("SPiVparser: didn't find a default scene");
                sceneToLoad.setStatusText(status);
            }
        }

        //If the id is specified, then we try to find
        //a scene inside the tour with this id
        //if we do, then we load it.
        else
        {
            p_aux = NULL;
            p_aux = FindElement(m_root, "scene", "id", id, WIDE);
            if (p_aux != NULL)
            {
                m_current_scene = p_aux;
                p_aux = FindElement(m_root, "meta", NULL, NULL, WIDE);
                setupCamera(p_aux, sceneToLoad.getCamera());
                status.append("The current scene is ");
                status.append((const char *)xmlGetProp(m_current_scene, (const xmlChar *)"id"));
                sceneToLoad.setStatusText(status);
                loadSceneBehaviors(m_current_scene, sceneToLoad);
                sceneToLoad.setSceneElement(setupScene(m_current_scene));
                return true;
            }
            else if (m_current_scene == NULL)
            {
                loadScene(sceneToLoad);
            }
        }
    }
    return false;
}

void SPiVparser::loadSceneBehaviors(xmlNodePtr _node, Scene &_scene)
{
    if (!validateElement(_node, "scene", NULL, NULL))
        return;
    if (m_global != NULL)
    {
        behaviorList::iterator iter = m_global_behaviors.begin();
        while (iter != m_global_behaviors.end())
        {
            _scene.addBehavior(iter->first.c_str(), iter->second);
            iter++;
        }
    }
    xmlNodePtr behavior_node = NULL;
    xmlNodePtr action = NULL;
    behavior_node = _node->children;
    while (behavior_node != NULL)
    {
        if (validateElement(behavior_node, "behavior", "id", NULL))
        {
            Behavior scene_behavior;
            const char *id = (const char *)xmlGetProp(behavior_node, (const xmlChar *)"id");
            action = behavior_node->children;
            while (action != NULL)
            {
                getAction(action, scene_behavior);
                action = action->next;
            }
            _scene.addBehavior(id, scene_behavior);
            std::cerr << "A behavior has been added" << std::endl;
        }
        behavior_node = behavior_node->next;
    }
}

EventType SPiVparser::getEventType(const unsigned char *event)
{
    if (cmp("enter", event))
    {
        return FPV_ENTER;
    }
    if (cmp("leave", event))
    {
        return FPV_LEAVE;
    }
    if (cmp("press", event))
    {
        return FPV_PRESS;
    }
    if (cmp("release", event))
    {
        return FPV_RELEASE;
    }
    return FPV_UNKNOWN;
}

void SPiVparser::getAction(xmlNodePtr action, Behavior &behavior)
{
    if (validateElement(action, NULL, "event", NULL))
    {
        if (validateElement(action, "action", "type", "setView"))
        {
            float _fov = 45;
            float _yaw = 0;
            float _pitch = 0;

            if (xmlHasProp(action, (const xmlChar *)"fov"))
                _fov = getFloat(xmlGetProp(action, (const xmlChar *)"fov"));
            if (xmlHasProp(action, (const xmlChar *)"pan"))
                _yaw = getFloat(xmlGetProp(action, (const xmlChar *)"pan"));
            if (xmlHasProp(action, (const xmlChar *)"tilt"))
                _pitch = getFloat(xmlGetProp(action, (const xmlChar *)"tilt"));

            const char *_scene = (const char *)xmlGetProp(action, (const xmlChar *)"scene");
            const unsigned char *_event = xmlGetProp(action, (const xmlChar *)"event");

            EventType type = getEventType(_event);

            Action *behavior_action = new SetView(_fov, _yaw, _pitch, _scene);
            behavior.add(behavior_action, type);
        }
    }
}

SceneElementNode *SPiVparser::setupScene(xmlNodePtr node)
{
    SceneElement *pano = NULL;
    SceneElement *element = NULL;
    SceneElementNode *group = NULL;
    SceneElementNode *elementNode = NULL;
    xmlNodePtr children = node->children;
    //Node's ID
    const char *id = NULL;
    const char *behavior_id = NULL;
    //Node's zorder.
    float depth;
    while (children != NULL)
    {
        id = (char *)xmlGetProp(children, (const xmlChar *)"id");
        if (xmlHasProp(children, (const xmlChar *)"zorder") != NULL)
        {
            depth = 10 - getFloat(xmlGetProp(children, (const xmlChar *)"zorder"));
        }
        else
        {
            depth = 9;
        }
        if (cmp("panogroup", children->name))
        {
            element = new SceneElement();
            group = new SceneElementNode(element, id, depth);
            getElementRotation(children, element);
            if (elementNode)
            {
                SceneElementNode *aux = NULL;
                if (elementNode->getDepth() < depth)
                {
                    aux = elementNode;
                    elementNode = group;
                    elementNode->setSibling(aux);
                }
                else
                {
                    elementNode->setSibling(group);
                }
            }
            else
            {
                elementNode = group;
            }
            group->setChildren(setupScene(children));
        }
        else if (cmp("panoelement", children->name))
        {
            Image *img = NULL;
            pano = NULL;

            id = (const char *)xmlGetProp(children, (const xmlChar *)"id");
            behavior_id = (const char *)xmlGetProp(children, (const xmlChar *)"behavior");

            if (validateElement(children, NULL, "type", "flat"))
            {
                float hfov = 8;
                std::cerr << "SPiVparser: element type=flat" << std::endl;
                getElementImage(children, img);
                if (validateElement(children, NULL, "hfov", NULL))
                    hfov = getFloat(xmlGetProp(children, (const xmlChar *)"hfov"));
                if (img != NULL)
                {
                    pano = new FlatPano(hfov);
                    ((FlatPano *)pano)->setImage(img);
                    getElementRotation(children, pano);
                }
            }
            else if (validateElement(children, NULL, "type", "cubic"))
            {
                std::cerr << "SPiVparser: element type=cubic" << std::endl;
                getElementImage(children, img);
                if (img != NULL)
                {
                    pano = pano = new CubicPano();
                    ((CubicPano *)pano)->setCubeFace((CubicPano::FaceID)0, img);
                    getElementRotation(children, pano);
                }
                getElementRotation(children, pano);
            }
            else if (validateElement(children, NULL, "type", "cylindrical"))
            {
                std::cerr << "SPiVparser: element type=cylindrical" << std::endl;
                getElementImage(children, img);
                if (img != NULL)
                {
                    pano = new CylindricalPano();
                    ((SphericalPano *)pano)->setImage(img);
                    getElementRotation(children, pano);
                }
            }
            else if (validateElement(children, NULL, "type", "spherical"))
            {
                std::cerr << "SPiVparser: element type=spherical" << std::endl;
                getElementImage(children, img);
                if (img != NULL)
                {
                    pano = new SphericalPano();
                    ((SphericalPano *)pano)->setImage(img);
                    getElementRotation(children, pano);
                }
            }
            else
            {
                std::cerr << "SPiVparser: element type=placeholder" << std::endl;
                getElementImage(children, img);
                if (img != NULL)
                {
                    pano = new PlaceHolder();
                    ((PlaceHolder *)pano)->setImage(img);
                    getElementRotation(children, pano);
                }
            }
            //If a pano element was created, then we need to add it
            //to the SceneElements tree.
            if (pano)
            {
                if (xmlHasProp(children, (const xmlChar *)"visible"))
                {
                    bool value = getBool(xmlGetProp(children, (const xmlChar *)"visible"));
                    pano->setVisible(value);
                }
                if (xmlHasProp(children, (const xmlChar *)"enable"))
                {
                    bool value = getBool(xmlGetProp(children, (const xmlChar *)"enable"));
                    pano->enable(value);
                }
                if (xmlHasProp(children, (const xmlChar *)"catchevents"))
                {
                    bool value = getBool(xmlGetProp(children, (const xmlChar *)"catchevents"));
                    pano->catchEvents(value);
                }
                //attach inline behavior
                loadInlineBehaviors(children, (Subject *)pano);
                //attach referenced behavior
                Behavior *behavior = m_scene->getBehavior(behavior_id);
                if (behavior)
                {
                    pano->attach(behavior);
                }
                std::cerr << "SPiVparser: A panoelement was created" << std::endl;
                if (elementNode)
                {
                    //We add a sibling node
                    SceneElementNode *aux = NULL;
                    if (elementNode->getDepth() < depth)
                    {
                        aux = elementNode;
                        elementNode = new SceneElementNode(pano, id, depth);
                        elementNode->setSibling(aux);
                    }
                    else
                    {
                        elementNode->setSibling(pano, id, depth);
                    }
                }
                else
                {
                    //We create a node.
                    elementNode = new SceneElementNode(pano, id, depth);
                }
            }
        }
        children = children->next;
    }
    return elementNode;
}

void SPiVparser::getElementImage(xmlNodePtr node, Image *&img)
{
    xmlNodePtr img_node = NULL;
    xmlNodePtr img_layer = NULL;
    //We need to find the image node inside the element
    img_node = FindElement(node->children, "image", NULL, NULL, WIDE);
    if (img_node != NULL)
    {
        //In case we find it inside the element we need to find a layer
        //for the moment it just suppor base images, with type bitmap
        //in the future it should support other ones.
        getElementImageLayers(img_node, img);
    }
    else
    {
        //probably buggy... must check it
        if (xmlHasProp(node, (const xmlChar *)"image"))
        {
            const char *image = (const char *)xmlGetProp(node, (const xmlChar *)"image");
            img_node = FindElement(m_root, "image", "id", image, WIDE);
            if (img_node != NULL)
            {
                std::string id = image;
                imageList::iterator iter = imgList.find(id);
                //First we look inside the list
                if (iter != imgList.end())
                {
                    img_info info = iter->second;
                    img = info.img;
                }
                else
                {
                    //If it's not in the list we try to load the layers specified in the node
                    getElementImageLayers(img_node, img);
                }
            }
        }
    }
}

void SPiVparser::getElementImageLayers(xmlNodePtr node, Image *&img)
{
    xmlNodePtr img_layer = FindElement(node->children, "layer", "class", "base", WIDE);
    if (img_layer != NULL)
    {
        if (validateElement(img_layer, "layer", "type", "bitmap"))
        {
            //Now that we found the layer we wanted, we need to create a new image
            img_info info;

            //get the name of the img file
            const char *src = (const char *)xmlGetProp(img_layer, (const xmlChar *)"src");

            if (src != NULL)
            {
                //get the path from the xml file
                std::string path = m_path;
                std::string file_name = src;
                //concatenate the path and the name
                path += file_name;
                //set the url to download
                info.url = path;
                img = new Image();
                info.img = img;
                std::string id = (const char *)xmlGetProp(node, (const xmlChar *)"id");
                imgList.insert(imgList.begin(), std::make_pair(id, info));
            }
        }
    }
}

void SPiVparser::getElementRotation(xmlNodePtr node, SceneElement *element)
{
    float tilt = 0, pan = 0, roll = 0;
    if (validateElement(node, NULL, "tilt", NULL))
    {
        tilt = getFloat(xmlGetProp(node, (const xmlChar *)"tilt"));
    }
    if (validateElement(node, NULL, "pan", NULL))
    {
        pan = -getFloat(xmlGetProp(node, (const xmlChar *)"pan"));
    }
    if (validateElement(node, NULL, "roll", NULL))
    {
        roll = getFloat(xmlGetProp(node, (const xmlChar *)"roll"));
    }
    element->m_quaternion.fromEulerAngles(pan, tilt, roll);
}

bool SPiVparser::setupCamera(xmlNodePtr node, Camera *sceneCamera)
{
    xmlNodePtr p_aux = NULL;
    xmlAttrPtr p_attr = NULL;

    if (!validateElement(node, "meta", NULL, NULL))
        return false;

    std::cerr << "Setting the camera up" << std::endl;
    if (m_global_camera != NULL)
    {
        *sceneCamera = *m_global_camera;
    }

    if (node == NULL)
        return false;

    p_aux = FindElement(node->children, "cameralimits", NULL, NULL, WIDE);
    if (p_aux != NULL)
    {
        std::cerr << "Reading camera limits" << std::endl;
        for (p_attr = p_aux->properties; p_attr != NULL; p_attr = p_attr->next)
        {
            if (cmp("panmin", p_attr->name))
                sceneCamera->setMinPitch(getFloat(p_attr->children->content));
            else if (cmp("panmax", p_attr->children->content))
                sceneCamera->setMaxPitch(getFloat(p_attr->children->content));
            else if (cmp("tiltmin", p_attr->name))
                sceneCamera->setMinYaw(getFloat(p_attr->children->content));
            else if (cmp("tiltmax", p_attr->name))
                sceneCamera->setMaxYaw(getFloat(p_attr->children->content));
            else if (cmp("fovmin", p_attr->name))
                sceneCamera->setMinFov(getFloat(p_attr->children->content));
            else if (cmp("fovmax", p_attr->name))
                sceneCamera->setMaxFov(getFloat(p_attr->children->content));
        }
    }

    p_aux = FindElement(node->children, "cameradynamics", NULL, NULL, WIDE);
    if (p_aux != NULL)
    {
    }

    p_aux = FindElement(node, "defaultview", NULL, NULL, WIDE);
    if (p_aux != NULL)
    {
        std::cerr << "Reading defaultview" << std::endl;
        for (p_attr = p_aux->properties; p_attr != NULL; p_attr = p_attr->next)
        {
            if (cmp("fov", p_attr->name))
                sceneCamera->setFOV(getFloat(p_attr->children->content));
            else if (cmp("pan", p_attr->name))
                sceneCamera->setYaw(getFloat(p_attr->children->content));
            else if (cmp("tilt", p_attr->name))
                sceneCamera->setPitch(getFloat(p_attr->children->content));
        }
    }

    return true;
}

//This method remplace the nodes that has src attributes
void SPiVparser::parseNodeURL(const char *_url)
{
    const char *aux_url = NULL;
    const char *url = (char *)_url;
    if (aux_url = strrchr(_url, '/'))
    {
        url = aux_url;
        url++;
    }

    xmlDocPtr m_aux_doc = xmlParseFile(_url);
    xmlNodePtr doc_root = NULL;
    xmlNodePtr old_node = NULL;
    xmlNodePtr new_node = NULL;

    if (m_aux_doc != NULL)
    {
        doc_root = xmlDocGetRootElement(m_aux_doc);
        old_node = FindElement(m_root, NULL, "src", url, WIDE);
        while (old_node != NULL)
        {
            new_node = FindElement(doc_root, (const char *)old_node->name, NULL, NULL, WIDE);
            if (new_node != NULL)
            {
                old_node = xmlReplaceNode(old_node, new_node);
                xmlFreeNode(old_node);
            }
            else
            {
                xmlUnlinkNode(old_node);
                xmlFreeNode(old_node);
            }
            old_node = FindElement(m_root, NULL, "src", url, WIDE);
        }
        xmlFreeDoc(m_aux_doc);
    }
}

const char *SPiVparser::getImgURLToDownload()
{
    imageList::iterator iter = imgList.begin();
    if (iter != imgList.end())
    {
        return iter->second.url.c_str();
    }
    return NULL;
}

Image *SPiVparser::extractImage()
{
    Image *img;
    imageList::iterator iter = imgList.begin();
    if (iter != imgList.end())
    {
        img = iter->second.img;
        imgList.erase(iter);
    }
    return img;
}

void SPiVparser::loadInlineBehaviors(xmlNodePtr _node, Subject *_sub)
{
    xmlNodePtr behavior_node = NULL;
    xmlNodePtr action = NULL;
    behavior_node = _node->children;
    while (behavior_node != NULL)
    {
        if (validateElement(behavior_node, "behavior", "id", NULL))
        {
            Behavior inline_behavior;
            const char *id = (const char *)xmlGetProp(behavior_node, (const xmlChar *)"id");
            action = behavior_node->children;
            while (action != NULL)
            {
                getAction(action, inline_behavior);
                action = action->next;
            }
            m_scene->addBehavior(id, inline_behavior);
            Behavior *behavior = m_scene->getBehavior(id);
            _sub->attach(behavior);
        }
        behavior_node = behavior_node->next;
    }
}

const char *SPiVparser::getXMLToDownload()
{
    char *names[] = {"scene", "panogroup", "global"};
    int i = 0;
    if (m_root == NULL)
        return NULL;
    xmlNodePtr node = NULL;
    xmlChar *src = xmlCharStrdup("src");
    xmlChar *file;
    const char *c_file = NULL;
    while (node == NULL && i < 3)
    {
        node = FindElement(m_root, names[i], "src", NULL, WIDE);
        if (node == NULL)
            i++;
    }
    if (node != NULL)
    {
        file = xmlGetProp(node, (const xmlChar *)"src");
        if (file == NULL)
            std::cerr << "we got a null pointer" << std::endl;
        c_file = (const char *)file;
        std::cerr << "The file to download is " << c_file << std::endl;
        //xmlFree(file);
    }
    return c_file;
}

void SPiVparser::loadGlobal()
{
    Camera *p_camera;
    xmlNodePtr p_aux = NULL;
    xmlNodePtr p_meta = NULL;
    if (m_root != NULL)
    {
        p_aux = m_root->children;
        while (p_aux != NULL)
        {
            if (!xmlStrcmp(p_aux->name, (const xmlChar *)"global"))
            {
                if (m_global == NULL)
                {
                    m_global = p_aux;
                    p_meta = FindElement(m_global, "meta", NULL, NULL, DEPTH);
                    if (p_meta != NULL)
                    {
                        p_camera = new Camera();
                        setupCamera(p_meta, p_camera);
                        m_global_camera = p_camera;
                    }
                    loadGlobalBehaviors(m_global);
                }
            }
            p_aux = p_aux->next;
        }
    }
}

void SPiVparser::loadGlobalBehaviors(xmlNodePtr _node)
{
    if (!validateElement(_node, "global", NULL, NULL))
        return;
    xmlNodePtr behavior_node = NULL;
    xmlNodePtr action = NULL;
    behavior_node = _node->children;
    Behavior *p_behavior = NULL;
    while (behavior_node != NULL)
    {
        if (validateElement(behavior_node, "behavior", "id", NULL))
        {
            Behavior global_behavior;
            const char *id = (const char *)xmlGetProp(behavior_node, (const xmlChar *)"id");
            action = behavior_node->children;
            while (action != NULL)
            {
                getAction(action, global_behavior);
                action = action->next;
            }
            if (id)
            {
                std::string s_id = id;
                m_global_behaviors.insert(m_global_behaviors.begin(), std::make_pair(s_id, global_behavior));
            }
        }
        behavior_node = behavior_node->next;
    }
}

void SPiVparser::setPath(std::string _path)
{
    m_path = _path;
}
