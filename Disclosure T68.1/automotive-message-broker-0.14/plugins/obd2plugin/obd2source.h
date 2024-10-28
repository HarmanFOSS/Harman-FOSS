/*
Copyright (C) 2012 Intel Corporation

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


#ifndef OBD2SOURCE_H
#define OBD2SOURCE_H



#include <abstractsource.h>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include "obdlib.h"
#include <glib.h>

#include "obdpid.h"

class ObdRequest
{
public:
  VehicleProperty::Property property;
  std::string req;
  std::string arg;
};


class CommandRequest
{
public:
  std::string req;
  std::vector<std::string> arglist;
};
class StatusMessage
{
public:
  std::string statusStr;
  VehicleProperty::Property property;
};
class ObdReply
{
public:
  VehicleProperty::Property property;
  std::string req;
  std::string reply;
};



class Obd2Amb
{
public:

	typedef function<std::string (std::string)> ConversionFunction;

	typedef std::vector<unsigned char> ByteArray;
	Obd2Amb()
	{
		supportedPidsList.push_back(new VehicleSpeedPid());
		supportedPidsList.push_back(new EngineSpeedPid());
		supportedPidsList.push_back(new MassAirFlowPid());
		supportedPidsList.push_back(new VinPid());
		supportedPidsList.push_back(new WmiPid());
		supportedPidsList.push_back(new FuelConsumptionPid());
		supportedPidsList.push_back(new EngineCoolantPid());
		supportedPidsList.push_back(new AirIntakeTemperaturePid());
		supportedPidsList.push_back(new EngineLoadPid());
		supportedPidsList.push_back(new ThrottlePositionPid());
		supportedPidsList.push_back(new BatteryVoltagePid());
	}

	~Obd2Amb()
	{
		for(auto itr = supportedPidsList.begin(); itr != supportedPidsList.end(); itr++)
		{
			delete *itr;
		}
	}

	ObdPid* createPidFromReply(ByteArray replyVector)
	{
		ByteArray replyVectorClean = ObdPid::cleanup(replyVector);
		ByteArray replyVectorCompressed = ObdPid::compress(replyVectorClean);

		for(auto itr = supportedPidsList.begin(); itr != supportedPidsList.end(); itr++)
		{
			ObdPid* p = *itr;

			if(p->needsCompress())
			{
					replyVector = replyVectorCompressed;
			}
			else replyVector = replyVectorClean;

			if (!p->tryParse(replyVector))
			{
				continue;
			}

			ObdPid* pid = (*itr)->create();
			return pid;
		}
		return 0;
	}
	ObdPid* createPidforProperty(VehicleProperty::Property property)
	{
		for(auto itr = supportedPidsList.begin(); itr != supportedPidsList.end(); itr++)
		{
			VehicleProperty::Property p = (*itr)->property;
			if(p == property)
			{
				ObdPid* obj = *itr;
				return obj->create();
			}
		}
		return NULL;
	}

	std::list<ObdPid*> supportedPidsList;
};

class OBD2Source : public AbstractSource
{

public:
	OBD2Source(AbstractRoutingEngine* re, map<string, string> config);
	~OBD2Source();
	const string uuid();
	int portHandle;
	void getPropertyAsync(AsyncPropertyReply *reply);
	void getRangePropertyAsync(AsyncRangePropertyReply *reply){}
	AsyncPropertyReply * setProperty(AsyncSetPropertyRequest request);
	void subscribeToPropertyChanges(VehicleProperty::Property property);
	void unsubscribeToPropertyChanges(VehicleProperty::Property property);
	PropertyList supported();

	PropertyInfo getPropertyInfo(const VehicleProperty::Property &property);

	int supportedOperations();

	PropertyList queuedRequests;
	bool clientConnected;
	PropertyList activeRequests;
	void engineSpeed(double speed);
	void vehicleSpeed(int speed);
	void mafValue(double maf);
	void engineCoolantTemp(int temp);
	PropertyList removeRequests;
	void setSupported(PropertyList list);

	void supportedChanged(const PropertyList &) {}
	GAsyncQueue* commandQueue;
	GAsyncQueue* statusQueue;
	GAsyncQueue* subscriptionAddQueue;
	GAsyncQueue* subscriptionRemoveQueue;
	GAsyncQueue* singleShotQueue;
	GAsyncQueue* responseQueue;
	std::list<std::string> m_blacklistPidList;
	std::map<std::string,int> m_blacklistPidCountMap;
	void setConfiguration(map<string, string> config);
	//void randomizeProperties();
	std::string m_port;
	std::string m_baud;
	bool m_isBluetooth;
	std::string m_btDeviceAddress;
	std::string m_btAdapterAddress;
	std::vector<AsyncPropertyReply*> propertyReplyList;
	void updateProperty(AbstractPropertyType *value);
	obdLib * obd;
	bool m_threadLive;
	GThread *m_gThread;

	typedef BasicPropertyType<bool> Obd2ConnectType;
	Obd2ConnectType obd2Connected;

private:
	PropertyList m_supportedProperties;
	std::map<VehicleProperty::Property, AbstractPropertyType*> oldValueMap;
	GMutex *threadQueueMutex;


};

#endif // OBD2SOURCE_H
