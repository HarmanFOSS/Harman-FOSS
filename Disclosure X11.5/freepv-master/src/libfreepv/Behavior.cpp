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

#include "Behavior.h"
#include <assert.h>
#include <iostream>

namespace FPV{

    void Behavior::add(Action *_action, EventType _mod){
	if(_mod==FPV_UNKNOWN) return;
	std::map<EventType,actionList>::iterator iter=m_map.find(_mod);
	if(iter!=m_map.end()){
	    //add an action to the list
	    iter->second.insert(iter->second.begin(),_action);
	}else{
	    //create new action list
	    actionList aux_list;
	    aux_list.insert(aux_list.begin(),_action);
	    //insert it in the map with the type of event as key
	    m_map.insert(m_map.begin(), std::make_pair(_mod,aux_list));
	}
    }
  
    void Behavior::notify(EventType _mod){
	std::map<EventType,actionList>::iterator iter=m_map.find(_mod);
	if(iter==m_map.end()) return;
	actionList::iterator action= iter->second.begin();
	//Each action is executed
	while(action!=iter->second.end()){
	    (*action)->execute();
	    action++;
	}
    }

    Behavior::~Behavior(){
    }
    
}//namespace
