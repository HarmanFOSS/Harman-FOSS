/*
 * Copyright (C) 2011 - David Goulet <david.goulet@polymtl.ca>
 *                      Julien Desfossez <julien.desfossez@polymtl.ca>
 *                      Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * This header is meant for liblttng and libust internal use ONLY. These
 * declarations should NOT be considered stable API.
 */

#ifndef _LTTNG_SESSIOND_COMM_H
#define _LTTNG_SESSIOND_COMM_H

#include <limits.h>
#include <lttng/lttng.h>
#include <lttng/snapshot-internal.h>
#include <lttng/save-internal.h>
#include <lttng/channel-internal.h>
#include <lttng/trigger/trigger-internal.h>
#include <lttng/rotate-internal.h>
#include <common/compat/socket.h>
#include <common/uri.h>
#include <common/defaults.h>
#include <common/compat/uuid.h>
#include <common/macros.h>
#include <common/optional.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "inet.h"
#include "inet6.h"
#include <common/unix.h>

/* Queue size of listen(2) */
#define LTTNG_SESSIOND_COMM_MAX_LISTEN 64

/* Maximum number of FDs that can be sent over a Unix socket */
#define LTTCOMM_MAX_SEND_FDS           4

/*
 * Get the error code index from 0 since LTTCOMM_OK start at 1000
 */
#define LTTCOMM_ERR_INDEX(code) (code - LTTCOMM_CONSUMERD_COMMAND_SOCK_READY)

enum lttcomm_sessiond_command {
	/* Tracer command */
	LTTNG_ADD_CONTEXT                     = 0,
	/* LTTNG_CALIBRATE used to be here */
	LTTNG_DISABLE_CHANNEL                 = 2,
	LTTNG_DISABLE_EVENT                   = 3,
	LTTNG_LIST_SYSCALLS                   = 4,
	LTTNG_ENABLE_CHANNEL                  = 5,
	LTTNG_ENABLE_EVENT                    = 6,
	/* 7 */
	/* Session daemon command */
	/* 8 */
	LTTNG_DESTROY_SESSION                 = 9,
	LTTNG_LIST_CHANNELS                   = 10,
	LTTNG_LIST_DOMAINS                    = 11,
	LTTNG_LIST_EVENTS                     = 12,
	LTTNG_LIST_SESSIONS                   = 13,
	LTTNG_LIST_TRACEPOINTS                = 14,
	LTTNG_REGISTER_CONSUMER               = 15,
	LTTNG_START_TRACE                     = 16,
	LTTNG_STOP_TRACE                      = 17,
	LTTNG_LIST_TRACEPOINT_FIELDS          = 18,

	/* Consumer */
	LTTNG_DISABLE_CONSUMER                = 19,
	LTTNG_ENABLE_CONSUMER                 = 20,
	LTTNG_SET_CONSUMER_URI                = 21,
	/* 22 */
	/* 23 */
	LTTNG_DATA_PENDING                    = 24,
	LTTNG_SNAPSHOT_ADD_OUTPUT             = 25,
	LTTNG_SNAPSHOT_DEL_OUTPUT             = 26,
	LTTNG_SNAPSHOT_LIST_OUTPUT            = 27,
	LTTNG_SNAPSHOT_RECORD                 = 28,
	/* 29 */
	/* 30 */
	LTTNG_SAVE_SESSION                    = 31,
	LTTNG_TRACK_PID                       = 32,
	LTTNG_UNTRACK_PID                     = 33,
	LTTNG_LIST_TRACKER_PIDS               = 34,
	LTTNG_SET_SESSION_SHM_PATH            = 40,
	LTTNG_REGENERATE_METADATA             = 41,
	LTTNG_REGENERATE_STATEDUMP            = 42,
	LTTNG_REGISTER_TRIGGER                = 43,
	LTTNG_UNREGISTER_TRIGGER              = 44,
	LTTNG_ROTATE_SESSION                  = 45,
	LTTNG_ROTATION_GET_INFO               = 46,
	LTTNG_ROTATION_SET_SCHEDULE           = 47,
	LTTNG_SESSION_LIST_ROTATION_SCHEDULES = 48,
	LTTNG_CREATE_SESSION_EXT              = 49
};

