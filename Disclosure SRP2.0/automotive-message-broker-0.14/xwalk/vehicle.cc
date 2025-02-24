// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vehicle.h"

#include <abstractpropertytype.h>
#include <debugout.h>
#include <gio/gio.h>
#include <glib.h>
#include <listplusplus.h>
#include <picojson.h>
#include <superptr.hpp>

#include <algorithm>
#include <map>
#include <memory>

#include "common/extension.h"

common::Instance* Vehicle::CallbackInfo::instance = nullptr;

namespace {
const char* amb_service = "org.automotive.message.broker";
const char* prop_iface = "org.freedesktop.DBus.Properties";

const char* vehicle_error_permission_denied = "permission_denied";
const char* vehicle_error_invalid_operation = "invalid_operation";
const char* vehicle_error_timeout = "timeout";
const char* vehicle_error_invalid_zone = "invalid_zone";
const char* vehicle_error_unknown = "unknown";

picojson::value::array AmbZoneToW3C(const std::vector<int>& amb_zones);
picojson::value::array AmbZoneToW3C(int amb_zone);

template<typename T> unique_ptr<T> make_unique(T* t) {
	return unique_ptr<T>(t);
}

void PostReply(Vehicle::CallbackInfo* cb_obj, picojson::value object) {
	DebugOut() << "Posting reply" << endl;
	picojson::object msg;

	msg["method"] = picojson::value(cb_obj->method);
	msg["asyncCallId"] = picojson::value(cb_obj->callback_id);
	msg["value"] = object;

	std::string message = picojson::value(msg).serialize();

	DebugOut() << "Reply message: " << message << endl;

	cb_obj->instance->PostMessage(message.c_str());
}

void PostError(Vehicle::CallbackInfo* cb_obj, const std::string& error) {
	picojson::object msg;
	msg["method"] = picojson::value(cb_obj->method);
	msg["error"] = picojson::value(true);
	msg["value"] = picojson::value(error);
	msg["asyncCallId"] =
			picojson::value(static_cast<double>(cb_obj->callback_id));

	std::string message = picojson::value(msg).serialize();

	DebugOut() << "Error Reply message: " << message << endl;

	cb_obj->instance->PostMessage(message.c_str());
}

picojson::value GetBasic(GVariant* value) {
	std::string type = g_variant_get_type_string(value);
	picojson::value v;

	if (type == "i") {
		int tempVal = GVS<int>::value(value);
		v = picojson::value(static_cast<double>(tempVal));
	} else if (type == "d") {
		v = picojson::value(GVS<double>::value(value));
	} else if (type == "q") {
		v = picojson::value(static_cast<double>(GVS<uint16_t>::value(value)));
	} else if (type == "n") {
		v = picojson::value(static_cast<double>(GVS<int16_t>::value(value)));
	} else if (type == "y") {
		v = picojson::value(static_cast<double>(GVS<char>::value(value)));
	} else if (type == "u") {
		v = picojson::value(static_cast<double>(GVS<uint32_t>::value(value)));
	} else if (type == "x") {
		v = picojson::value(static_cast<double>(GVS<int64_t>::value(value)));
	} else if (type == "t") {
		v = picojson::value(static_cast<double>(GVS<uint64_t>::value(value)));
	} else if (type == "b") {
		v = picojson::value(GVS<bool>::value(value));
	} else if (type == "s") {
		v = picojson::value(g_variant_get_string(value, nullptr));
	} else {
		DebugOut(DebugOut::Error) << "Unsupported type: " << type << endl;
	}

	return v;
}

GVariant* GetBasic(picojson::value value, const std::string& type) {
	GVariant* v = nullptr;

	if (type == "i") {
		v = g_variant_new(type.c_str(), (int32_t)value.get<double>());
	} else if (type == "d") {
		v = g_variant_new(type.c_str(), value.get<double>());
	} else if (type == "q") {
		v = g_variant_new(type.c_str(), (uint16_t)value.get<double>());
	} else if (type == "n") {
		v = g_variant_new(type.c_str(), (int16_t)value.get<double>());
	} else if (type == "u") {
		v = g_variant_new(type.c_str(), (uint32_t)value.get<double>());
	} else if (type == "x") {
		v = g_variant_new(type.c_str(), (int64_t)value.get<double>());
	} else if (type == "t") {
		v = g_variant_new(type.c_str(), (uint64_t)value.get<double>());
	} else if (type == "b") {
		v = g_variant_new(type.c_str(), value.get<bool>());
	} else if (type == "s") {
		v = g_variant_new(type.c_str(), value.get<std::string>().c_str());
	} else {
		DebugOut(DebugOut::Error) << "Unsupported type: " << type << endl;
	}

	return v;
}

void AsyncGetCallback(GObject* source, GAsyncResult* res, gpointer user_data) {
	debugOut("GetAll() method call completed");

	Vehicle::CallbackInfo *cb_obj =
			static_cast<Vehicle::CallbackInfo*>(user_data);

	auto cb_obj_ptr = make_unique(cb_obj);

	if (!cb_obj_ptr) {
		debugOut("invalid cb object");
		return;
	}

	GError* error = nullptr;

	auto property_map = amb::make_super(
				g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error));

