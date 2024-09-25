/*
 * Copyright (C) 2011 - David Goulet <david.goulet@polymtl.ca>
 *                      Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *               2013 - Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <common/pipe.h>
#include <common/utils.h>

#include "manage-kernel.h"
#include "testpoint.h"
#include "health-sessiond.h"
#include "utils.h"
#include "thread.h"
#include "kernel.h"
#include "kernel-consumer.h"

struct thread_notifiers {
	struct lttng_pipe *quit_pipe;
	int kernel_poll_pipe_read_fd;
};

/*
 * Update the kernel poll set of all channel fd available over all tracing
 * session. Add the wakeup pipe at the end of the set.
 */
static int update_kernel_poll(struct lttng_poll_event *events)
{
	int ret;
	struct ltt_kernel_channel *channel;
	struct ltt_session *session;
	const struct ltt_session_list *session_list = session_get_list();

	DBG("Updating kernel poll set");

	session_lock_list();
	cds_list_for_each_entry(session, &session_list->head, list) {
		if (!session_get(session)) {
			continue;
		}
		session_lock(session);
		if (session->kernel_session == NULL) {
			session_unlock(session);
			session_put(session);
			continue;
		}

		cds_list_for_each_entry(channel,
				&session->kernel_session->channel_list.head, list) {
			/* Add channel fd to the kernel poll set */
			ret = lttng_poll_add(events, channel->fd, LPOLLIN | LPOLLRDNORM);
			if (ret < 0) {
				session_unlock(session);
				session_put(session);
				goto error;
			}
			DBG("Channel fd %d added to kernel set", channel->fd);
		}
		session_unlock(session);
		session_put(session);
	}
	session_unlock_list();

	return 0;

error:
	session_unlock_list();
	return -1;
}

/*
 * Find the channel fd from 'fd' over all tracing session. When found, check
 * for new channel stream and send those stream fds to the kernel consumer.
 *
 * Useful for CPU hotplug feature.
 */
static int update_kernel_stream(int fd)
{
	int ret = 0;
	struct ltt_session *session;
	struct ltt_kernel_session *ksess;
	struct ltt_kernel_channel *channel;
	const struct ltt_session_list *session_list = session_get_list();

	DBG("Updating kernel streams for channel fd %d", fd);

	session_lock_list();
	cds_list_for_each_entry(session, &session_list->head, list) {
		if (!session_get(session)) {
			continue;
		}
		session_lock(session);
		if (session->kernel_session == NULL) {
			session_unlock(session);
			session_put(session);
			continue;
		}
		ksess = session->kernel_session;

		cds_list_for_each_entry(channel,
				&ksess->channel_list.head, list) {
			struct lttng_ht_iter iter;
			struct consumer_socket *socket;

			if (channel->fd != fd) {
				continue;
			}
			DBG("Channel found, updating kernel streams");
			ret = kernel_open_channel_stream(channel);
			if (ret < 0) {
				goto error;
			}
			/* Update the stream global counter */
			ksess->stream_count_global += ret;

			/*
			 * Have we already sent fds to the consumer? If yes, it
			 * means that tracing is started so it is safe to send
			 * our updated stream fds.
			 */
			if (ksess->consumer_fds_sent != 1
					|| ksess->consumer == NULL) {
				ret = -1;
				goto error;
			}

			rcu_read_lock();
			cds_lfht_for_each_entry(ksess->consumer->socks->ht,
					&iter.iter, socket, node.node) {
				pthread_mutex_lock(socket->lock);
				ret = kernel_consumer_send_channel_streams(socket,
						channel, ksess,
						session->output_traces ? 1 : 0);
				pthread_mutex_unlock(socket->lock);
				if (ret < 0) {
					rcu_read_unlock();
					goto error;
				}
			}
			rcu_read_unlock();
		}
		session_unlock(session);
		session_put(session);
	}
	session_unlock_list();
	return ret;

error:
	session_unlock(session);
	session_put(session);
	session_unlock_list();
	return ret;
}

/*
 * This thread manage event coming from the kernel.
 *
 * Features supported in this thread:
 *    -) CPU Hotplug
 */