enum lttcomm_relayd_command {
	RELAYD_ADD_STREAM                   = 1,
	RELAYD_CREATE_SESSION               = 2,
	RELAYD_START_DATA                   = 3,
	RELAYD_UPDATE_SYNC_INFO             = 4,
	RELAYD_VERSION                      = 5,
	RELAYD_SEND_METADATA                = 6,
	RELAYD_CLOSE_STREAM                 = 7,
	RELAYD_DATA_PENDING                 = 8,
	RELAYD_QUIESCENT_CONTROL            = 9,
	RELAYD_BEGIN_DATA_PENDING           = 10,
	RELAYD_END_DATA_PENDING             = 11,
	RELAYD_ADD_INDEX                    = 12,
	RELAYD_SEND_INDEX                   = 13,
	RELAYD_CLOSE_INDEX                  = 14,
	/* Live-reading commands (2.4+). */
	RELAYD_LIST_SESSIONS                = 15,
	/* All streams of the channel have been sent to the relayd (2.4+). */
	RELAYD_STREAMS_SENT                 = 16,
	/* Ask the relay to reset the metadata trace file (2.8+) */
	RELAYD_RESET_METADATA               = 17,
	/* Ask the relay to rotate a set of stream files (2.11+) */
	RELAYD_ROTATE_STREAMS                = 18,
	/* Ask the relay to create a trace chunk (2.11+) */
	RELAYD_CREATE_TRACE_CHUNK           = 19,
	/* Ask the relay to close a trace chunk (2.11+) */
	RELAYD_CLOSE_TRACE_CHUNK            = 20,
	/* Ask the relay whether a trace chunk exists (2.11+) */
	RELAYD_TRACE_CHUNK_EXISTS           = 21,
};

/*
 * lttcomm error code.
 */
enum lttcomm_return_code {
	LTTCOMM_CONSUMERD_SUCCESS            = 0,   /* Everything went fine. */
	/*
	 * Some code paths use -1 to express an error, others
	 * negate this consumer return code. Starting codes at
	 * 100 ensures there is no mix-up between this error value
	 * and legitimate status codes.
	 */
	LTTCOMM_CONSUMERD_COMMAND_SOCK_READY = 100, /* Command socket ready */
	LTTCOMM_CONSUMERD_SUCCESS_RECV_FD,          /* Success on receiving fds */
	LTTCOMM_CONSUMERD_ERROR_RECV_FD,            /* Error on receiving fds */
	LTTCOMM_CONSUMERD_ERROR_RECV_CMD,           /* Error on receiving command */
	LTTCOMM_CONSUMERD_POLL_ERROR,               /* Error in polling thread */
	LTTCOMM_CONSUMERD_POLL_NVAL,                /* Poll on closed fd */
	LTTCOMM_CONSUMERD_POLL_HUP,                 /* All fds have hungup */
	LTTCOMM_CONSUMERD_EXIT_SUCCESS,             /* Consumerd exiting normally */
	LTTCOMM_CONSUMERD_EXIT_FAILURE,             /* Consumerd exiting on error */
	LTTCOMM_CONSUMERD_OUTFD_ERROR,              /* Error opening the tracefile */
	LTTCOMM_CONSUMERD_SPLICE_EBADF,             /* EBADF from splice(2) */
	LTTCOMM_CONSUMERD_SPLICE_EINVAL,            /* EINVAL from splice(2) */
	LTTCOMM_CONSUMERD_SPLICE_ENOMEM,            /* ENOMEM from splice(2) */
	LTTCOMM_CONSUMERD_SPLICE_ESPIPE,            /* ESPIPE from splice(2) */
	LTTCOMM_CONSUMERD_ENOMEM,                   /* Consumer is out of memory */
	LTTCOMM_CONSUMERD_ERROR_METADATA,           /* Error with metadata. */
	LTTCOMM_CONSUMERD_FATAL,                    /* Fatal error. */
	LTTCOMM_CONSUMERD_RELAYD_FAIL,              /* Error on remote relayd */
	LTTCOMM_CONSUMERD_CHANNEL_FAIL,             /* Channel creation failed. */
	LTTCOMM_CONSUMERD_CHAN_NOT_FOUND,           /* Channel not found. */
	LTTCOMM_CONSUMERD_ALREADY_SET,              /* Resource already set. */
	LTTCOMM_CONSUMERD_ROTATION_FAIL,            /* Rotation has failed. */
	LTTCOMM_CONSUMERD_SNAPSHOT_FAILED,          /* snapshot has failed. */
	LTTCOMM_CONSUMERD_CREATE_TRACE_CHUNK_FAILED,/* Trace chunk creation failed. */
	LTTCOMM_CONSUMERD_CLOSE_TRACE_CHUNK_FAILED, /* Trace chunk creation failed. */
	LTTCOMM_CONSUMERD_INVALID_PARAMETERS,       /* Invalid parameters. */
	LTTCOMM_CONSUMERD_TRACE_CHUNK_EXISTS_LOCAL, /* Trace chunk exists on consumer daemon. */
	LTTCOMM_CONSUMERD_TRACE_CHUNK_EXISTS_REMOTE,/* Trace chunk exists on relay daemon. */
	LTTCOMM_CONSUMERD_UNKNOWN_TRACE_CHUNK,      /* Unknown trace chunk. */

