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


#include "websocketsource.h"
#include <iostream>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <sstream>
#include <listplusplus.h>
#include <memory>
#include <timestamp.h>
#include "uuidhelper.h"

#include <QVariantMap>
#include <QJsonDocument>
#include <QStringList>

#include "debugout.h"
#include "common.h"
#include "superptr.hpp"

#define __SMALLFILE__ std::string(__FILE__).substr(std::string(__FILE__).rfind("/")+1)
libwebsocket_context *context = NULL;
WebSocketSource *source;
AbstractRoutingEngine *m_re;

double oldTimestamp=0;
double totalTime=0;
double numUpdates=0;
double averageLatency=0;

class UniquePropertyCache
{
public:
	bool hasProperty(std::string name, std::string source, Zone::Type zone)
	{
		for(auto i : mProperties)
		{
			if(i->name == name &&
					i->sourceUuid == source &&
					i->zone == zone)
			{
				return true;
			}
		}
		return false;
	}

	std::shared_ptr<AbstractPropertyType> append(std::string name, std::string source, Zone::Type zone, std::string type)
	{
		for(auto i : mProperties)
		{
			if(i->name == name &&
					i->sourceUuid == source &&
					i->zone == zone)
			{
				return i;
			}
		}

		auto t = VehicleProperty::getPropertyTypeForPropertyNameValue(name);

		if(!t)
		{
			VehicleProperty::registerProperty(name, [name, type]() -> AbstractPropertyType* {
				if(type == amb::BasicTypes::UInt16Str)
				{
					return new BasicPropertyType<uint16_t>(name, 0);
				}
				else if(type == amb::BasicTypes::Int16Str)
				{
					return new BasicPropertyType<int16_t>(name, 0);
				}
				else if(type == amb::BasicTypes::UInt32Str)
				{
					return new BasicPropertyType<uint32_t>(name, 0);
				}
				else if(type == amb::BasicTypes::Int32Str)
				{
					return new BasicPropertyType<int32_t>(name, 0);
				}
				else if(type == amb::BasicTypes::StringStr)
				{
					return new StringPropertyType(name);
				}
				else if(type == amb::BasicTypes::DoubleStr)
				{
					return new BasicPropertyType<double>(name, 0);
				}
				else if(type == amb::BasicTypes::BooleanStr)
				{
					return new BasicPropertyType<bool>(name, false);
				}
				DebugOut(DebugOut::Warning) << "Unknown or unsupported type: " << type << endl;
				return nullptr;
			});
			t = VehicleProperty::getPropertyTypeForPropertyNameValue(name);
		}

		if(t)/// check again to see if registration succeeded
		{
			t->sourceUuid = source;
			t->zone = zone;

			mProperties.emplace_back(t);
		}

		return property(name, source, zone); /// will return nullptr if t didn't register
	}

	std::shared_ptr<AbstractPropertyType> property(std::string name, std::string source, Zone::Type zone)
	{
		for(auto i : mProperties)
		{
			if(i->name == name &&
					i->sourceUuid == source &&
					i->zone == zone)
			{
				return i;
			}
		}

		return nullptr;
	}

	std::vector<std::shared_ptr<AbstractPropertyType>> properties() { return mProperties; }

private:
	std::vector<std::shared_ptr<AbstractPropertyType>> mProperties;
};

UniquePropertyCache properties;

static int callback_http_only(libwebsocket_context *context,struct libwebsocket *wsi,enum libwebsocket_callback_reasons reason,void *user, void *in, size_t len);
static struct libwebsocket_protocols protocols[] = {
{
	"http-only",
	callback_http_only,
	0,
	128,
},
{  /* end of list */
	NULL,
	NULL,
	0,
	0
}
};

