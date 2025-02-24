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

#ifndef ABSTRACTDBUSINTERFACE_H
#define ABSTRACTDBUSINTERFACE_H

#include <string>
#include <functional>
#include <unordered_map>
#include <list>
#include <gio/gio.h>
#include <boost/any.hpp>
#include <nullptr.h>
#include "abstractpropertytype.h"
#include <abstractroutingengine.h>
#include "varianttype.h"

const uint getPid(const char *owner);

typedef std::vector<VariantType*> PropertyDBusMap;

class AbstractDBusInterface
{

public:
	AbstractDBusInterface(std::string interfaceName, std::string objectName, GDBusConnection* connection);

	virtual ~AbstractDBusInterface();

	void setDBusConnection(GDBusConnection* connection)
	{
		mConnection = connection;
	}

	void registerObject();
	void unregisterObject();

	void addProperty(VariantType* property);
	virtual void updateValue(VariantType* property);

	static PropertyList implementedProperties() { return mimplementedProperties; }

	static std::list<AbstractDBusInterface *> getObjectsForProperty(std::string property);

	static list<AbstractDBusInterface*> interfaces();

	static std::vector<string> supportedInterfaces();

	std::string interfaceName() { return mInterfaceName; }
	void setInterfaceName(const std::string & ifaceName)
	{
		mInterfaceName = ifaceName;
	}

	bool implementsProperty(std::string property);

	/*!
	 * \brief hasPropertyDBus
	 * \param attributeName, name of DBus property
	 * \return true if attributeName is supported by this interface
	 */
	bool hasPropertyDBus(std::string attributeName)
	{

		for(auto i : propertyDBusMap)
		{
			if(i->name() == attributeName)
			{
				return true;
			}
		}

		return false;
	}

	std::string objectPath() { return mObjectPath; }

	bool isSupported() { return supported; }

	double time() { return mTime; }

	VariantType* property(std::string propertyName)
	{
		if(properties.find(propertyName) != properties.end())
			return properties[propertyName];
		return nullptr;
	}

	AbstractRoutingEngine* re;

	void setObjectPath(std::string op)
	{
		if(objectMap.find(op) != objectMap.end())
			objectMap.erase(op);

		mObjectPath = op;
		objectMap[mObjectPath] = this;
	}

	std::string objectName() { return mPropertyName; }

	Zone::Type zone() { return zoneFilter; }

	std::string source() { return mSourceFilter; }

	std::unordered_map<std::string, VariantType*> getProperties() { return properties; }

	bool isRegistered() { return regId > 0; }

protected:

	void startRegistration();

	static GVariant *getProperty(GDBusConnection * connection, const gchar * sender, const gchar *objectPath,
								 const gchar *interfaceName, const gchar * propertyName, GError** error,
								 gpointer userData);
	static gboolean setProperty(GDBusConnection * connection, const gchar * sender, const gchar *objectPath,
								const gchar *interfaceName, const gchar * propertyName, GVariant *value,
								GError** error, gpointer userData, std::function<void (bool, AsyncPropertyReply::Error)> callback);

	static void handleMyMethodCall(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
								   const gchar *interface_name, const gchar *method_name, GVariant *parameters,
								   GDBusMethodInvocation *invocation, gpointer user_data);

	virtual void setProperty(std::string propertyName, GVariant * value, std::function<void (bool, AsyncPropertyReply::Error)> callback);
	virtual GVariant * getProperty(std::string propertyName);

	void setTimeout(int timeout);

	std::unordered_map<std::string, VariantType*> properties;

	Zone::Type zoneFilter;

	PropertyDBusMap propertyDBusMap;

	bool supported;
	double mTime;
	std::string mSourceFilter;

private:
	std::string mInterfaceName;
	std::string mObjectPath;
	std::string mPropertyName;
	std::string introspectionXml;
	GDBusConnection * mConnection;
	static std::unordered_map<std::string, AbstractDBusInterface*> objectMap;
	static PropertyList mimplementedProperties;
	guint regId;
	int mTimeout;

};

#endif // ABSTRACTDBUSINTERFACE_H