	/* MUST be last element */
	LTTCOMM_NR,						/* Last element */
};

/* lttng socket protocol. */
enum lttcomm_sock_proto {
	LTTCOMM_SOCK_UDP,
	LTTCOMM_SOCK_TCP,
};

/*
 * Index in the net_families array below. Please keep in sync!
 */
enum lttcomm_sock_domain {
	LTTCOMM_INET      = 0,
	LTTCOMM_INET6     = 1,
};

enum lttcomm_metadata_command {
	LTTCOMM_METADATA_REQUEST = 1,
};

/*
 * Commands sent from the consumerd to the sessiond to request if new metadata
 * is available. This message is used to find the per UID _or_ per PID registry
 * for the channel key. For per UID lookup, the triplet
 * bits_per_long/uid/session_id is used. On lookup failure, we search for the
 * per PID registry indexed by session id ignoring the other values.
 */
struct lttcomm_metadata_request_msg {
	uint64_t session_id; /* Tracing session id */
	uint64_t session_id_per_pid; /* Tracing session id for per-pid */
	uint32_t bits_per_long; /* Consumer ABI */
	uint32_t uid;
	uint64_t key; /* Metadata channel key. */
} LTTNG_PACKED;

struct lttcomm_sockaddr {
	enum lttcomm_sock_domain type;
	union {
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} addr;
} LTTNG_PACKED;

struct lttcomm_sock {
	int32_t fd;
	enum lttcomm_sock_proto proto;
	struct lttcomm_sockaddr sockaddr;
	const struct lttcomm_proto_ops *ops;
} LTTNG_PACKED;

/*
 * Relayd sock. Adds the protocol version to use for the communications with
 * the relayd.
 */
struct lttcomm_relayd_sock {
	struct lttcomm_sock sock;
	uint32_t major;
	uint32_t minor;
} LTTNG_PACKED;

struct lttcomm_net_family {
	int family;
	int (*create) (struct lttcomm_sock *sock, int type, int proto);
};

struct lttcomm_proto_ops {
	int (*bind) (struct lttcomm_sock *sock);
	int (*close) (struct lttcomm_sock *sock);
	int (*connect) (struct lttcomm_sock *sock);
	struct lttcomm_sock *(*accept) (struct lttcomm_sock *sock);
	int (*listen) (struct lttcomm_sock *sock, int backlog);
	ssize_t (*recvmsg) (struct lttcomm_sock *sock, void *buf, size_t len,
			int flags);
	ssize_t (*sendmsg) (struct lttcomm_sock *sock, const void *buf,
			size_t len, int flags);
};