//Called when a client connects, subscribes, or unsubscribes.
void WebSocketSource::checkSubscriptions()
{
	while (queuedRequests.size() > 0)
	{
		VehicleProperty::Property prop = queuedRequests.front();
		removeOne(&queuedRequests,prop);
		if (contains(activeRequests,prop))
		{
			return;
		}
		activeRequests.push_back(prop);

		QVariantMap reply;

		reply["type"] = "method";
		reply["name"] = "subscribe";
		reply["property"] = prop.c_str();
		reply["transactionid"] = "d293f670-f0b3-11e1-aff1-0800200c9a66";

		lwsWriteVariant(clientsocket, reply);
	}
}
void WebSocketSource::setConfiguration(map<string, string> config)
{
	//printf("WebSocketSource::setConfiguration has been called\n");
	std::string ip;
	int port;
	configuration = config;

	if(config.find("binaryProtocol") != config.end())
	{
		doBinary = config["binaryProtocol"] == "true";
	}

	for (map<string,string>::iterator i=configuration.begin();i!=configuration.end();i++)
	{
		DebugOut() << __SMALLFILE__ <<":"<< __LINE__ << "Incoming setting for WebSocketSource:" << (*i).first << ":" << (*i).second << "\n";
		//printf("Incoming setting: %s:%s\n",(*i).first.c_str(),(*i).second.c_str());
		if ((*i).first == "ip")
		{
			ip = (*i).second;
		}
		if ((*i).first == "port")
		{
			port = boost::lexical_cast<int>((*i).second);
		}
		if ((*i).first == "ssl")
		{
			if ((*i).second == "true")
			{
				m_sslEnabled = true;
			}
			else
			{
				m_sslEnabled = false;
			}
		}
	}
	//printf("Connecting to websocket server at %s port %i\n",ip.c_str(),port);
	DebugOut() << __SMALLFILE__ <<":"<< __LINE__ << "Connecting to websocket server at" << ip << ":" << port << "\n";
	int sslval = 0;
	if (m_sslEnabled)
	{
		DebugOut(5) << "SSL ENABLED" << endl;
		sslval = 2;
	}

	clientsocket = libwebsocket_client_connect(context, ip.c_str(), port, sslval,"/", "localhost", "websocket", protocols[0].name, -1);
}

PropertyInfo WebSocketSource::getPropertyInfo(const VehicleProperty::Property &property)
{
	Zone::ZoneList zones;
	for(auto i : properties.properties())
	{
		if(i->name == property)
		{
			zones.push_back(i->zone);
		}
	}

	return PropertyInfo(0, zones);
}

bool gioPollingFunc(GIOChannel *source, GIOCondition condition, gpointer data)
{
	//This is the polling function. If it return false, glib will stop polling this FD.

	oldTimestamp = amb::currentTime();

	struct pollfd pollstruct;
	int newfd = g_io_channel_unix_get_fd(source);
	pollstruct.fd = newfd;
	pollstruct.events = condition;
	pollstruct.revents = condition;
	libwebsocket_service_fd(context,&pollstruct);
	if (condition & G_IO_HUP)
	{
		//Hang up. Returning false closes out the GIOChannel.
		//printf("Callback on G_IO_HUP\n");
		return false;
	}
	if (condition & G_IO_IN)
	{

	}
	DebugOut() << "gioPollingFunc" << condition << endl;

	return true;
}

static int checkTimeouts(gpointer data)
{
	WebSocketSource *src = (WebSocketSource*)data;
	for (auto i=src->uuidTimeoutMap.begin();i!= src->uuidTimeoutMap.end();i++)
	{
		if (src->uuidRangedReplyMap.find((*i).first) != src->uuidRangedReplyMap.end())
		{
			//A source exists!
			if (amb::currentTime() > (*i).second)
			{
				//We've reached timeout
				DebugOut() << "Timeout reached for request ID:" << (*i).first << "\n";
				src->uuidRangedReplyMap[(*i).first]->success = false;
				src->uuidRangedReplyMap[(*i).first]->completed(src->uuidRangedReplyMap[(*i).first]);
				src->uuidRangedReplyMap.erase((*i).first);
				src->uuidTimeoutMap.erase((*i).first);
				i--;

				if (src->uuidTimeoutMap.size() == 0)
				{
					return 0;
				}

			}
			else
			{
				//No timeout yet, keep waiting.
			}
		}
		else
		{
			//Reply has already come back, ignore and erase from list.
			src->uuidTimeoutMap.erase((*i).first);
			i--;

			if (src->uuidTimeoutMap.size() == 0)
			{
				return 0;
			}
		}

	}
	return 0;
}

