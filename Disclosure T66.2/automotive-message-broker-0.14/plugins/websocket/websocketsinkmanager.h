/*
	Copyright (C) 2012  Intel Corporation

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef WEBSOCKETSINKMANAGER_H
#define WEBSOCKETSINKMANAGER_H

#include <abstractroutingengine.h>
#include <abstractsink.h>
#include "websocketsink.h"
#include <gio/gio.h>
#include <map>
#include <libwebsockets.h>
#include "debugout.h"
#include <stdexcept>
#include "sys/types.h"
#include <stdlib.h>
#include <QByteArray>

class WebSocketSinkManager
{
public:
	WebSocketSinkManager(AbstractRoutingEngine* engine, map<string, string> config);
	void addSingleShotSink(libwebsocket* socket, VehicleProperty::Property property, Zone::Type zone, string id);
	void addSingleShotRangedSink(libwebsocket* socket, PropertyList properties,double start, double end, double seqstart,double seqend, string id);
	void addSink(libwebsocket* socket, VehicleProperty::Property property, string uuid);
	void disconnectAll(libwebsocket* socket);
	void removeSink(libwebsocket* socket, VehicleProperty::Property property, string uuid);
	void addPoll(int fd);
	void removePoll(int fd);
	void init();
	std::map<std::string, list<WebSocketSink*> > m_sinkMap;
	void setConfiguration(map<string, string> config);
	void setValue(libwebsocket* socket,VehicleProperty::Property property, string value, Zone::Type zone, string uuid);
	PropertyList getSupportedProperties();

	AbstractRoutingEngine * router() { return routingEngine; }

	int partialMessageIndex;
	QByteArray incompleteMessage;
	int expectedMessageFrames;

private:
	std::map<int, GIOChannel*> m_ioChannelMap;
	std::map<int, guint> m_ioSourceMap;
	AbstractRoutingEngine *m_engine;
	struct libwebsocket_protocols protocollist[2];

	AbstractRoutingEngine * routingEngine;
	std::map<std::string, std::string> configuration;

};

#endif // WEBSOCKETSINKMANAGER_H