	auto error_ptr = amb::make_super(error);

	if (error_ptr) {
		DebugOut() << "failed to call GetAll on interface: "
				   << error_ptr->message << endl;
		PostError(cb_obj_ptr.get(), "unknown");
		return;
	}

	GVariantIter* iter;
	gchar* key;
	GVariant* value;

	g_variant_get(property_map.get(), "(a{sv})", &iter);

	auto iter_ptr = amb::make_super(iter);

	picojson::value::object object;

	while (g_variant_iter_next(iter_ptr.get(), "{sv}", &key, &value)) {
		auto key_ptr = amb::make_super(key);
		auto value_ptr = amb::make_super(value);

		std::string temp_key = key_ptr.get();

		std::transform(temp_key.begin(), temp_key.begin() + 1,
					   temp_key.begin(), ::tolower);

		object[temp_key] = GetBasic(value_ptr.get());

		if (temp_key == "zone") {
			object[temp_key] =
					picojson::value(AmbZoneToW3C(object[temp_key].get<double>()));
		}
	}

	PostReply(cb_obj_ptr.get(), picojson::value(object));
}

picojson::value::array AmbZoneToW3C(int amb_zone) {
	picojson::value::array z;

	if (amb_zone & Zone::Left) {
		z.push_back(picojson::value("left"));
	}
	if (amb_zone & Zone::Right) {
		z.push_back(picojson::value("right"));
	}
	if (amb_zone & Zone::Front) {
		z.push_back(picojson::value("front"));
	}
	if (amb_zone & Zone::Middle) {
		z.push_back(picojson::value("middle"));
	}
	if (amb_zone & Zone::Center) {
		z.push_back(picojson::value("center"));
	}
	if (amb_zone & Zone::Rear) {
		z.push_back(picojson::value("rear"));
	}

	return z;
}

picojson::value::array AmbZoneToW3C(const std::vector<int>& amb_zones) {
	picojson::value::array zones;

	for (auto i : amb_zones) {
		zones.push_back(picojson::value(AmbZoneToW3C(i)));
	}

	return zones;
}

static void SignalCallback(GDBusConnection* connection,
						   const gchar*,
						   const gchar* object_path,
						   const gchar*,
						   const gchar*,
						   GVariant* parameters,
						   gpointer user_data) {
	DebugOut() << "Got signal" << endl;
	std::vector<ObjectZone*> amb_objects_ =
			*(static_cast<std::vector<ObjectZone*>*>(user_data));

	GVariant* value_array;
	GVariant* iface_name;
	GVariant* invalidated;

	g_variant_get(parameters,
				  "(&s@a{sv}^a&s)",
				  &iface_name,
				  &value_array,
				  &invalidated);

	GVariantIter iter;

	g_variant_iter_init(&iter, value_array);

	ObjectZone* object = nullptr;

	for (auto i : amb_objects_) {
		if (i->object_path == object_path) {
			object = i;
		}
	}

	if (!object) {
		DebugOut(DebugOut::Error) << "received signal for which "
								  << "we have no corresponding amb object" << endl;
		return;
	}

	char* key;
	GVariant* value;

	while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
		auto key_ptr = amb::make_super(key);
		auto value_ptr = amb::make_super(value);

		std::string tempkey = key_ptr.get();

		std::transform(tempkey.begin(), tempkey.begin() + 1, tempkey.begin(),
					   ::tolower);

		object->value[tempkey] = GetBasic(value_ptr.get());

		if (tempkey == "zone") {
			object->value[tempkey] =
					picojson::value(AmbZoneToW3C(object->value[tempkey].get<double>()));
		}
	}

	object->value["interfaceName"] = picojson::value(object->name);

	Vehicle::CallbackInfo call;
	call.method = "subscribe";
	call.callback_id = -1;

	PostReply(&call, picojson::value(object->value));
}

}  // namespace