static int callback_http_only(libwebsocket_context *context, struct libwebsocket *wsi,enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 + LWS_SEND_BUFFER_POST_PADDING];
	DebugOut() << __SMALLFILE__ << ":" << __LINE__ << reason << "callback_http_only" << endl;
	switch (reason)
	{
		case LWS_CALLBACK_CLOSED:
			//fprintf(stderr, "mirror: LWS_CALLBACK_CLOSED\n");
			//wsi_mirror = NULL;
			//printf("Connection closed!\n");
			break;

			//case LWS_CALLBACK_PROTOCOL_INIT:
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
		{
			//This happens when a client initally connects. We need to request the support event types.
			source->clientConnected = true;
			source->checkSubscriptions();
			//printf("Incoming connection!\n");
			DebugOut() << __SMALLFILE__ <<":"<< __LINE__ << "Incoming connection" << endl;

			QVariantMap toSend;
			toSend["type"] = "method";
			toSend["name"] = "getSupported";
			toSend["transactionid"] = amb::createUuid().c_str();

			lwsWriteVariant(wsi, toSend);

			break;
		}
		case LWS_CALLBACK_CLIENT_RECEIVE:
		{
			QByteArray d((char*)in, len);

			WebSocketSource * manager = source;

			if(manager->expectedMessageFrames && manager->partialMessageIndex < manager->expectedMessageFrames)
			{
				manager->incompleteMessage += d;
				manager->partialMessageIndex++;
				break;
			}
			else if(manager->expectedMessageFrames && manager->partialMessageIndex == manager->expectedMessageFrames)
			{
				d = manager->incompleteMessage + d;
				manager->expectedMessageFrames = 0;
			}

			DebugOut(7) << "data received: " << d.data() << endl;

			int start = d.indexOf("{");

			if(manager->incompleteMessage.isEmpty() && start > 0)
			{
				DebugOut(7)<< "We have an incomplete message at the beginning.  Toss it away." << endl;
				d = d.right(start-1);
			}


			int end = d.lastIndexOf("}");

			if(end == -1)
			{
				manager->incompleteMessage += d;
				break;
			}

			QByteArray tryMessage = manager->incompleteMessage + d.left(end+1);

			DebugOut(6) << "Trying to parse message: " << tryMessage.data() << endl;

			QJsonDocument doc;

			QJsonParseError parseError;

			doc = QJsonDocument::fromJson(tryMessage, &parseError);

			if(doc.isNull())
			{
				DebugOut(7) << "Invalid or incomplete message" << endl;
				DebugOut(7) << parseError.errorString().toStdString() << ": " << parseError.offset << endl;
				manager->incompleteMessage += d;
				break;
			}

			manager->incompleteMessage = end == d.length()-1 ? "" : d.right(end);

			QVariantMap call = doc.toVariant().toMap();

			string type = call["type"].toString().toStdString();
			string name = call["name"].toString().toStdString();
			string id = call["transactionid"].toString().toStdString();

			if(type == "multiframe")
			{
				manager->expectedMessageFrames = call["frames"].toInt();
				manager->partialMessageIndex = 1;
				manager->incompleteMessage = "";

			}
			else if (type == "valuechanged")
			{
				QVariantMap data = call["data"].toMap();

				string value = data["value"].toString().toStdString();
				double timestamp = data["timestamp"].toDouble();
				int sequence = data["sequence"].toInt();
				Zone::Type zone = data["zone"].toInt();
				string type = data["type"].toString().toStdString();

				DebugOut() << __SMALLFILE__ <<":"<< __LINE__ << "Value changed:" << name << value << endl;

				try
				{
					auto property = properties.append(name, source->uuid(), zone, type);

					if(!property)
					{
						DebugOut(DebugOut::Warning) << "We either don't have this or don't support it ("
													<< name << "," << zone << "," << type << ")" << endl;
					}

					property->timestamp = timestamp;
					property->sequence = sequence;
					property->fromString(value);

					m_re->updateProperty(property.get(), source->uuid());

					double currenttime = amb::currentTime();

					/** This is now the latency between when something is available to read on the socket, until
					 *  a property is about to be updated in AMB.  This includes libwebsockets parsing and the
					 *  JSON parsing in this section.
					 */

					DebugOut()<<"websocket network + parse latency: "<<(currenttime - property->timestamp)*1000<<"ms"<<endl;
					totalTime += (currenttime - oldTimestamp)*1000;
					numUpdates ++;
					averageLatency = totalTime / numUpdates;

					DebugOut()<<"Average parse latency: "<<averageLatency<<endl;
				}
				catch (exception ex)
				{
					//printf("Exception %s\n",ex.what());
					DebugOut() << __SMALLFILE__ <<":"<< __LINE__ << "Exception:" << ex.what() << "\n";
				}
			}
			else if (type == "methodReply")
			{
				if (name == "getSupported" || name == "supportedChanged")
				{

					QVariant data = call["data"];

					QVariantList supported = data.toList();

					DebugOut() << __SMALLFILE__ <<":"<< __LINE__ << "Got getSupported request"<<endl;

					double serverTime = call["systemTime"].toDouble();

					DebugOut() << "Server time is: " << serverTime << endl;

					if(serverTime)
						source->serverTimeOffset = amb::Timestamp::instance()->epochTime() - serverTime;

					Q_FOREACH(QVariant p, supported)
					{
						QVariantMap d = p.toMap();
						Zone::Type zone = d["zone"].toInt();
						std::string name = d["property"].toString().toStdString();
						std::string proptype = d["type"].toString().toStdString();
						std::string source = d["source"].toString().toStdString();

						properties.append(name, source, zone, proptype);
					}

					source->updateSupported();

				}
				else if (name == "getRanged")
				{
					QVariantList data = call["data"].toList();

					std::list<AbstractPropertyType*> propertylist;

					Q_FOREACH(QVariant d, data)
					{
						QVariantMap obj = d.toMap();

						std::string name = obj["property"].toString().toStdString();
						std::string value = obj["value"].toString().toStdString();
						double timestamp = obj["timestamp"].toDouble() + source->serverTimeOffset;
						int sequence = obj["sequence"].toInt();

						AbstractPropertyType* type = VehicleProperty::getPropertyTypeForPropertyNameValue(name, value);
						if(!type)
						{
							DebugOut() << "TODO: support custom types here: " << endl;
							continue;
						}
						type->timestamp = timestamp;
						type->sequence = sequence;

						propertylist.push_back(type);
					}

					if (source->uuidRangedReplyMap.find(id) != source->uuidRangedReplyMap.end())
					{
						source->uuidRangedReplyMap[id]->values = propertylist;
						source->uuidRangedReplyMap[id]->success = true;
						source->uuidRangedReplyMap[id]->completed(source->uuidRangedReplyMap[id]);
						source->uuidRangedReplyMap.erase(id);
					}
					else
					{
						DebugOut() << "getRanged methodReply has been recieved, without a request being in!. This is likely due to a request coming in after the timeout has elapsed.\n";
					}
				}
				else if (name == "get")
				{

					DebugOut() << __SMALLFILE__ << ":" << __LINE__ << "Got \"GET\" reply" << endl;
					if (source->uuidReplyMap.find(id) != source->uuidReplyMap.end())
					{
						QVariantMap obj = call["data"].toMap();

						std::string property = obj["property"].toString().toStdString();
						std::string value = obj["value"].toString().toStdString();
						double timestamp = obj["timestamp"].toDouble();
						int sequence = obj["sequence"].toInt();
						Zone::Type zone = obj["zone"].toInt();

						auto v = amb::make_unique(VehicleProperty::getPropertyTypeForPropertyNameValue(property, value));

						v->timestamp = timestamp;
						v->sequence = sequence;
						v->zone = zone;

						if (source->uuidReplyMap.find(id) != source->uuidReplyMap.end() && source->uuidReplyMap[id]->error != AsyncPropertyReply::Timeout)
						{
							source->uuidReplyMap[id]->value = v.get();
							source->uuidReplyMap[id]->success = true;
							source->uuidReplyMap[id]->completed(source->uuidReplyMap[id]);
							source->uuidReplyMap.erase(id);

						}
						else
						{
							DebugOut() << "get methodReply has been recieved, without a request being in!. This is likely due to a request coming in after the timeout has elapsed.\n";
						}
					}
					else
					{
						DebugOut() << __SMALLFILE__ << ":" << __LINE__ << "GET Method Reply INVALID! Multiple properties detected, only single are supported!!!" << "\n";
					}


				}
				else if (name == "set")
				{
					DebugOut() << __SMALLFILE__ << ":" << __LINE__ << "Got \"SET\" event" << endl;
					std::string id = call["transactionid"].toString().toStdString();

					if(source->setReplyMap.find(id) != source->setReplyMap.end() && source->setReplyMap[id]->error != AsyncPropertyReply::Timeout)
					{
						AsyncPropertyReply* reply = source->setReplyMap[id];

						reply->success = call["success"].toBool();
						reply->error = AsyncPropertyReply::strToError(call["error"].toString().toStdString());

						QVariantMap obj = call["data"].toMap();

						std::string property = obj["property"].toString().toStdString();
						std::string value = obj["value"].toString().toStdString();

						double timestamp = obj["timestamp"].toDouble();
						int sequence = obj["sequence"].toInt();
						Zone::Type zone = obj["zone"].toInt();

						auto v = amb::make_unique(VehicleProperty::getPropertyTypeForPropertyNameValue(property, value));

						if(v)
						{
							v->timestamp = timestamp;
							v->sequence = sequence;
							v->zone = zone;
						}
						else
						{
							throw std::runtime_error("property may not be registered.");
						}

						reply->value = v.get();
						reply->completed(reply);
						source->setReplyMap.erase(id);
					}
				}
				else
				{
					DebugOut(DebugOut::Warning) << "Unhandled methodReply: " << name << endl;
				}

			}

			break;

		}
		case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
		{
			//printf("Requested extension: %s\n",(char*)in);
			return 0;
			break;
		}
		case LWS_CALLBACK_ADD_POLL_FD:
		{
			DebugOut(5) << __SMALLFILE__ << ":" << __LINE__ << "Adding poll for websocket IO channel" << endl;
			//Add a FD to the poll list.
			GIOChannel *chan = g_io_channel_unix_new(libwebsocket_get_socket_fd(wsi));

			/// TODO: I changed this to be more consistent with the websocket sink end. it may not be correct. TEST

			g_io_add_watch(chan,GIOCondition(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP),(GIOFunc)gioPollingFunc,0);
			g_io_channel_set_close_on_unref(chan,true);
			g_io_channel_unref(chan); //Pass ownership of the GIOChannel to the watch.

			break;
		}
			return 0;
	}
}
void WebSocketSource::updateSupported()
{
	PropertyList list;
	for(auto i : properties.properties())
	{
		if(!contains(list, i->name))
			list.push_back(i->name);
	}

	m_re->updateSupported(list, PropertyList(), this);
}

