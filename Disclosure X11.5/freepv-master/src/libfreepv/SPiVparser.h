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

#ifndef FPV_SPIV_H
#define FPV_SPIV_H

#include "XMLparser.h"
#include "Scene.h"
#include "Image.h"
#include <map>
#include <string>
#include "Action.h"

namespace FPV
{

    /*!This structure is used
      to store info that is not
      inside the image class and
      that is going to be used at some
      point.
    */
    typedef struct
    {
	//!Pointer to the associated image.
	Image* img;
	//!External url to the image file.
	std::string url;
    } img_info;
    
    //! Image list where the key is the id in the SPi-V file.
    typedef std::map<std::string,img_info> imageList;
    //! Behavior list where the key is the id in the SPi-V file.
    typedef std::map<std::string,Behavior> behaviorList;

    /*!SPi-V parser is a class to parse SPi-V files, it has
      several privates methods that are in charge of creating
      the scene tree and creating a list of images to load and
      behaviors to attach.*/
    class SPiVparser: public XMLparser{
    private:
	//!Root Node of the SPi-V document.
	xmlNodePtr m_root;
	//!Global Node of the SPi-V document.
	xmlNodePtr m_global;
	//!Scene Node of the current scene.
	xmlNodePtr m_current_scene;
	/*!Pointer to the camera specified in th
	  Global Node*/
	Camera *m_global_camera;
	//!List to store the images to be downloaded
	imageList imgList;
	//!List of behaviors that where found in the global node.
	behaviorList m_global_behaviors;
	//!Pointer to the scene that is being loaded.
	Scene *m_scene;
	
	std::string m_path;
	
    public:
	
	//!Constructor
	SPiVparser();
	//!Destructor
	~SPiVparser();

	//!set the path to download
	void setPath(std::string _path);
  
	/*!Try to load a Tour(root element)
	  after calling this function
	  an internal tree of the document is created
	  \return true in case of finding a tour
	  \return false in other case.
	*/
	bool loadTour();

	/*!Try to load an Scene, if the id is NULL, then
	  we look for the one specified in the global's meta node
	  if it's not specified we load the first one in the tour.
	*/
	bool loadScene(Scene &sceneToLoad, const char* id=NULL);
  
	//!This method receives xml to parse.
	void parseNodeURL(const char* url);

	/*!In some cases the specification of some nodes would be
	  made in external xml documents, then the library needs to
	  find this files to be downloade, in order to complet the
	  document tree.
	  This method returns the xml urls needed to be downloaded.
	*/
	const char * getXMLToDownload();

        /*!This method returns the url that is in the front of the
	  list of images to be downloaded*/
	const char * getImgURLToDownload();

	/*!This method extract the pointer to the image that is in
	  the front of the list of images to be donwloaded.
	  \returns Pointer to an Image.
	*/
	Image * extractImage();
	
    private:
	
	/*!This function tries to load the meta data from the scene
	 in order to setup the camera*/ 
	bool setupCamera(xmlNodePtr node, Camera* sceneCamera);
	
	/*!A method that looks for the global node and tries to load
	  the different information that could be found in it.*/
	void loadGlobal();
	
	/*!This method is in charge of creating the nodes of the 
	  Scene's tree, create the SceneElements and attach their
	  behaviors.
	  \note This method is recursive to build the Scene's tree.
	  \param This is a pointer to the scene node.
	  \return pointer to the Scene's root node or pointer to group.
	*/
	SceneElementNode * setupScene(xmlNodePtr node);
	
	/*!This function gets the attributes like tilt, pan and roll
	  and set the orientation of the element.
	  \param This is a pointer to the element xml node.
	  \param This is a pointer to the SceneElement to be rotated. 
	*/
	void getElementRotation(xmlNodePtr node, SceneElement * element);
	
	/*!This method recives the pointer to the image from a SceneElement
	  and the xml node to load, it looks for an image inside the element
	  if not, it looks for it inside the list, in case it is allready
	  referenced by another element, in other case it looks for it
	  in all the XML document's tree by using the image reference.
	*/
	void getElementImage(xmlNodePtr node, Image * &img);

	/*!This method loads the Behaviors that are specified inside the global node
	 \todo add support to object attribute*/
	void loadGlobalBehaviors(xmlNodePtr _node);

	/*!This method loads the Behaviors that are specified inside the scene node
	 \todo add support to object attribute*/
	void loadSceneBehaviors(xmlNodePtr _node, Scene &_scene);

	/*!This method loads the Behaviors that are specified inside the panoelement node
	 \todo add support to object attribute*/
	void loadInlineBehaviors(xmlNodePtr _node, Subject *_sub);

	/*!This method loads the layers from an image
	 \todo add support to multiple layers, at the moment it just supports one*/
	void getElementImageLayers(xmlNodePtr node, Image * &img);

	/*!This method loads the actions for a Behavior*/
	void getAction(xmlNodePtr action, Behavior &behavior);

	/*!This method converts the string of the event attribute value to an EventType value*/
	EventType getEventType(const unsigned char*);
    };

}//name space 
  
#endif

  
