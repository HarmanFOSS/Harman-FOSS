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
#include "Subject.h"

namespace FPV{
  
  void Subject::attach(Behavior* _behavior){
    m_list.insert(m_list.begin(),_behavior);
  }

  void Subject::notify(EventType _mod){
    if(!m_enable) return;
    std::list<Behavior*>::iterator behavior = m_list.begin();
    while(behavior!=m_list.end()){
      (*behavior)->notify(_mod);
      behavior++;
    }
  }

  Subject::Subject(){
    m_enable=false;
    m_catch=false;
  }
  
}
