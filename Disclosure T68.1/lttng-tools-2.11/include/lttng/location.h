/*
 * Copyright (C) 2018 - Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, version 2.1 only,
 * as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LTTNG_LOCATION_H
#define LTTNG_LOCATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum lttng_trace_archive_location_type {
	LTTNG_TRACE_ARCHIVE_LOCATION_TYPE_UNKNOWN = 0,
	LTTNG_TRACE_ARCHIVE_LOCATION_TYPE_LOCAL = 1,
	LTTNG_TRACE_ARCHIVE_LOCATION_TYPE_RELAY = 2,
};

enum lttng_trace_archive_location_status {
	LTTNG_TRACE_ARCHIVE_LOCATION_STATUS_OK = 0,
	LTTNG_TRACE_ARCHIVE_LOCATION_STATUS_INVALID = -1,
	LTTNG_TRACE_ARCHIVE_LOCATION_STATUS_ERROR = -2,
};

enum lttng_trace_archive_location_relay_protocol_type {
	LTTNG_TRACE_ARCHIVE_LOCATION_RELAY_PROTOCOL_TYPE_TCP = 0,
};

/*
 * Location of a trace archive.
 */
struct lttng_trace_archive_location;

/*
 * Get a trace archive location's type.
 */
extern enum lttng_trace_archive_location_type
lttng_trace_archive_location_get_type(
		const struct lttng_trace_archive_location *location);

/*
 * Get the absolute path of a local trace archive location.
 *
 * The trace archive location maintains ownership of the absolute_path.
 */
extern enum lttng_trace_archive_location_status
lttng_trace_archive_location_local_get_absolute_path(
		const struct lttng_trace_archive_location *location,
		const char **absolute_path);

/*
 * Get the host address of the relay daemon associated to this trace archive
 * location. May be a hostname, IPv4, or IPv6 address.
 *
 * The trace archive location maintains ownership of relay_host.
 */
extern enum lttng_trace_archive_location_status
lttng_trace_archive_location_relay_get_host(
		const struct lttng_trace_archive_location *location,
		const char **relay_host);

/*
 * Get the control port of the relay daemon associated to this trace archive
 * location.
 */
extern enum lttng_trace_archive_location_status
lttng_trace_archive_location_relay_get_control_port(
		const struct lttng_trace_archive_location *location,
		uint16_t *control_port);

/*
 * Get the data port of the relay daemon associated to this trace archive
 * location.
 */
extern enum lttng_trace_archive_location_status
lttng_trace_archive_location_relay_get_data_port(
		const struct lttng_trace_archive_location *location,
		uint16_t *data_port);

/*
 * Get the protocol used to communicate with the relay daemon associated to this
 * trace archive location.
 */
extern enum lttng_trace_archive_location_status
lttng_trace_archive_location_relay_get_protocol_type(
		const struct lttng_trace_archive_location *location,
		enum lttng_trace_archive_location_relay_protocol_type *protocol);

/*
 * Get path relative to the relay daemon's current output path.
 *
 * The trace archive location maintains ownership of relative_path.
 */
extern enum lttng_trace_archive_location_status
lttng_trace_archive_location_relay_get_relative_path(
		const struct lttng_trace_archive_location *location,
		const char **relative_path);

#ifdef __cplusplus
}
#endif

#endif /* LTTNG_LOCATION_H */