Vehicle::Vehicle(common::Instance* instance)
	: main_loop_(g_main_loop_new(0, FALSE)),
	  thread_(Vehicle::SetupMainloop, this),
	  instance_(instance),
	  manager_proxy_(nullptr){
	CallbackInfo::instance = instance_;
	thread_.detach();

	GError* error = nullptr;

	dbus_connection_ = amb::make_super(g_bus_get_sync(G_BUS_TYPE_SYSTEM,
													  nullptr,
													  &error));

	auto errorPtr = amb::make_super(error);
	if (errorPtr) {
		DebugOut(DebugOut::Error) << "getting bus: "
				   << errorPtr->message << std::endl;
	}

	GError* error_manager = nullptr;
	manager_proxy_ = amb::make_super(g_dbus_proxy_new_sync(dbus_connection_.get(),
														   G_DBUS_PROXY_FLAGS_NONE, NULL,
														   amb_service,
														   "/",
														   "org.automotive.Manager",
														   NULL,
														   &error));

	auto error_ptr = amb::make_super(error_manager);

	if (error_ptr) {
		DebugOut(DebugOut::Error) << "calling GetAutomotiveManager: "
				   << error_ptr->message << endl;
	}


	GError* list_error = nullptr;
	auto supported_list_variant = amb::make_super(
				g_dbus_proxy_call_sync(manager_proxy_.get(),
									   "List",
									   nullptr,
									   G_DBUS_CALL_FLAGS_NONE,
									   -1, NULL, &list_error));

	auto list_error_ptr = amb::make_super(list_error);

	if (list_error_ptr) {
		DebugOut(DebugOut::Error) << "error calling List: "
				   << error_ptr->message << endl;
		return;
	}

	GVariantIter iter;
	g_variant_iter_init(&iter, supported_list_variant.get());
	picojson::array list;

	gchar* propertyName = nullptr;

	while(g_variant_iter_next(&iter,"s", &propertyName))
	{
		auto propertyNamePtr = amb::make_super(propertyName);

		std::string p = propertyNamePtr.get();

		std::transform(p.begin(), p.begin()+1, p.begin(), ::tolower);
		list.push_back(picojson::value(p));
	}

	picojson::object obj;
	obj["method"] = picojson::value("vehicleSupportedAttributes");
	obj["value"] = picojson::value(list);

	instance->PostMessage(picojson::value(obj).serialize().c_str());
}

Vehicle::~Vehicle() {
	g_main_loop_quit(main_loop_);
	g_main_loop_unref(main_loop_);

	for (auto i : amb_objects_) {
		delete i;
	}
}

void Vehicle::Get(const std::string& property, Zone::Type zone, double ret_id) {
	CallbackInfo* data = new CallbackInfo;

	data->callback_id = ret_id;
	data->method = "get";
	data->instance = instance_;


	std::string find_error;
	std::string obj_pstr = FindProperty(property, zone, find_error);

	if (obj_pstr.empty()) {
		debugOut("could not find property " + property);
		PostError(data, find_error);
		return;
	}

	GError* error = nullptr;

	auto properties_proxy = amb::make_super(
				g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
											  G_DBUS_PROXY_FLAGS_NONE, NULL,
											  amb_service,
											  obj_pstr.c_str(),
											  prop_iface,
											  NULL,
											  &error));

	auto error_ptr = amb::make_super(error);

	if (error_ptr) {
		DebugOut(DebugOut::Error) << "failed to get properties proxy: " << error->message << endl;
		return;
	}

	std::string interfaceName = "org.automotive." + property;

	g_dbus_proxy_call(properties_proxy.get(),
					  "GetAll",
					  g_variant_new("(s)", interfaceName.c_str()),
					  G_DBUS_CALL_FLAGS_NONE, -1, NULL,
					  AsyncGetCallback, data);
}