/*
 * Data structure received from lttng client to session daemon.
 */
struct lttcomm_session_msg {
	uint32_t cmd_type;	/* enum lttcomm_sessiond_command */
	struct lttng_session session;
	struct lttng_domain domain;
	union {
		/* Event data */
		struct {
			char channel_name[LTTNG_SYMBOL_NAME_LEN];
			struct lttng_event event LTTNG_PACKED;
			/* Length of following filter expression. */
			uint32_t expression_len;
			/* Length of following bytecode for filter. */
			uint32_t bytecode_len;
			/* Exclusion count (fixed-size strings). */
			uint32_t exclusion_count;
			/* Userspace probe location size. */
			uint32_t userspace_probe_location_len;
			/*
			 * After this structure, the following variable-length
			 * items are transmitted:
			 * - char exclusion_names[LTTNG_SYMBOL_NAME_LEN][exclusion_count]
			 * - char filter_expression[expression_len]
			 * - unsigned char filter_bytecode[bytecode_len]
			 */
		} LTTNG_PACKED enable;
		struct {
			char channel_name[LTTNG_SYMBOL_NAME_LEN];
			struct lttng_event event LTTNG_PACKED;
			/* Length of following filter expression. */
			uint32_t expression_len;
			/* Length of following bytecode for filter. */
			uint32_t bytecode_len;
			/*
			 * After this structure, the following variable-length
			 * items are transmitted:
			 * - unsigned char filter_expression[expression_len]
			 * - unsigned char filter_bytecode[bytecode_len]
			 */
		} LTTNG_PACKED disable;
		/* Create channel */
		struct {
			struct lttng_channel chan LTTNG_PACKED;
			/* struct lttng_channel_extended is already packed. */
			struct lttng_channel_extended extended;
		} LTTNG_PACKED channel;
		/* Context */
		struct {
			char channel_name[LTTNG_SYMBOL_NAME_LEN];
			struct lttng_event_context ctx LTTNG_PACKED;
			uint32_t provider_name_len;
			uint32_t context_name_len;
		} LTTNG_PACKED context;
		/* Use by register_consumer */
		struct {
			char path[PATH_MAX];
		} LTTNG_PACKED reg;
		/* List */
		struct {
			char channel_name[LTTNG_SYMBOL_NAME_LEN];
		} LTTNG_PACKED list;
		struct lttng_calibrate calibrate;
		/* Used by the set_consumer_url and used by create_session also call */
		struct {
			/* Number of lttng_uri following */
			uint32_t size;
		} LTTNG_PACKED uri;
		struct {
			struct lttng_snapshot_output output LTTNG_PACKED;
		} LTTNG_PACKED snapshot_output;
		struct {
			uint32_t wait;
			struct lttng_snapshot_output output LTTNG_PACKED;
		} LTTNG_PACKED snapshot_record;
		struct {
			uint32_t nb_uri;
			unsigned int timer_interval;	/* usec */
		} LTTNG_PACKED session_live;
		struct {
			struct lttng_save_session_attr attr; /* struct already packed */
		} LTTNG_PACKED save_session;
		struct {
			char shm_path[PATH_MAX];
		} LTTNG_PACKED set_shm_path;
		struct {
			uint32_t pid;
		} LTTNG_PACKED pid_tracker;
		struct {
			uint32_t length;
		} LTTNG_PACKED trigger;
		struct {
			uint64_t rotation_id;
		} LTTNG_PACKED get_rotation_info;
		struct {
			/* enum lttng_rotation_schedule_type */
			uint8_t type;
			/*
			 * If set == 1, set schedule to value, if set == 0,
			 * clear this schedule type.
			 */
			uint8_t set;
			uint64_t value;
		} LTTNG_PACKED rotation_set_schedule;
		struct {
			/*
			 * Includes the null-terminator.
			 * Must be an absolute path.
			 *
			 * Size bounded by LTTNG_PATH_MAX.
			 */
			uint16_t home_dir_size;
			uint64_t session_descriptor_size;
			/* An lttng_session_descriptor follows. */
		} LTTNG_PACKED create_session;
	} u;
} LTTNG_PACKED;

