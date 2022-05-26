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

#include "EventProcessor.h"

namespace FPV{

    EventProcessor* EventProcessor::Instance(){
	static EventProcessor Processor;
	return &Processor;
    }

    void EventProcessor::Reseat(){
	m_current_msEvent=FPV_UNKNOWN;
	m_last_msEvent=FPV_UNKNOWN;
	m_current_subject=NULL;
	m_last_subject=NULL;
	m_scene=m_viewer->getScene();
    }
  
    void EventProcessor::processMouseEvent(const MouseEvent & event){
	//The memeber variables need to be reseated, since
	//when the scene is changed the last subject could not exist.
	if(m_scene!=m_viewer->getScene()) Reseat();
	//The current becomes the last...
	m_last_subject=m_current_subject;
	m_last_msEvent=m_current_msEvent;
	//Get current subject by rendering in the backbuffer...
	m_current_subject=getPointedSubject(event);
	
	//if the last mouse event was a press type event
	if(m_last_msEvent==FPV_PRESS){
	    //then it could have been released.
	    if(!event.down){
		//in that case the current element is released
		if(m_current_subject)   m_current_subject->notify(FPV_RELEASE);
		m_current_msEvent=FPV_RELEASE;
		if(m_scene!=m_viewer->getScene()) Reseat();
	    } 
	}else if((m_last_subject!=m_current_subject)){

	    //if the last subject is not the same that the current
	    //the we most have been entered one and leaved the other.
	    //the last subject is leaved and the current is entered.

	    if(m_last_subject)   m_last_subject->notify(FPV_LEAVE);
	    if(m_scene!=m_viewer->getScene()) Reseat();
	    if(m_current_subject)   m_current_subject->notify(FPV_ENTER);
	    m_current_msEvent=FPV_ENTER;
	    if(m_scene!=m_viewer->getScene()) Reseat();
	}else if(event.down){

	    //if the mouse button bit is one then the current subject
	    //has been pressed.

	    if(m_current_subject)   m_current_subject->notify(FPV_PRESS);
	    m_current_msEvent=FPV_PRESS;
	    if(m_scene!=m_viewer->getScene()) Reseat();
	} 
	
    }

    EventProcessor::EventProcessor(){
	m_viewer = PanoViewer::Instance();
	m_scene = m_viewer->getScene();
	EventType m_current_msEvent=FPV_UNKNOWN;
	EventType m_last_msEvent=FPV_UNKNOWN;
	Subject* m_current_subject=NULL;
	Subject* m_last_subject=NULL;
    }
  
    Subject* EventProcessor::getPointedSubject(const MouseEvent &event){
	std::list<Subject*>* aux;
	Renderer* renderer = m_viewer->getRenderer();
	aux = renderer->getPointedSubjects(*m_scene, event.pos);
	std::list<Subject*>::iterator iter=aux->begin();
	while(iter!=aux->end()){
	    Subject* sub=(*iter);
	    if(sub->isEnable())
		return sub;
	    if(sub->isCatching())
		iter++;
	    else break;
	}
	return NULL;
    }

}//namespace
