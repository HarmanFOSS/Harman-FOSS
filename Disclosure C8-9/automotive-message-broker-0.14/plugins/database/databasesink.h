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


#ifndef DATABASESINK_H
#define DATABASESINK_H

#include "abstractsink.h"
#include "abstractsource.h"
#include "basedb.hpp"
#include <asyncqueue.hpp>
#include "listplusplus.h"
#include "ambpluginimpl.h"

#include <glib.h>

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

const std::string DatabaseLogging = "DatabaseLogging";
const std::string DatabasePlayback = "DatabasePlayback";
const std::string DatabaseFile = "DatabaseFile";

class DBObject {
public:
	DBObject(): zone(0), time(0), sequence(0), quit(false) {}
	std::string key;
	std::string value;
	std::string source;
	int32_t zone;
	double time;
	int32_t sequence;
	std::string tripId;

	bool quit;

	bool operator == (const DBObject & other) const
	{
		return (key == other.key && source == other.source && zone == other.zone &&
				value == other.value && sequence == other.sequence && time == other.time);
	}

	bool operator != (const DBObject & other)
	{
		return (*this == other) == false;
	}
};

namespace amb
{

struct DBObjectCompare
{
	bool operator()(DBObject const & lhs, DBObject & rhs) const
	{
		if (lhs == rhs)
		{
			return true;
		}

		return false;
	}

};

}

namespace std {
  template <> struct hash<DBObject>
  {
	size_t operator()(const DBObject & x) const
	{
	  return x.key.length() * x.value.length() + x.time;
	}
  };
}

class Shared
{
public:
	Shared()
		:queue(true, true)
	{
		db = new BaseDB;
	}
	~Shared()
	{
		delete db;
	}

	BaseDB * db;
	amb::Queue<DBObject, amb::DBObjectCompare> queue;
	std::string tripId;
};

class PlaybackShared
{
public:
	PlaybackShared(AbstractRoutingEngine* re, std::string u, uint playbackMult)
		:routingEngine(re),uuid(u),playBackMultiplier(playbackMult),stop(false) {}
	~PlaybackShared()
	{
		for(auto itr = playbackQueue.begin(); itr != playbackQueue.end(); itr++)
		{
			DBObject obj = *itr;
		}

		playbackQueue.clear();
	}

	AbstractRoutingEngine* routingEngine;
	std::list<DBObject> playbackQueue;
	uint playBackMultiplier;
	std::string uuid;
	bool stop;
};

PROPERTYTYPEBASIC(DatabaseLogging, bool)
PROPERTYTYPEBASIC(DatabasePlayback, bool)
PROPERTYTYPE(DatabaseFile, DatabaseFileType, StringPropertyType, std::string)

class DatabaseSink : public AmbPluginImpl
{

public:
	DatabaseSink(AbstractRoutingEngine* engine, map<string, string> config, AbstractSource &parent);
	~DatabaseSink();
	virtual void supportedChanged(const PropertyList & supportedProperties);
	virtual void propertyChanged(AbstractPropertyType *value);
	const std::string uuid() const;

	void init();

	///source role:
	virtual void getRangePropertyAsync(AsyncRangePropertyReply *reply);
	virtual AsyncPropertyReply * setProperty(const AsyncSetPropertyRequest & request);
	virtual void subscribeToPropertyChanges(VehicleProperty::Property property);
	virtual void unsubscribeToPropertyChanges(VehicleProperty::Property property);
	int supportedOperations() const { return AbstractSource::GetRanged | AbstractSource::Get | AbstractSource::Set;}

private: //methods:

	void parseConfig();
	void stopDb();
	void startDb();
	void startPlayback();
	void initDb();
	void updateForNewDbFilename();

private:
	PropertyList mSubscriptions;
	Shared *shared;
	std::unique_ptr<std::thread> thread;
	std::string tablename;
	std::string tablecreate;
	PropertyList propertiesToSubscribeTo;
	PlaybackShared* playbackShared;
	uint playbackMultiplier;
	std::shared_ptr<AbstractPropertyType> playback;
	std::shared_ptr<AbstractPropertyType> databaseName;
	std::shared_ptr<AbstractPropertyType> databaseLogging;
};

#endif // DATABASESINK_H
