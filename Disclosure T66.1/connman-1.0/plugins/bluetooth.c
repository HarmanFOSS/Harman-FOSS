/*
 *
 *  Connection Manager
 *
 *  Copyright (C) 2007-2012  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ether.h>

#include <gdbus.h>

#define CONNMAN_API_SUBJECT_TO_CHANGE
#include <connman/plugin.h>
#include <connman/technology.h>
#include <connman/device.h>
#include <connman/inet.h>
#include <connman/dbus.h>
#include <connman/log.h>

#define BLUEZ_SERVICE			"org.bluez"
#define BLUEZ_MANAGER_INTERFACE		BLUEZ_SERVICE ".Manager"
#define BLUEZ_ADAPTER_INTERFACE		BLUEZ_SERVICE ".Adapter"
#define BLUEZ_DEVICE_INTERFACE		BLUEZ_SERVICE ".Device"
#define BLUEZ_NETWORK_INTERFACE		BLUEZ_SERVICE ".Network"
#define BLUEZ_NETWORK_SERVER		BLUEZ_SERVICE ".NetworkServer"

#define LIST_ADAPTERS			"ListAdapters"
#define ADAPTER_ADDED			"AdapterAdded"
#define ADAPTER_REMOVED			"AdapterRemoved"
#define DEVICE_REMOVED			"DeviceRemoved"

#define PROPERTY_CHANGED		"PropertyChanged"
#define GET_PROPERTIES			"GetProperties"
#define SET_PROPERTY			"SetProperty"

#define CONNECT				"Connect"
#define DISCONNECT			"Disconnect"

#define REGISTER			"Register"
#define UNREGISTER			"Unregister"

#define UUID_NAP	"00001116-0000-1000-8000-00805f9b34fb"

#define TIMEOUT 5000

static DBusConnection *connection;

static GHashTable *bluetooth_devices = NULL;
static GHashTable *bluetooth_networks = NULL;

static int pan_probe(struct connman_network *network)
{
	DBG("network %p", network);

	return 0;
}

static void pan_remove(struct connman_network *network)
{
	DBG("network %p", network);
}

static void connect_reply(DBusPendingCall *call, void *user_data)
{
	struct connman_network *network = user_data;
	DBusMessage *reply;
	DBusError error;
	const char *interface = NULL;
	int index;

	DBG("network %p", network);

	reply = dbus_pending_call_steal_reply(call);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, reply) == TRUE) {
		connman_error("%s", error.message);
		dbus_error_free(&error);

		goto err;
	}

	if (dbus_message_get_args(reply, &error,
					DBUS_TYPE_STRING, &interface,
						DBUS_TYPE_INVALID) == FALSE) {
		if (dbus_error_is_set(&error) == TRUE) {
			connman_error("%s", error.message);
			dbus_error_free(&error);
		} else
			connman_error("Wrong arguments for connect");
		goto err;
	}

	if (interface == NULL)
		goto err;

	DBG("interface %s", interface);

	index = connman_inet_ifindex(interface);

	connman_network_set_index(network, index);

	connman_network_set_connected(network, TRUE);

	dbus_message_unref(reply);

	dbus_pending_call_unref(call);

	return;
err:

	connman_network_set_connected(network, FALSE);

	dbus_message_unref(reply);

	dbus_pending_call_unref(call);
}

static int pan_connect(struct connman_network *network)
{
	const char *path = connman_network_get_string(network, "Path");
	const char *uuid = "nap";
	DBusMessage *message;
	DBusPendingCall *call;

	DBG("network %p", network);

	if (path == NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(BLUEZ_SERVICE, path,
					BLUEZ_NETWORK_INTERFACE, CONNECT);
	if (message == NULL)
		return -ENOMEM;

	dbus_message_set_auto_start(message, FALSE);

	dbus_message_append_args(message, DBUS_TYPE_STRING, &uuid,
							DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(connection, message,
					&call, TIMEOUT * 10) == FALSE) {
		connman_error("Failed to connect service");
		dbus_message_unref(message);
		return -EINVAL;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		dbus_message_unref(message);
		return -EINVAL;
	}

	dbus_pending_call_set_notify(call, connect_reply, network, NULL);

	dbus_message_unref(message);

	return -EINPROGRESS;
}

static void disconnect_reply(DBusPendingCall *call, void *user_data)
{
	struct connman_network *network = user_data;
	DBusMessage *reply;
	DBusError error;

	DBG("network %p", network);

	reply = dbus_pending_call_steal_reply(call);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, reply) == TRUE) {
		connman_error("%s", error.message);
		dbus_error_free(&error);
		goto done;
	}

	if (dbus_message_get_args(reply, &error, DBUS_TYPE_INVALID) == FALSE) {
		if (dbus_error_is_set(&error) == TRUE) {
			connman_error("%s", error.message);
			dbus_error_free(&error);
		} else
			connman_error("Wrong arguments for disconnect");
		goto done;
	}

	connman_network_set_connected(network, FALSE);

done:
	dbus_message_unref(reply);

	dbus_pending_call_unref(call);

	connman_network_unref(network);
}

static int pan_disconnect(struct connman_network *network)
{
	const char *path = connman_network_get_string(network, "Path");
	DBusMessage *message;
	DBusPendingCall *call;

	DBG("network %p", network);

	if (path == NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(BLUEZ_SERVICE, path,
					BLUEZ_NETWORK_INTERFACE, DISCONNECT);
	if (message == NULL)
		return -ENOMEM;

	dbus_message_set_auto_start(message, FALSE);

	dbus_message_append_args(message, DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(connection, message,
						&call, TIMEOUT) == FALSE) {
		connman_error("Failed to disconnect service");
		dbus_message_unref(message);
		return -EINVAL;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		dbus_message_unref(message);
		return -EINVAL;
	}

	connman_network_ref(network);

	connman_network_set_associating(network, FALSE);

	dbus_pending_call_set_notify(call, disconnect_reply, network, NULL);

	dbus_message_unref(message);

	return 0;
}

static struct connman_network_driver pan_driver = {
	.name		= "bluetooth-pan",
	.type		= CONNMAN_NETWORK_TYPE_BLUETOOTH_PAN,
	.probe		= pan_probe,
	.remove		= pan_remove,
	.connect	= pan_connect,
	.disconnect	= pan_disconnect,
};

static gboolean network_changed(DBusConnection *connection,
				DBusMessage *message, void *user_data)
{
	const char *path = dbus_message_get_path(message);
	struct connman_network *network;
	DBusMessageIter iter, value;
	const char *key;

	DBG("path %s", path);

	network = g_hash_table_lookup(bluetooth_networks, path);
	if (network == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &iter) == FALSE)
		return TRUE;

	dbus_message_iter_get_basic(&iter, &key);

	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &value);

	if (g_str_equal(key, "Connected") == TRUE) {
		dbus_bool_t connected;

		dbus_message_iter_get_basic(&value, &connected);

		if (connected == TRUE)
			return TRUE;

		connman_network_set_associating(network, FALSE);
		connman_network_set_connected(network, FALSE);
	}

	return TRUE;
}

static void extract_properties(DBusMessage *reply, const char **parent,
						const char **address,
						const char **name,
						const char **alias,
						dbus_bool_t *powered,
						dbus_bool_t *scanning,
						DBusMessageIter *uuids,
						DBusMessageIter *networks)
{
	DBusMessageIter array, dict;

	if (dbus_message_iter_init(reply, &array) == FALSE)
		return;

	if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_ARRAY)
		return;

	dbus_message_iter_recurse(&array, &dict);

	while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
		DBusMessageIter entry, value;
		const char *key;

		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);

		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &value);

		if (g_str_equal(key, "Adapter") == TRUE) {
			if (parent != NULL)
				dbus_message_iter_get_basic(&value, parent);
		} else if (g_str_equal(key, "Address") == TRUE) {
			if (address != NULL)
				dbus_message_iter_get_basic(&value, address);
		} else if (g_str_equal(key, "Name") == TRUE) {
			if (name != NULL)
				dbus_message_iter_get_basic(&value, name);
		} else if (g_str_equal(key, "Alias") == TRUE) {
			if (alias != NULL)
				dbus_message_iter_get_basic(&value, alias);
		} else if (g_str_equal(key, "Powered") == TRUE) {
			if (powered != NULL)
				dbus_message_iter_get_basic(&value, powered);
		} else if (g_str_equal(key, "Discovering") == TRUE) {
			if (scanning != NULL)
				dbus_message_iter_get_basic(&value, scanning);
		} else if (g_str_equal(key, "Devices") == TRUE) {
			if (networks != NULL)
				memcpy(networks, &value, sizeof(value));
		} else if (g_str_equal(key, "UUIDs") == TRUE) {
			if (uuids != NULL)
				memcpy(uuids, &value, sizeof(value));
		}

		dbus_message_iter_next(&dict);
	}
}

static dbus_bool_t has_pan(DBusMessageIter *array)
{
	DBusMessageIter value;

	if (dbus_message_iter_get_arg_type(array) != DBUS_TYPE_ARRAY)
		return FALSE;

	dbus_message_iter_recurse(array, &value);

	while (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
		const char *uuid;

		dbus_message_iter_get_basic(&value, &uuid);

		if (g_strcmp0(uuid, UUID_NAP) == 0)
			return TRUE;

		dbus_message_iter_next(&value);
	}

	return FALSE;
}

static void network_properties_reply(DBusPendingCall *call, void *user_data)
{
	char *path = user_data;
	struct connman_device *device;
	struct connman_network *network;
	DBusMessage *reply;
	DBusMessageIter uuids;
	const char *parent = NULL, *address = NULL, *name = NULL;
	struct ether_addr addr;
	char ident[13];

	reply = dbus_pending_call_steal_reply(call);

	extract_properties(reply, &parent, &address, NULL, &name,
						NULL, NULL, &uuids, NULL);

	if (parent == NULL)
		goto done;

	device = g_hash_table_lookup(bluetooth_devices, parent);
	if (device == NULL)
		goto done;

	if (address == NULL)
		goto done;

	ether_aton_r(address, &addr);

	snprintf(ident, 13, "%02x%02x%02x%02x%02x%02x",
						addr.ether_addr_octet[0],
						addr.ether_addr_octet[1],
						addr.ether_addr_octet[2],
						addr.ether_addr_octet[3],
						addr.ether_addr_octet[4],
						addr.ether_addr_octet[5]);

	if (has_pan(&uuids) == FALSE)
		goto done;

	network = connman_device_get_network(device, ident);
	if (network != NULL)
		goto done;

	network = connman_network_create(ident,
					CONNMAN_NETWORK_TYPE_BLUETOOTH_PAN);
	if (network == NULL)
		goto done;

	connman_network_set_string(network, "Path", path);

	connman_network_set_name(network, name);

	connman_device_add_network(device, network);

	connman_network_set_group(network, ident);

	g_hash_table_replace(bluetooth_networks, g_strdup(path), network);

done:
	dbus_message_unref(reply);

	dbus_pending_call_unref(call);
}

static void add_network(struct connman_device *device, const char *path)
{
	DBusMessage *message;
	DBusPendingCall *call;

	DBG("path %s", path);

	message = dbus_message_new_method_call(BLUEZ_SERVICE, path,
				BLUEZ_DEVICE_INTERFACE, GET_PROPERTIES);
	if (message == NULL)
		return;

	dbus_message_set_auto_start(message, FALSE);

	if (dbus_connection_send_with_reply(connection, message,
						&call, TIMEOUT) == FALSE) {
		connman_error("Failed to get network properties for %s", path);
		goto done;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		goto done;
	}

	dbus_pending_call_set_notify(call, network_properties_reply,
						g_strdup(path), g_free);

done:
	dbus_message_unref(message);
}

static void check_networks(struct connman_device *device,
						DBusMessageIter *array)
{
	DBusMessageIter value;

	if (dbus_message_iter_get_arg_type(array) != DBUS_TYPE_ARRAY)
		return;

	dbus_message_iter_recurse(array, &value);

	while (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_OBJECT_PATH) {
		const char *path;

		dbus_message_iter_get_basic(&value, &path);

		add_network(device, path);

		dbus_message_iter_next(&value);
	}
}

static gboolean adapter_changed(DBusConnection *connection,
				DBusMessage *message, void *user_data)
{
	const char *path = dbus_message_get_path(message);
	struct connman_device *device;
	DBusMessageIter iter, value;
	const char *key;

	DBG("path %s", path);

	device = g_hash_table_lookup(bluetooth_devices, path);
	if (device == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &iter) == FALSE)
		return TRUE;

	dbus_message_iter_get_basic(&iter, &key);

	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &value);

	if (g_str_equal(key, "Powered") == TRUE) {
		dbus_bool_t val;

		dbus_message_iter_get_basic(&value, &val);
		connman_device_set_powered(device, val);
	} else if (g_str_equal(key, "Discovering") == TRUE) {
		dbus_bool_t val;

		dbus_message_iter_get_basic(&value, &val);
		connman_device_set_scanning(device, val);
	} else if (g_str_equal(key, "Devices") == TRUE) {
		check_networks(device, &value);
	}

	return TRUE;
}

static gboolean device_removed(DBusConnection *connection,
				DBusMessage *message, void *user_data)
{
	const char *network_path;
	struct connman_network *network;
	struct connman_device *device;
	DBusMessageIter iter;

	DBG("");

	if (dbus_message_iter_init(message, &iter) == FALSE)
		return TRUE;

	dbus_message_iter_get_basic(&iter, &network_path);

	network = g_hash_table_lookup(bluetooth_networks, network_path);
	if (network == NULL)
		return TRUE;

	device = connman_network_get_device(network);
	if (device == NULL)
		return TRUE;

	g_hash_table_remove(bluetooth_networks, network_path);

	return TRUE;
}

static gboolean device_changed(DBusConnection *connection,
				DBusMessage *message, void *user_data)
{
	const char *path = dbus_message_get_path(message);
	DBusMessageIter iter, value;
	const char *key;

	DBG("path %s", path);

	if (dbus_message_iter_init(message, &iter) == FALSE)
		return TRUE;

	dbus_message_iter_get_basic(&iter, &key);

	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &value);

	DBG("key %s", key);

	if (g_str_equal(key, "UUIDs") == TRUE)
		add_network(NULL, path);

	return TRUE;
}

static void remove_device_networks(struct connman_device *device)
{
	GHashTableIter iter;
	gpointer key, value;
	GSList *key_list = NULL;
	GSList *list;

	if (bluetooth_networks == NULL)
		return;

	g_hash_table_iter_init(&iter, bluetooth_networks);

	while (g_hash_table_iter_next(&iter, &key, &value) == TRUE) {
		struct connman_network *network = value;

		if (connman_network_get_device(network) != device)
			continue;

		key_list = g_slist_append(key_list, key);
	}

	for (list = key_list; list != NULL; list = list->next) {
		const char *network_path = list->data;

		g_hash_table_remove(bluetooth_networks, network_path);
	}

	g_slist_free(key_list);
}

static void adapter_properties_reply(DBusPendingCall *call, void *user_data)
{
	char *path = user_data;
	struct connman_device *device;
	DBusMessage *reply;
	DBusMessageIter networks;
	const char *address = NULL, *name = NULL;
	dbus_bool_t powered = FALSE, scanning = FALSE;
	struct ether_addr addr;
	char ident[13];

	DBG("path %s", path);

	reply = dbus_pending_call_steal_reply(call);

	if (path == NULL)
		goto done;

	extract_properties(reply, NULL, &address, &name, NULL,
					&powered, &scanning, NULL, &networks);

	if (address == NULL)
		goto done;

	if (g_strcmp0(address, "00:00:00:00:00:00") == 0)
		goto done;

	device = g_hash_table_lookup(bluetooth_devices, path);
	if (device != NULL)
		goto update;

	ether_aton_r(address, &addr);

	snprintf(ident, 13, "%02x%02x%02x%02x%02x%02x",
						addr.ether_addr_octet[0],
						addr.ether_addr_octet[1],
						addr.ether_addr_octet[2],
						addr.ether_addr_octet[3],
						addr.ether_addr_octet[4],
						addr.ether_addr_octet[5]);

	device = connman_device_create(ident, CONNMAN_DEVICE_TYPE_BLUETOOTH);
	if (device == NULL)
		goto done;

	connman_device_set_ident(device, ident);

	connman_device_set_string(device, "Path", path);

	if (connman_device_register(device) < 0) {
		connman_device_unref(device);
		goto done;
	}

	g_hash_table_insert(bluetooth_devices, g_strdup(path), device);

update:
	connman_device_set_string(device, "Address", address);
	connman_device_set_string(device, "Name", name);
	connman_device_set_string(device, "Path", path);

	connman_device_set_powered(device, powered);
	connman_device_set_scanning(device, scanning);

	if (powered == TRUE)
		check_networks(device, &networks);
	else
		remove_device_networks(device);

done:
	dbus_message_unref(reply);

	dbus_pending_call_unref(call);
}

static void add_adapter(DBusConnection *connection, const char *path)
{
	DBusMessage *message;
	DBusPendingCall *call;

	DBG("path %s", path);

	message = dbus_message_new_method_call(BLUEZ_SERVICE, path,
				BLUEZ_ADAPTER_INTERFACE, GET_PROPERTIES);
	if (message == NULL)
		return;

	dbus_message_set_auto_start(message, FALSE);

	if (dbus_connection_send_with_reply(connection, message,
						&call, TIMEOUT) == FALSE) {
		connman_error("Failed to get adapter properties for %s", path);
		goto done;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		goto done;
	}

	dbus_pending_call_set_notify(call, adapter_properties_reply,
						g_strdup(path), g_free);

done:
	dbus_message_unref(message);
}

static gboolean adapter_added(DBusConnection *connection, DBusMessage *message,
				void *user_data)
{
	const char *path;

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH, &path,
				DBUS_TYPE_INVALID);
	add_adapter(connection, path);
	return TRUE;
}

static void remove_adapter(DBusConnection *connection, const char *path)
{
	DBG("path %s", path);

	g_hash_table_remove(bluetooth_devices, path);
}

static gboolean adapter_removed(DBusConnection *connection, DBusMessage *message,
				void *user_data)
{
	const char *path;

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH, &path,
				DBUS_TYPE_INVALID);
	remove_adapter(connection, path);
	return TRUE;
}

static void list_adapters_reply(DBusPendingCall *call, void *user_data)
{
	DBusMessage *reply;
	DBusError error;
	char **adapters;
	int i, num_adapters;

	DBG("");

	reply = dbus_pending_call_steal_reply(call);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, reply) == TRUE) {
		connman_error("%s", error.message);
		dbus_error_free(&error);
		goto done;
	}

	if (dbus_message_get_args(reply, &error,
				DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH,
						&adapters, &num_adapters,
						DBUS_TYPE_INVALID) == FALSE) {
		if (dbus_error_is_set(&error) == TRUE) {
			connman_error("%s", error.message);
			dbus_error_free(&error);
		} else
			connman_error("Wrong arguments for adapter list");
		goto done;
	}

	for (i = 0; i < num_adapters; i++)
		add_adapter(connection, adapters[i]);

	g_strfreev(adapters);

done:
	dbus_message_unref(reply);

	dbus_pending_call_unref(call);
}

static void unregister_device(gpointer data)
{
	struct connman_device *device = data;

	DBG("");

	remove_device_networks(device);

	connman_device_unregister(device);
	connman_device_unref(device);
}

static void remove_network(gpointer data)
{
	struct connman_network *network = data;
	struct connman_device *device;

	DBG("network %p", network);

	device = connman_network_get_device(network);
	if (device != NULL)
		connman_device_remove_network(device, network);

	connman_network_unref(network);
}

static void bluetooth_connect(DBusConnection *connection, void *user_data)
{
	DBusMessage *message;
	DBusPendingCall *call;

	DBG("connection %p", connection);

	bluetooth_devices = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, unregister_device);

	bluetooth_networks = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, remove_network);

	message = dbus_message_new_method_call(BLUEZ_SERVICE, "/",
				BLUEZ_MANAGER_INTERFACE, LIST_ADAPTERS);
	if (message == NULL)
		return;

	dbus_message_set_auto_start(message, FALSE);

	if (dbus_connection_send_with_reply(connection, message,
						&call, TIMEOUT) == FALSE) {
		connman_error("Failed to get Bluetooth adapters");
		goto done;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		goto done;
	}

	dbus_pending_call_set_notify(call, list_adapters_reply, NULL, NULL);

done:
	dbus_message_unref(message);
}

static void bluetooth_disconnect(DBusConnection *connection, void *user_data)
{
	DBG("connection %p", connection);

	if (bluetooth_devices == NULL)
		return;

	g_hash_table_destroy(bluetooth_networks);
	bluetooth_networks = NULL;
	g_hash_table_destroy(bluetooth_devices);
	bluetooth_devices = NULL;
}

static int bluetooth_probe(struct connman_device *device)
{
	DBG("device %p", device);

	return 0;
}

static void bluetooth_remove(struct connman_device *device)
{
	DBG("device %p", device);
}

static void powered_reply(DBusPendingCall *call, void *user_data)
{
	DBusError error;
	DBusMessage *reply;

	DBG("");

	reply = dbus_pending_call_steal_reply(call);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, reply) == TRUE) {
		connman_error("%s", error.message);
		dbus_error_free(&error);
		dbus_message_unref(reply);
		dbus_pending_call_unref(call);
		return;
	}

	dbus_message_unref(reply);
	dbus_pending_call_unref(call);

	add_adapter(connection, user_data);
}

static int change_powered(DBusConnection *connection, const char *path,
							dbus_bool_t powered)
{
	DBusMessage *message;
	DBusMessageIter iter;
	DBusPendingCall *call;

	DBG("");

	if (path == NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(BLUEZ_SERVICE, path,
					BLUEZ_ADAPTER_INTERFACE, SET_PROPERTY);
	if (message == NULL)
		return -ENOMEM;

	dbus_message_set_auto_start(message, FALSE);

	dbus_message_iter_init_append(message, &iter);
	connman_dbus_property_append_basic(&iter, "Powered",
						DBUS_TYPE_BOOLEAN, &powered);

	if (dbus_connection_send_with_reply(connection, message,
						&call, TIMEOUT) == FALSE) {
		connman_error("Failed to change Powered property");
		dbus_message_unref(message);
		return -EINVAL;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		dbus_message_unref(message);
		return -EINVAL;
	}

	dbus_pending_call_set_notify(call, powered_reply,
					g_strdup(path), g_free);

	dbus_message_unref(message);

	return -EINPROGRESS;
}

static int bluetooth_enable(struct connman_device *device)
{
	const char *path = connman_device_get_string(device, "Path");

	DBG("device %p", device);

	return change_powered(connection, path, TRUE);
}

static int bluetooth_disable(struct connman_device *device)
{
	const char *path = connman_device_get_string(device, "Path");

	DBG("device %p", device);

	return change_powered(connection, path, FALSE);
}

static struct connman_device_driver bluetooth_driver = {
	.name		= "bluetooth",
	.type		= CONNMAN_DEVICE_TYPE_BLUETOOTH,
	.probe		= bluetooth_probe,
	.remove		= bluetooth_remove,
	.enable		= bluetooth_enable,
	.disable	= bluetooth_disable,
};

static int tech_probe(struct connman_technology *technology)
{
	return 0;
}

static void tech_remove(struct connman_technology *technology)
{
}

static void server_register_reply(DBusPendingCall *call, void *user_data)
{
	struct connman_technology *technology = user_data;
	DBusError error;
	DBusMessage *reply;

	DBG("");

	reply = dbus_pending_call_steal_reply(call);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, reply) == TRUE) {
		connman_error("%s", error.message);
		dbus_error_free(&error);
		dbus_message_unref(reply);
		dbus_pending_call_unref(call);
		return;
	}

	dbus_message_unref(reply);
	dbus_pending_call_unref(call);

	connman_technology_tethering_notify(technology, TRUE);
}

static void server_unregister_reply(DBusPendingCall *call, void *user_data)
{
	struct connman_technology *technology = user_data;
	DBusError error;
	DBusMessage *reply;

	DBG("");

	reply = dbus_pending_call_steal_reply(call);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, reply) == TRUE) {
		connman_error("%s", error.message);
		dbus_error_free(&error);
		dbus_message_unref(reply);
		dbus_pending_call_unref(call);
		return;
	}

	dbus_message_unref(reply);
	dbus_pending_call_unref(call);

	connman_technology_tethering_notify(technology, FALSE);
}


static void server_register(const char *path, const char *uuid,
				struct connman_technology *technology,
				const char *bridge, connman_bool_t enabled)
{
	DBusMessage *message;
	DBusPendingCall *call;
	char *command;

	DBG("path %s enabled %d", path, enabled);

	command = enabled ? REGISTER : UNREGISTER;

	message = dbus_message_new_method_call(BLUEZ_SERVICE, path,
					BLUEZ_NETWORK_SERVER, command);
	if (message == NULL)
		return;

	dbus_message_set_auto_start(message, FALSE);

	dbus_message_append_args(message, DBUS_TYPE_STRING, &uuid,
							DBUS_TYPE_INVALID);

	if (enabled == TRUE)
		dbus_message_append_args(message, DBUS_TYPE_STRING, &bridge,
							DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(connection, message,
						&call, TIMEOUT) == FALSE) {
		connman_error("Failed to enable PAN server");
		dbus_message_unref(message);
		return;
	}

	if (call == NULL) {
		connman_error("D-Bus connection not available");
		dbus_message_unref(message);
		return;
	}

	if (enabled == TRUE)
		dbus_pending_call_set_notify(call, server_register_reply,
						technology, NULL);
	else
		dbus_pending_call_set_notify(call, server_unregister_reply,
						technology, NULL);

	dbus_message_unref(message);
}

struct tethering_info {
	struct connman_technology *technology;
	const char *bridge;
};

static void enable_nap(gpointer key, gpointer value, gpointer user_data)
{
	struct tethering_info *info = user_data;
	struct connman_device *device = value;
	const char *path;

	DBG("");

	path = connman_device_get_string(device, "Path");

	server_register(path, "nap", info->technology, info->bridge, TRUE);
}

static void disable_nap(gpointer key, gpointer value, gpointer user_data)
{
	struct tethering_info *info = user_data;
	struct connman_device *device = value;
	const char *path;

	DBG("");

	path = connman_device_get_string(device, "Path");

	server_register(path, "nap", info->technology, info->bridge, FALSE);
}

static int tech_set_tethering(struct connman_technology *technology,
				const char *identifier, const char *passphrase,
				const char *bridge, connman_bool_t enabled)
{
	struct tethering_info info = {
		.technology	= technology,
		.bridge		= bridge,
	};

	DBG("bridge %s", bridge);

	if (enabled)
		g_hash_table_foreach(bluetooth_devices, enable_nap, &info);
	else
		g_hash_table_foreach(bluetooth_devices, disable_nap, &info);

	return 0;
}

static struct connman_technology_driver tech_driver = {
	.name		= "bluetooth",
	.type		= CONNMAN_SERVICE_TYPE_BLUETOOTH,
	.probe		= tech_probe,
	.remove		= tech_remove,
	.set_tethering	= tech_set_tethering,
};

static guint watch;
static guint added_watch;
static guint removed_watch;
static guint adapter_watch;
static guint device_watch;
static guint device_removed_watch;
static guint network_watch;

static int bluetooth_init(void)
{
	int err;

	connection = connman_dbus_get_connection();
	if (connection == NULL)
		return -EIO;

	watch = g_dbus_add_service_watch(connection, BLUEZ_SERVICE,
			bluetooth_connect, bluetooth_disconnect, NULL, NULL);

	added_watch = g_dbus_add_signal_watch(connection, NULL, NULL,
						BLUEZ_MANAGER_INTERFACE,
						ADAPTER_ADDED, adapter_added,
						NULL, NULL);

	removed_watch = g_dbus_add_signal_watch(connection, NULL, NULL,
						BLUEZ_MANAGER_INTERFACE,
						ADAPTER_REMOVED, adapter_removed,
						NULL, NULL);

	adapter_watch = g_dbus_add_signal_watch(connection, NULL, NULL,
						BLUEZ_ADAPTER_INTERFACE,
						PROPERTY_CHANGED, adapter_changed,
						NULL, NULL);

	device_removed_watch = g_dbus_add_signal_watch(connection, NULL, NULL,
						BLUEZ_ADAPTER_INTERFACE,
						DEVICE_REMOVED, device_removed,
						NULL, NULL);

	device_watch = g_dbus_add_signal_watch(connection, NULL, NULL,
						BLUEZ_DEVICE_INTERFACE,
						PROPERTY_CHANGED, device_changed,
						NULL, NULL);

	network_watch = g_dbus_add_signal_watch(connection, NULL, NULL,
						BLUEZ_NETWORK_INTERFACE,
						PROPERTY_CHANGED, network_changed,
						NULL, NULL);

	if (watch == 0 || added_watch == 0 || removed_watch == 0
			|| adapter_watch == 0 || network_watch == 0
				|| device_watch == 0
					|| device_removed_watch == 0) {
		err = -EIO;
		goto remove;
	}

	err = connman_network_driver_register(&pan_driver);
	if (err < 0)
		goto remove;

	err = connman_device_driver_register(&bluetooth_driver);
	if (err < 0) {
		connman_network_driver_unregister(&pan_driver);
		goto remove;
	}

	err = connman_technology_driver_register(&tech_driver);
	if (err < 0) {
		connman_device_driver_unregister(&bluetooth_driver);
		connman_network_driver_unregister(&pan_driver);
		goto remove;
	}

	return 0;

remove:
	g_dbus_remove_watch(connection, watch);
	g_dbus_remove_watch(connection, added_watch);
	g_dbus_remove_watch(connection, removed_watch);
	g_dbus_remove_watch(connection, adapter_watch);
	g_dbus_remove_watch(connection, device_removed_watch);
	g_dbus_remove_watch(connection, device_watch);
	g_dbus_remove_watch(connection, network_watch);

	dbus_connection_unref(connection);

	return err;
}

static void bluetooth_exit(void)
{
	g_dbus_remove_watch(connection, watch);
	g_dbus_remove_watch(connection, added_watch);
	g_dbus_remove_watch(connection, removed_watch);
	g_dbus_remove_watch(connection, adapter_watch);
	g_dbus_remove_watch(connection, device_removed_watch);
	g_dbus_remove_watch(connection, device_watch);
	g_dbus_remove_watch(connection, network_watch);

	bluetooth_disconnect(connection, NULL);

	connman_technology_driver_unregister(&tech_driver);

	connman_device_driver_unregister(&bluetooth_driver);
	connman_network_driver_unregister(&pan_driver);

	dbus_connection_unref(connection);
}

CONNMAN_PLUGIN_DEFINE(bluetooth, "Bluetooth technology plugin", VERSION,
		CONNMAN_PLUGIN_PRIORITY_DEFAULT, bluetooth_init, bluetooth_exit)