#define LTTNG_FILTER_MAX_LEN	65536
#define LTTNG_SESSION_DESCRIPTOR_MAX_LEN	65536

/*
 * Filter bytecode data. The reloc table is located at the end of the
 * bytecode. It is made of tuples: (uint16_t, var. len. string). It
 * starts at reloc_table_offset.
 */
#define LTTNG_FILTER_PADDING	32
struct lttng_filter_bytecode {
	uint32_t len;	/* len of data */
	uint32_t reloc_table_offset;
	uint64_t seqnum;
	char padding[LTTNG_FILTER_PADDING];
	char data[0];
} LTTNG_PACKED;

/*
 * Event exclusion data. At the end of the structure, there will actually
 * by zero or more names, where the actual number of names is given by
 * the 'count' item of the structure.
 */
#define LTTNG_EVENT_EXCLUSION_PADDING	32
struct lttng_event_exclusion {
	uint32_t count;
	char padding[LTTNG_EVENT_EXCLUSION_PADDING];
	char names[0][LTTNG_SYMBOL_NAME_LEN];
} LTTNG_PACKED;

#define LTTNG_EVENT_EXCLUSION_NAME_AT(_exclusion, _i) \
	(&(_exclusion)->names[_i][0])

/*
 * Event command header.
 */
struct lttcomm_event_command_header {
	/* Number of events */
	uint32_t nb_events;
} LTTNG_PACKED;

/*
 * Event extended info header. This is the structure preceding each
 * extended info data.
 */
struct lttcomm_event_extended_header {
	/*
	 * Size of filter string immediately following this header.
	 * This size includes the terminal null character.
	 */
	uint32_t filter_len;

	/*
	 * Number of exclusion names, immediately following the filter
	 * string. Each exclusion name has a fixed length of
	 * LTTNG_SYMBOL_NAME_LEN bytes, including the terminal null
	 * character.
	 */
	uint32_t nb_exclusions;

	/*
	 * Size of the event's userspace probe location (if applicable).
	 */
	uint32_t userspace_probe_location_len;
} LTTNG_PACKED;

/*
 * Command header of the reply to an LTTNG_DESTROY_SESSION command.
 */
struct lttcomm_session_destroy_command_header {
	/* enum lttng_session */
	int32_t rotation_state;
};

/*
 * Data structure for the response from sessiond to the lttng client.
 */
struct lttcomm_lttng_msg {
	uint32_t cmd_type;	/* enum lttcomm_sessiond_command */
	uint32_t ret_code;	/* enum lttcomm_return_code */
	uint32_t pid;		/* pid_t */
	uint32_t cmd_header_size;
	uint32_t data_size;
} LTTNG_PACKED;

struct lttcomm_lttng_output_id {
	uint32_t id;
} LTTNG_PACKED;

/*
 * lttcomm_consumer_msg is the message sent from sessiond to consumerd
 * to either add a channel, add a stream, update a stream, or stop
 * operation.
 */