void Vehicle::GetZones(const std::string& object_name, double ret_id) {
	GError* error(nullptr);

	auto zones_variant = amb::make_super(
				g_dbus_proxy_call_sync(manager_proxy_.get(),
									   "ZonesForObjectName",
									   g_variant_new("(s)",
													 object_name.c_str()),
									   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

	auto error_ptr = amb::make_super(error);

	if (error_ptr) {
		DebugOut() << "error calling ZonesForObjectName: "
				   << error_ptr->message << endl;
		return;
	}

	if (!zones_variant) {
		DebugOut() << "Invalid response from ZonesForObjectName " << endl;
		return;
	}

	GVariantIter* iter(nullptr);

	g_variant_get(zones_variant.get(), "(ai)", &iter);

	if (!iter) {
		DebugOut() << "No zones for object " << object_name << endl;
		return;
	}

	auto iter_ptr = amb::make_super(iter);

	std::vector<int> zones_array;

	GVariant* value(nullptr);

	while ((value = g_variant_iter_next_value(iter_ptr.get()))) {
		auto value_ptr = amb::make_super(value);
		int v = 0;

		g_variant_get(value_ptr.get(), "i", &v);
		zones_array.push_back(v);
	}

	picojson::value::array w3c_zones = AmbZoneToW3C(zones_array);

	CallbackInfo* data = new CallbackInfo;

	data->callback_id = ret_id;
	data->method = "zones";
	data->instance = instance_;

	PostReply(data, picojson::value(w3c_zones));
}

std::string Vehicle::FindProperty(const std::string& object_name, int zone, std::string& error_str) {
	GError* error(nullptr);

	auto object_path_variant = amb::make_super(
				g_dbus_proxy_call_sync(manager_proxy_.get(),
									   "FindObjectForZone",
									   g_variant_new("(si)",
													 object_name.c_str(),
													 zone),
									   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

	auto error_ptr = amb::make_super(error);

	if (error_ptr) {
		DebugOut(DebugOut::Error) << "error calling FindObjectForZone: "
				   << error_ptr->message << endl;

		DebugOut() << "Could not find object in zone: " << zone << endl;
		error_str = vehicle_error_invalid_operation;
		return "";
	}

	if (!object_path_variant) {
		DebugOut() << "Could not find object in zone: "  << zone << endl;
		error_str = vehicle_error_invalid_operation;
		return "";
	}

	gchar* obj_path = nullptr;
	g_variant_get(object_path_variant.get(), "(o)", &obj_path);

	auto obj_path_ptr = amb::make_super(obj_path);

	DebugOut() << "FindObjectForZone() returned object path: "
			   << obj_path_ptr.get() << endl;

	return obj_path;
}

void Vehicle::SetupMainloop(void* data) {
	Vehicle* self = reinterpret_cast<Vehicle*>(data);
	GMainContext* ctx = g_main_context_default();

	g_main_context_push_thread_default(ctx);
	g_main_loop_run(self->main_loop_);
}


void Vehicle::Subscribe(const std::string& object_name, Zone::Type zone) {
	std::string find_error;
	std::string object_path = FindProperty(object_name, zone, find_error);

	if (object_path.empty()) {
		DebugOut() << "Error FindProperty failed for " << object_name;
		return;
	}

	bool already_subscribed = false;

	for (auto i : amb_objects_) {
		if (i->object_path == object_path) {
			already_subscribed = true;
			break;
		}
	}

	if (!already_subscribed) {
		GError* proxy_error = nullptr;

		auto properties_proxy =
				amb::make_super(g_dbus_proxy_new_sync(dbus_connection_.get(),
													  G_DBUS_PROXY_FLAGS_NONE,
													  NULL,
													  amb_service,
													  object_path.c_str(),
													  prop_iface,
													  NULL,
													  &proxy_error));

		auto proxy_error_ptr = amb::make_super(proxy_error);

		if (proxy_error_ptr) {
			DebugOut() << "error creating properties proxy: "
					   << proxy_error_ptr->message << endl;
		}

		std::string interface_name = "org.automotive." + object_name;

		GError* get_all_error = nullptr;

		GVariant* property_map =
				g_dbus_proxy_call_sync(properties_proxy.get(),
									   "GetAll",
									   g_variant_new("(s)", interface_name.c_str()),
									   G_DBUS_CALL_FLAGS_NONE, -1, NULL,
									   &get_all_error);

		auto get_all_error_ptr = amb::make_super(get_all_error);

		if (get_all_error_ptr) {
			DebugOut(DebugOut::Error) << "failed to call GetAll on interface "
									  << interface_name << " "
									  << get_all_error_ptr->message << endl;
			return;
		}

		GVariantIter* iter;

		g_variant_get(property_map, "(a{sv})", &iter);

		auto iter_ptr = amb::make_super(iter);

		char* key;
		GVariant* value;

		ObjectZone* object = new ObjectZone(object_name, zone, object_path);

		while (g_variant_iter_next(iter_ptr.get(), "{sv}", &key, &value)) {
			auto key_ptr = amb::make_super(key);
			auto value_ptr = amb::make_super(value);

			std::string tempkey = key_ptr.get();

			std::transform(tempkey.begin(), tempkey.begin() + 1, tempkey.begin(),
						   ::tolower);

			object->value[tempkey] = GetBasic(value_ptr.get());
		}

		object->handle =
				g_dbus_connection_signal_subscribe(dbus_connection_.get(),
												   amb_service,
												   prop_iface,
												   "PropertiesChanged",
												   object_path.c_str(), NULL,
												   G_DBUS_SIGNAL_FLAGS_NONE,
												   SignalCallback, &amb_objects_,
												   NULL);

		amb_objects_.push_back(object);
	} else {
		DebugOut() << "Already subscribed to " << object_name << endl;
	}
}


void Vehicle::Unsubscribe(const std::string& property, Zone::Type zone) {
	std::vector<ObjectZone*> to_clean;

	for (auto obj : amb_objects_) {
		if (obj->name == property && obj->zone == zone) {
			g_dbus_connection_signal_unsubscribe(dbus_connection_.get(),
												 obj->handle);
			to_clean.push_back(obj);
		}
	}

	for (auto obj : to_clean) {
		removeOne(&amb_objects_, obj);
		delete obj;
	}
}


void Vehicle::Set(const std::string &object_name, picojson::object value,
				  Zone::Type zone, double ret_id)
{
	std::string find_error;
	std::string object_path = FindProperty(object_name, zone, find_error);

	Vehicle::CallbackInfo callback;
	callback.callback_id = ret_id;
	callback.method = "set";
	callback.instance = instance_;

	if (object_path.empty() || !find_error.empty()) {
		DebugOut(DebugOut::Error) << "Object not found.  Check object Name and zone."
								  << object_name << std::endl;
		PostError(&callback, find_error);
		return;
	}

	GError* proxy_error = nullptr;

	auto properties_proxy =
			amb::make_super(g_dbus_proxy_new_sync(dbus_connection_.get(),
												  G_DBUS_PROXY_FLAGS_NONE,
												  NULL,
												  amb_service,
												  object_path.c_str(),
												  prop_iface,
												  NULL,
												  &proxy_error));

	auto proxy_error_ptr = amb::make_super(proxy_error);

	if (proxy_error_ptr) {
		DebugOut(DebugOut::Error) << "Error creating property proxy for " << object_path << std::endl;
		PostError(&callback, vehicle_error_unknown);
		return;
	}

	std::string interface_name = "org.automotive." + object_name;

	for (auto itr : value) {
		GError* err = nullptr;
		std::string attribute = itr.first;

		std::transform(attribute.begin(), attribute.begin()+1, attribute.begin(), ::toupper);

		auto var_value =
				amb::make_super(g_dbus_proxy_call_sync(properties_proxy.get(),
													   "Get",
													   g_variant_new("(ss)",
																	 interface_name.c_str(),
																	 attribute.c_str()),
													   G_DBUS_CALL_FLAGS_NONE,
													   -1,
													   NULL,
													   &err));
		auto err_ptr = amb::make_super(err);
		if (err_ptr || !var_value) {
			DebugOut(DebugOut::Error) << "Error getting initial property signature type: " <<
										 err_ptr->message << endl;
			PostError(&callback, vehicle_error_unknown);
			return;
		}

		GVariant* get_value = nullptr;
		g_variant_get(var_value.get(), "(v)", &get_value);

		auto get_value_ptr = amb::make_super(get_value);

		if (!get_value_ptr) {
			DebugOut(DebugOut::Error) << "Error getting variant value." << endl;
			PostError(&callback, vehicle_error_unknown);
			return;
		}

		GVariant* v = GetBasic(itr.second, g_variant_get_type_string(get_value_ptr.get()));

		if (!v) {
			DebugOut(DebugOut::Error) << "Error converting value to GVariant" << endl;
			PostError(&callback, vehicle_error_unknown);
			return;
		}

		GError* set_error = nullptr;

		DebugOut() << "Trying to set " << attribute << " to " << itr.second.serialize() << endl;

		g_dbus_proxy_call_sync(properties_proxy.get(), "Set",
							   g_variant_new("(ssv)",
											 interface_name.c_str(),
											 attribute.c_str(),
											 v),
							   G_DBUS_CALL_FLAGS_NONE,
							   -1, NULL, &set_error);

		auto set_error_ptr = amb::make_super(set_error);

		if (set_error_ptr) {
			DebugOut(DebugOut::Error) << "error setting property:" << set_error_ptr->message << endl;

			if(set_error_ptr->code == G_IO_ERROR_PERMISSION_DENIED
					|| std::string(g_dbus_error_get_remote_error(set_error_ptr.get())) == "org.freedesktop.DBus.Error.AccessDenied")
			{
				DebugOut(DebugOut::Error) << "permission denied" << endl;
				PostError(&callback, vehicle_error_permission_denied);
				return;
			}
			PostError(&callback, vehicle_error_unknown);
			return;
		}
	}

	PostReply(&callback, picojson::value());
}

void Vehicle::Supported(const string& object_name, double ret_id)
{
	Vehicle::CallbackInfo callback;
	callback.callback_id = ret_id;
	callback.method = "supported";
	callback.instance = instance_;

	GError* error(nullptr);

	auto object_path_variant = amb::make_super(
				g_dbus_proxy_call_sync(manager_proxy_.get(),
									   "FindObject",
									   g_variant_new("(s)", object_name.c_str()),
									   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

	auto error_ptr = amb::make_super(error);

	if (error_ptr) {
		DebugOut(DebugOut::Error) << "error calling FindObjectForZone: "
				   << error_ptr->message << endl;

		DebugOut() << "Could not find object for: "  << object_name << endl;
		PostReply(&callback, picojson::value(false));
		return;
	}

	if (!object_path_variant) {
		DebugOut() << "Could not find object for: "  << object_name << endl;
		PostReply(&callback, picojson::value(false));
		return;
	}

	PostReply(&callback, picojson::value(true));
}

bool Vehicle::AvailableForRetrieval(const string &objectName, const string &attName)
{
	GError* error = nullptr;

	auto supportedVariant = amb::make_super(
				g_dbus_proxy_call_sync(manager_proxy_.get(),
									   "SupportsProperty",
									   g_variant_new("(ss)", objectName.c_str(), attName.c_str()),
									   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

	auto error_ptr = amb::make_super(error);

	if (error_ptr) {
		DebugOut(DebugOut::Error) << "error calling SupportsProperty: "
				   << error_ptr->message << endl;

		DebugOut() << "Could not find object for: "  << objectName << endl;
		return false;
	}

	bool supported = false;
	g_variant_get(supportedVariant.get(), "(b)", &supported);

	return supported;
}
