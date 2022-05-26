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

#ifndef FPV_NodeElement_H
#define FPV_NodeElement_H

#include "Utils/stringID.h"
#include "SceneElement.h"

namespace FPV
{

    /*! The NodeElement template class is a container for the elements that are in a
      Scene, this NodeElement class has different methods to add siblings nodes
      and childeren nodes, the methods use the depth value to determine the position
      of the node in the tree, since the renderer needs to render first the elements
      that are farther to the camera and then the elements that are closer to the camera.*/
    template <class Element>
	class NodeElement
	{
	public:
	    /*!This class is used to iterate the tree, the Scene's Tree*/
	    class Iterator{
	    private:
		NodeElement* m_root;
		//!Current node.
		NodeElement* node;
		//!Constructor
		Iterator(NodeElement* root){node=m_root=root;}
		/*!The NodeElement class is the only one that can create
		an iterator*/
		friend class NodeElement;
		
	    public:
		//!This method moves the pointer to the next node in the tree.
		void operator++(){
			//This means we need to restart the iterator
		    if(node==NULL){
			node=m_root;
			return;
		    }

		    //std::cerr<<"Current node is: "<<node->m_id<<std::endl;
		    //std::cerr<<"node reference: "<<node<<std::endl;
		    //std::cerr<<"We get the parent node reference: "<<node->getParent()<<std::endl;
		    //Move to the children
		    if(node->getChildren()!=NULL){
			//std::cerr<<"We move to the children node"<<std::endl;
			node=node->getChildren();
		    }
		    //Move to the sibling
		    else if(node->getSibling()!=NULL){
			//std::cerr<<"We move to the sibling node"<<std::endl;
			node=node->getSibling();
		    }else{
			//In other case we return to the parent
			node=node->getParent();
			//To see if it has a sibling
			while(node!=NULL){
			    //std::cerr<<"We move to the parent "<<node->m_id<<std::endl;
			    //the iterator can not go further than the 
			    //node provided as root.
			    if(node==m_root->getParent()){
				//std::cerr<<"We can't go up any more "<<std::endl;
				node=NULL;
				//check if the parent node has a sibling
			    }else if(node->getSibling()!=NULL){
				node=node->getSibling();
				//std::cerr<<"We move to the sibling node"<<std::endl;
				break;
			    }
			    //we keep moving up
			    node=node->getParent();
			}
		    }
		}
       
		//!This method moves the pointer to the element back to the root.
		void operator--(){
			//This means we need to restart the iterator
		    if(node==NULL){
			node=m_root;
			return;
		    }
		    
		    //The iterator can not go further
		    if(node==m_root){
			    node=NULL;
			    return;
		    }

		    //move to the parent node
		    node=node->getParent();
		    
		}

		//!This method tries to find a node by it's id.
		Element* operator[](std::string id){
		    
		    NodeElement* aux;
		    aux=m_root;
		    
		    while(aux!=NULL&&aux->m_id!=id){
			//Move to the children
			if(aux->getChildren()!=NULL){
			    aux=aux->getChildren();
			}
			//Move to the sibling
			else if(aux->getSibling()!=NULL)
			    aux=aux->getSibling();
			else{
			    //I other case we return to the parent
			    aux=aux->getParent();
			    //To see if it has a sibling
			    while(aux!=NULL){
				//the iterator can not go further than the 
				//node provided as root.
				if(aux==m_root->getParent()){
				    aux=NULL;
				    //check if the parent node has a sibling
				}else if(aux->getSibling()!=NULL){
				    aux=aux->getSibling();
				    break;
				}
				//we keep moving up
				aux=aux->getParent();
			    }
			}
		    }
		    return aux;
		}

		Iterator& operator++(int){operator++(); return *this;}
		Iterator& operator--(int){operator--(); return *this;}

		/*!\return Pointed element*/
		Element* element(){
		    if(node){
			return node->getElement();
		    }
		    return NULL;
		}
       
		/*!\return Node's id*/
		std::string id(){
		    if(node)
			return node->m_id;
		    return "";
		}

		/*!\return Depth of the current node*/
		float depth(){
		    if(node)
			return node->getDepth();
		    return 0;
		}
		
		/*!This function is used to remplace the element
		  containde in the node*/
		void swap(Element *_element){
			node->setElement(_element);
		}
	    };

	    //!Constructor
	    /*!\param contained element.
	      \param id of the container.
	      \param depth default value is 9.*/
	    NodeElement(Element* _element, const char* id=NULL, float depth=9.0f);
	    ~NodeElement();
	    
