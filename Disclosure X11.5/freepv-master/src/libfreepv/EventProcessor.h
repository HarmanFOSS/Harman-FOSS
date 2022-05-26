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

#ifndef FPV_EventProcessor_H
#define FPV_EventProcessor_H

#include "PanoViewer.h"
#include "Subject.h"

namespace FPV{

    /*!This class is a singlenton and is in charge of determine which
      element is being pointed and to notify it with the correct type
      of event*/
    class EventProcessor{
    protected:
	//!Constructor
	EventProcessor();
	//!This method returns the element that is under the mouse
	Subject* getPointedSubject(const MouseEvent &event);
	//!Reset the member variable to it's original values
	void Reseat();

	PanoViewer* m_viewer;
	EventType m_current_msEvent;
	EventType m_last_msEvent;
	Subject* m_current_subject;
	Subject* m_last_subject;
	Scene* m_scene;
    
    public:
	/*!\return Pointer to the processor instance*/
	static EventProcessor* Instance();
	
	/*! called when a mouse event happend */
	void processMouseEvent(const MouseEvent & event);

	/*! called when a key event has happend */
	void processKeyEvent(const KeyEvent & event) {};
    };

}//namespace

#endif