struct lttcomm_consumer_msg {
	uint32_t cmd_type;	/* enum lttng_consumer_command */
	union {
		struct {
			uint64_t channel_key;
			uint64_t session_id;
			/* ID of the session's current trace chunk. */
			LTTNG_OPTIONAL_COMM(uint64_t) LTTNG_PACKED chunk_id;
			char pathname[PATH_MAX];
			uint64_t relayd_id;
			/* nb_init_streams is the number of streams open initially. */
			uint32_t nb_init_streams;
			char name[LTTNG_SYMBOL_NAME_LEN];
			/* Use splice or mmap to consume this fd */
			enum lttng_event_output output;
			int type; /* Per cpu or metadata. */
			uint64_t tracefile_size; /* bytes */
			uint32_t tracefile_count; /* number of tracefiles */
			/* If the channel's streams have to be monitored or not. */
			uint32_t monitor;
			/* timer to check the streams usage in live mode (usec). */
			unsigned int live_timer_interval;
			/* timer to sample a channel's positions (usec). */
			unsigned int monitor_timer_interval;
		} LTTNG_PACKED channel; /* Only used by Kernel. */
		struct {
			uint64_t stream_key;
			uint64_t channel_key;
			int32_t cpu;	/* On which CPU this stream is assigned. */
			/* Tells the consumer if the stream should be or not monitored. */
			uint32_t no_monitor;
		} LTTNG_PACKED stream;	/* Only used by Kernel. */
		struct {
			uint64_t net_index;
			enum lttng_stream_type type;
			/* Open socket to the relayd */
			struct lttcomm_relayd_sock sock;
			/* Tracing session id associated to the relayd. */
			uint64_t session_id;
			/* Relayd session id, only used with control socket. */
			uint64_t relayd_session_id;
		} LTTNG_PACKED relayd_sock;
		struct {
			uint64_t net_seq_idx;
		} LTTNG_PACKED destroy_relayd;
		struct {
			uint64_t session_id;
		} LTTNG_PACKED data_pending;
		struct {
			uint64_t subbuf_size;			/* bytes */
			uint64_t num_subbuf;			/* power of 2 */
			int32_t overwrite;			/* 1: overwrite, 0: discard */
			uint32_t switch_timer_interval;		/* usec */
			uint32_t read_timer_interval;		/* usec */
			unsigned int live_timer_interval;	/* usec */
			uint32_t monitor_timer_interval;	/* usec */
			int32_t output;				/* splice, mmap */
			int32_t type;				/* metadata or per_cpu */
			uint64_t session_id;			/* Tracing session id */
			char pathname[PATH_MAX];		/* Channel file path. */
			char name[LTTNG_SYMBOL_NAME_LEN];	/* Channel name. */
			/* Credentials used to open the UST buffer shared mappings. */
			struct {
				uint32_t uid;
				uint32_t gid;
			} LTTNG_PACKED buffer_credentials;
			uint64_t relayd_id;			/* Relayd id if apply. */
			uint64_t key;				/* Unique channel key. */
			/* ID of the session's current trace chunk. */
			LTTNG_OPTIONAL_COMM(uint64_t) LTTNG_PACKED chunk_id;
			unsigned char uuid[UUID_LEN];	/* uuid for ust tracer. */
			uint32_t chan_id;			/* Channel ID on the tracer side. */
			uint64_t tracefile_size;	/* bytes */
			uint32_t tracefile_count;	/* number of tracefiles */
			uint64_t session_id_per_pid;	/* Per-pid session ID. */
			/* Tells the consumer if the stream should be or not monitored. */
			uint32_t monitor;
			/*
			 * For UST per UID buffers, this is the application UID of the
			 * channel.  This can be different from the user UID requesting the
			 * channel creation and used for the rights on the stream file
			 * because the application can be in the tracing for instance.
			 */
			uint32_t ust_app_uid;
			int64_t blocking_timeout;
			char root_shm_path[PATH_MAX];
			char shm_path[PATH_MAX];
		} LTTNG_PACKED ask_channel;
		struct {
			uint64_t key;
		} LTTNG_PACKED get_channel;
		struct {
			uint64_t key;
		} LTTNG_PACKED destroy_channel;
		struct {
			uint64_t key;	/* Metadata channel key. */
			uint64_t target_offset;	/* Offset in the consumer */
			uint64_t len;	/* Length of metadata to be received. */
			uint64_t version; /* Version of the metadata. */
		} LTTNG_PACKED push_metadata;
		struct {
			uint64_t key;	/* Metadata channel key. */
		} LTTNG_PACKED close_metadata;
		struct {
			uint64_t key;	/* Metadata channel key. */
		} LTTNG_PACKED setup_metadata;
		struct {
			uint64_t key;	/* Channel key. */
		} LTTNG_PACKED flush_channel;
		struct {
			uint64_t key;	/* Channel key. */
		} LTTNG_PACKED clear_quiescent_channel;
		struct {
			char pathname[PATH_MAX];
			/* Indicate if the snapshot goes on the relayd or locally. */
			uint32_t use_relayd;
			uint32_t metadata;		/* This a metadata snapshot. */
			uint64_t relayd_id;		/* Relayd id if apply. */
			uint64_t key;
			uint64_t nb_packets_per_stream;
		} LTTNG_PACKED snapshot_channel;
		struct {
			uint64_t channel_key;
			uint64_t net_seq_idx;
		} LTTNG_PACKED sent_streams;
		struct {
			uint64_t session_id;
			uint64_t channel_key;
		} LTTNG_PACKED discarded_events;
		struct {
			uint64_t session_id;
			uint64_t channel_key;
		} LTTNG_PACKED lost_packets;
		struct {
			uint64_t session_id;
		} LTTNG_PACKED regenerate_metadata;
		struct {
			uint32_t metadata; /* This is a metadata channel. */
			uint64_t relayd_id; /* Relayd id if apply. */
			uint64_t key;
		} LTTNG_PACKED rotate_channel;
		struct {
			uint64_t session_id;
			uint64_t chunk_id;
		} LTTNG_PACKED check_rotation_pending_local;
		struct {
			uint64_t relayd_id;
			uint64_t session_id;
			uint64_t chunk_id;
		} LTTNG_PACKED check_rotation_pending_relay;
		struct {
			/*
			 * Relayd id, if applicable (remote).
			 *
			 * A directory file descriptor referring to the chunk's
			 * output folder is transmitted if the chunk is local
			 * (relayd_id unset).
			 *
			 * `override_name` is left NULL (all-zeroes) if the
			 * chunk's name is not overridden.
			 */
			LTTNG_OPTIONAL_COMM(uint64_t) LTTNG_PACKED relayd_id;
			char override_name[LTTNG_NAME_MAX];
			uint64_t session_id;
			uint64_t chunk_id;
			uint64_t creation_timestamp;
			LTTNG_OPTIONAL_COMM(struct {
				uint32_t uid;
				uint32_t gid;
			} LTTNG_PACKED ) LTTNG_PACKED credentials;
		} LTTNG_PACKED create_trace_chunk;
		struct {
			LTTNG_OPTIONAL_COMM(uint64_t) LTTNG_PACKED relayd_id;
			uint64_t session_id;
			uint64_t chunk_id;
			uint64_t close_timestamp;
			/* enum lttng_trace_chunk_command_type */
			LTTNG_OPTIONAL_COMM(uint32_t) LTTNG_PACKED close_command;
		} LTTNG_PACKED close_trace_chunk;
		struct {
			LTTNG_OPTIONAL_COMM(uint64_t) LTTNG_PACKED relayd_id;
			uint64_t session_id;
			uint64_t chunk_id;
		} LTTNG_PACKED trace_chunk_exists;
		struct {
			lttng_uuid sessiond_uuid;
		} LTTNG_PACKED init;
	} u;
} LTTNG_PACKED;