WebSocketSource::WebSocketSource(AbstractRoutingEngine *re, map<string, string> config) : AbstractSource(re, config), partialMessageIndex(0),expectedMessageFrames(0),
	serverTimeOffset(0)
{
	m_sslEnabled = false;
	clientConnected = false;
	source = this;
	m_re = re;
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof info);
	info.protocols = protocols;
	info.extensions = nullptr;

	if(config.find("useExtensions") != config.end() && config["useExtensions"] == "true")
	{
		info.extensions = libwebsocket_get_internal_extensions();
	}

	info.gid = -1;
	info.uid = -1;
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.user = this;

	context = libwebsocket_create_context(&info);

	setConfiguration(config);

	//printf("websocketsource loaded!!!\n");
	g_timeout_add(1000,checkTimeouts,this); //Do this once per second, check for functions that have timed out and reply with success = false;

}
PropertyList WebSocketSource::supported()
{
	PropertyList list;
	for(auto i : properties.properties())
	{
		list.push_back(i->name);
	}
	return list;
}

int WebSocketSource::supportedOperations()
{
	/// TODO: need to do this correctly based on what the host supports.
	return Get | Set | GetRanged;
}

const string WebSocketSource::uuid()
{
	return "d293f670-f0b3-11e1-aff1-0800200c9a66";
}