static void *thread_kernel_management(void *data)
{
	int ret, i, pollfd, update_poll_flag = 1, err = -1;
	uint32_t revents, nb_fd;
	char tmp;
	struct lttng_poll_event events;
	struct thread_notifiers *notifiers = data;
	const int quit_pipe_read_fd = lttng_pipe_get_readfd(notifiers->quit_pipe);

	DBG("[thread] Thread manage kernel started");

	health_register(health_sessiond, HEALTH_SESSIOND_TYPE_KERNEL);

	/*
	 * This first step of the while is to clean this structure which could free
	 * non NULL pointers so initialize it before the loop.
	 */
	lttng_poll_init(&events);

	if (testpoint(sessiond_thread_manage_kernel)) {
		goto error_testpoint;
	}

	health_code_update();

	if (testpoint(sessiond_thread_manage_kernel_before_loop)) {
		goto error_testpoint;
	}

	while (1) {
		health_code_update();

		if (update_poll_flag == 1) {
			/* Clean events object. We are about to populate it again. */
			lttng_poll_clean(&events);

			ret = lttng_poll_create(&events, 2, LTTNG_CLOEXEC);
			if (ret < 0) {
				goto error_poll_create;
			}

			ret = lttng_poll_add(&events,
					notifiers->kernel_poll_pipe_read_fd,
					LPOLLIN);
			if (ret < 0) {
				goto error;
			}

			ret = lttng_poll_add(&events,
					quit_pipe_read_fd,
					LPOLLIN);
			if (ret < 0) {
				goto error;
			}

			/* This will add the available kernel channel if any. */
			ret = update_kernel_poll(&events);
			if (ret < 0) {
				goto error;
			}
			update_poll_flag = 0;
		}

		DBG("Thread kernel polling");

		/* Poll infinite value of time */
	restart:
		health_poll_entry();
		ret = lttng_poll_wait(&events, -1);
		DBG("Thread kernel return from poll on %d fds",
				LTTNG_POLL_GETNB(&events));
		health_poll_exit();
		if (ret < 0) {
			/*
			 * Restart interrupted system call.
			 */
			if (errno == EINTR) {
				goto restart;
			}
			goto error;
		} else if (ret == 0) {
			/* Should not happen since timeout is infinite */
			ERR("Return value of poll is 0 with an infinite timeout.\n"
				"This should not have happened! Continuing...");
			continue;
		}

		nb_fd = ret;

		for (i = 0; i < nb_fd; i++) {
			/* Fetch once the poll data */
			revents = LTTNG_POLL_GETEV(&events, i);
			pollfd = LTTNG_POLL_GETFD(&events, i);

			health_code_update();

			if (pollfd == quit_pipe_read_fd) {
				err = 0;
				goto exit;
			}

			/* Check for data on kernel pipe */
			if (revents & LPOLLIN) {
				if (pollfd == notifiers->kernel_poll_pipe_read_fd) {
					(void) lttng_read(notifiers->kernel_poll_pipe_read_fd,
						&tmp, 1);
					/*
					 * Ret value is useless here, if this pipe gets any actions an
					 * update is required anyway.
					 */
					update_poll_flag = 1;
					continue;
				} else {
					/*
					 * New CPU detected by the kernel. Adding kernel stream to
					 * kernel session and updating the kernel consumer
					 */
					ret = update_kernel_stream(pollfd);
					if (ret < 0) {
						continue;
					}
					break;
				}
			} else if (revents & (LPOLLERR | LPOLLHUP | LPOLLRDHUP)) {
				update_poll_flag = 1;
				continue;
			} else {
				ERR("Unexpected poll events %u for sock %d", revents, pollfd);
				goto error;
			}
		}
	}

exit:
error:
	lttng_poll_clean(&events);
error_poll_create:
error_testpoint:
	if (err) {
		health_error();
		ERR("Health error occurred in %s", __func__);
		WARN("Kernel thread died unexpectedly. "
				"Kernel tracing can continue but CPU hotplug is disabled.");
	}
	health_unregister(health_sessiond);
	DBG("Kernel thread dying");
	return NULL;
}

static bool shutdown_kernel_management_thread(void *data)
{
	struct thread_notifiers *notifiers = data;
	const int write_fd = lttng_pipe_get_writefd(notifiers->quit_pipe);

	return notify_thread_pipe(write_fd) == 1;
}

static void cleanup_kernel_management_thread(void *data)
{
	struct thread_notifiers *notifiers = data;

	lttng_pipe_destroy(notifiers->quit_pipe);
	free(notifiers);
}

bool launch_kernel_management_thread(int kernel_poll_pipe_read_fd)
{
	struct lttng_pipe *quit_pipe;
	struct thread_notifiers *notifiers = NULL;
	struct lttng_thread *thread;

	notifiers = zmalloc(sizeof(*notifiers));
	if (!notifiers) {
		goto error_alloc;
	}
	quit_pipe = lttng_pipe_open(FD_CLOEXEC);
	if (!quit_pipe) {
		goto error;
	}
	notifiers->quit_pipe = quit_pipe;
	notifiers->kernel_poll_pipe_read_fd = kernel_poll_pipe_read_fd;

	thread = lttng_thread_create("Kernel management",
			thread_kernel_management,
			shutdown_kernel_management_thread,
			cleanup_kernel_management_thread,
			notifiers);
	if (!thread) {
		goto error;
	}
	lttng_thread_put(thread);
	return true;
error:
	cleanup_kernel_management_thread(notifiers);
error_alloc:
	return false;
}