/*
 * Channel monitoring message returned to the session daemon on every
 * monitor timer expiration.
 */
struct lttcomm_consumer_channel_monitor_msg {
	/* Key of the sampled channel. */
	uint64_t key;
	/*
	 * Lowest and highest usage (bytes) at the moment the sample was taken.
	 */
	uint64_t lowest, highest;
	/*
	 * Sum of all the consumed positions for a channel.
	 */
	uint64_t total_consumed;
} LTTNG_PACKED;

/*
 * Status message returned to the sessiond after a received command.
 */
struct lttcomm_consumer_status_msg {
	enum lttcomm_return_code ret_code;
} LTTNG_PACKED;

struct lttcomm_consumer_status_channel {
	enum lttcomm_return_code ret_code;
	uint64_t key;
	unsigned int stream_count;
} LTTNG_PACKED;

struct lttcomm_consumer_close_trace_chunk_reply {
	enum lttcomm_return_code ret_code;
	uint32_t path_length;
	char path[];
};

#ifdef HAVE_LIBLTTNG_UST_CTL

#include <lttng/ust-abi.h>

/*
 * Data structure for the commands sent from sessiond to UST.
 */
struct lttcomm_ust_msg {
	uint32_t handle;
	uint32_t cmd;
	union {
		struct lttng_ust_channel channel;
		struct lttng_ust_stream stream;
		struct lttng_ust_event event;
		struct lttng_ust_context context;
		struct lttng_ust_tracer_version version;
	} u;
} LTTNG_PACKED;