void WebSocketSource::subscribeToPropertyChanges(VehicleProperty::Property property)
{
	//printf("Subscribed to property: %s\n",property.c_str());
	queuedRequests.push_back(property);
	if (clientConnected)
	{
		checkSubscriptions();
	}
}


void WebSocketSource::unsubscribeToPropertyChanges(VehicleProperty::Property property)
{
	removeRequests.push_back(property);
	if (clientConnected)
	{
		checkSubscriptions();
	}
}


void WebSocketSource::getPropertyAsync(AsyncPropertyReply *reply)
{
	std::string uuid = amb::createUuid();
	uuidReplyMap[uuid] = reply;
	uuidTimeoutMap[uuid] = amb::currentTime() + 10.0; ///TODO: 10 second timeout, make this configurable?

	QVariantMap data;
	data["property"] = reply->property.c_str();
	data["zone"] = reply->zoneFilter;

	QVariantMap replyvar;
	replyvar["type"] = "method";
	replyvar["name"] = "get";
	replyvar["data"] = data;
	replyvar["transactionid"] = uuid.c_str();

	lwsWriteVariant(clientsocket, replyvar);
}

void WebSocketSource::getRangePropertyAsync(AsyncRangePropertyReply *reply)
{
	std::string uuid = amb::createUuid();
	uuidRangedReplyMap[uuid] = reply;
	uuidTimeoutMap[uuid] = amb::currentTime() + 60; ///TODO: 60 second timeout, make this configurable?
	stringstream s;
	s.precision(15);
	s << "{\"type\":\"method\",\"name\":\"getRanged\",\"data\": {";

	QVariantMap replyvar;
	replyvar["type"] = "method";
	replyvar["name"] = "getRanged";
	replyvar["transactionid"] = uuid.c_str();
	replyvar["timeBegin"] = reply->timeBegin - serverTimeOffset;
	replyvar["timeEnd"] = reply->timeEnd - serverTimeOffset;
	replyvar["sequenceBegin"] = reply->sequenceBegin;
	replyvar["sequenceEnd"] = reply->sequenceEnd;

	QStringList properties;

	for (auto itr = reply->properties.begin(); itr != reply->properties.end(); itr++)
	{
		VehicleProperty::Property p = *itr;
		properties.append(p.c_str());
	}

	replyvar["data"] = properties;

	lwsWriteVariant(clientsocket, replyvar);
}

AsyncPropertyReply * WebSocketSource::setProperty( AsyncSetPropertyRequest request )
{
	AsyncPropertyReply* reply = new AsyncPropertyReply(request);

	std::string uuid = amb::createUuid();

	QVariantMap data;
	data["property"] = request.property.c_str();
	data["value"] = request.value->toString().c_str();
	data["zone"] = request.zoneFilter;

	QVariantMap replyvar;
	replyvar["type"] = "method";
	replyvar["name"] = "set";
	replyvar["data"] = data;
	replyvar["transactionid"] = uuid.c_str();

	lwsWriteVariant(clientsocket, replyvar);

	setReplyMap[uuid] = reply;

	return reply;
}

extern "C" AbstractSource * create(AbstractRoutingEngine* routingengine, map<string, string> config)
{
	return new WebSocketSource(routingengine, config);

}