	    //!\return Pointer to childre'n node
	    NodeElement* getChildren();
	    //!\return Pointer to sibling's node
	    NodeElement* getSibling();
	    //!\return Pointer to parent's node
	    NodeElement* getParent();
	    //!\return Pointer to containde element.
	    Element* getElement();

	    void setChildren(NodeElement *children);
	    void setParent(NodeElement *parent);
	    void setSibling(NodeElement *sibling);
	    void setChildren(Element *children, const char* id=NULL, float depth=9.0f);
	    void setParent(Element *parent, const char* id=NULL);
	    void setSibling(Element *sibling, const char* id=NULL, float depth=9.0f);
	    void setElement(Element *_element, const char* id=NULL);
	    Iterator getIterator(){
		Iterator iter(this);
		return iter;
	    }

	    //!set the depth of the elment.
	    void setDepth(float depth);
	    //!\return the depth of the elment.
	    float getDepth();

	    //!The id to identify the element.
	    std::string m_id;

	private:
	    
	    //!This is the order in wich the elment are renderer.
	    float m_depth;
	    
	    //!The element inside the node
	    Element* m_element;
	    //!Sibling of the node, a group or elment.
	    NodeElement* m_sibling;
	    //!Parent of the node, a group node.
	    NodeElement* m_parent;
	    //!Children of the node, an elment.
	    NodeElement* m_children;

	};

    template <class Element> NodeElement<Element>::NodeElement(Element* _element, const char* id, float depth){
	m_depth=depth;
	m_element=_element;
	m_children = NULL;
	m_sibling = NULL;
	m_parent = NULL;
	if(id) m_id=id;
	else m_id = Utils::stringID::generate();
	std::cerr<<"ID: "<<m_id<<std::endl;
    }

    template <class Element> NodeElement<Element>::~NodeElement(){
	    while(m_children!=NULL){
		    delete(m_children);
	    }
	    if(m_parent!=NULL)
		m_parent->setChildren(m_sibling);
	    delete(m_element);
    }

    template <class Element> NodeElement<Element>* NodeElement<Element>::getChildren(){
	return m_children;
    }
    
    template <class Element> NodeElement<Element>* NodeElement<Element>::getSibling(){
	return m_sibling;
    }

    template <class Element> NodeElement<Element>* NodeElement<Element>::getParent(){
	return m_parent;
    }

    template <class Element> Element* NodeElement<Element>::getElement(){
	return m_element;
    }

    template <class Element> void NodeElement<Element>::setParent(NodeElement<Element>* parent){
	m_parent=parent;
	if(m_sibling!=NULL)
	    m_sibling->setParent(m_parent);
    }

    template <class Element> void NodeElement<Element>::setParent(Element* parent, const char* id){
	setParent(new NodeElement<Element>(parent, id));
    }

    template <class Element> void NodeElement<Element>::setSibling(NodeElement<Element>* sibling){
	if(sibling==NULL){
	    m_sibling=NULL;
	    return;
	}
	sibling->setParent(m_parent);
	if(m_sibling!=NULL){
	    if(m_sibling->getDepth()<sibling->getDepth()){
		sibling->setSibling(m_sibling);       
		m_sibling=sibling;
	    }else{      
		m_sibling->setSibling(sibling);
	    }
	}else{   
	    m_sibling=sibling;
	}
    }

    template <class Element> void NodeElement<Element>::setSibling(Element* sibling, const char* id, float depth){
	setSibling(new NodeElement<Element>(sibling, id, depth));
    }

    template <class Element> void NodeElement<Element>::setElement(Element* _element, const char* id){
	if(m_element)
	    delete(m_element);
	m_element=_element;
    }

    template <class Element> void NodeElement<Element>::setChildren(NodeElement<Element>* children){
	if(children==NULL){
	    m_children=NULL;
	    return;
	}
	children->setParent(this);
	if(m_children!=NULL){
	    if(m_children->getDepth()<children->getDepth()){
		children->setSibling(m_children);
		m_children=children;
	    }else{
		m_children->setSibling(children);
	    }  
	}else{
	    m_children=children;
	}
    }

    template <class Element> void NodeElement<Element>::setChildren(Element* children, const char* id, float depth){
	setChildren(new NodeElement<Element>(children, id, depth));
    }

    template <class Element> float NodeElement<Element>::getDepth(){
	return m_depth;
    }

    template <class Element> void NodeElement<Element>::setDepth(float depth){
	m_depth=depth;
    }

    typedef NodeElement<SceneElement> SceneElementNode;

    /*!This function gets the rotation of an SceneElementNode*/
    quaternion getRotation( SceneElementNode::Iterator iter);

}//namespace

#endif