/*
 * Data structure for the response from UST to the session daemon.
 * cmd_type is sent back in the reply for validation.
 */
struct lttcomm_ust_reply {
	uint32_t handle;
	uint32_t cmd;
	uint32_t ret_code;	/* enum lttcomm_return_code */
	uint32_t ret_val;	/* return value */
	union {
		struct {
			uint64_t memory_map_size;
		} LTTNG_PACKED channel;
		struct {
			uint64_t memory_map_size;
		} LTTNG_PACKED stream;
		struct lttng_ust_tracer_version version;
	} u;
} LTTNG_PACKED;

#endif /* HAVE_LIBLTTNG_UST_CTL */

LTTNG_HIDDEN const char *lttcomm_get_readable_code(enum lttcomm_return_code code);

LTTNG_HIDDEN int lttcomm_init_inet_sockaddr(struct lttcomm_sockaddr *sockaddr,
		const char *ip, unsigned int port);
LTTNG_HIDDEN int lttcomm_init_inet6_sockaddr(struct lttcomm_sockaddr *sockaddr,
		const char *ip, unsigned int port);

LTTNG_HIDDEN struct lttcomm_sock *lttcomm_alloc_sock(enum lttcomm_sock_proto proto);
LTTNG_HIDDEN int lttcomm_create_sock(struct lttcomm_sock *sock);
LTTNG_HIDDEN struct lttcomm_sock *lttcomm_alloc_sock_from_uri(struct lttng_uri *uri);
LTTNG_HIDDEN void lttcomm_destroy_sock(struct lttcomm_sock *sock);
LTTNG_HIDDEN struct lttcomm_sock *lttcomm_alloc_copy_sock(struct lttcomm_sock *src);
LTTNG_HIDDEN void lttcomm_copy_sock(struct lttcomm_sock *dst,
		struct lttcomm_sock *src);

/* Relayd socket object. */
LTTNG_HIDDEN struct lttcomm_relayd_sock *lttcomm_alloc_relayd_sock(
		struct lttng_uri *uri, uint32_t major, uint32_t minor);

LTTNG_HIDDEN int lttcomm_setsockopt_rcv_timeout(int sock, unsigned int msec);
LTTNG_HIDDEN int lttcomm_setsockopt_snd_timeout(int sock, unsigned int msec);

LTTNG_HIDDEN int lttcomm_sock_get_port(const struct lttcomm_sock *sock,
		uint16_t *port);
/*
 * Set a port to an lttcomm_sock. This will have no effect is the socket is
 * already bound.
 */
LTTNG_HIDDEN int lttcomm_sock_set_port(struct lttcomm_sock *sock, uint16_t port);

LTTNG_HIDDEN void lttcomm_init(void);
/* Get network timeout, in milliseconds */
LTTNG_HIDDEN unsigned long lttcomm_get_network_timeout(void);

#endif	/* _LTTNG_SESSIOND_COMM_H */
