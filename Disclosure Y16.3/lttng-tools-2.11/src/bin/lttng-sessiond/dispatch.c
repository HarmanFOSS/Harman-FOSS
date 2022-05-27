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

#include <stddef.h>
#include <stdlib.h>
#include <urcu.h>
#include <common/futex.h>
#include <common/macros.h>

#include "dispatch.h"
#include "ust-app.h"
#include "testpoint.h"
#include "fd-limit.h"
#include "health-sessiond.h"
#include "lttng-sessiond.h"
#include "thread.h"

struct thread_notifiers {
	struct ust_cmd_queue *ust_cmd_queue;
	int apps_cmd_pipe_write_fd;
	int apps_cmd_notify_pipe_write_fd;
	int dispatch_thread_exit;
};

/*
 * For each tracing session, update newly registered apps. The session list
 * lock MUST be acquired before calling this.
 */
static void update_ust_app(int app_sock)
{
	struct ltt_session *sess, *stmp;
	const struct ltt_session_list *session_list = session_get_list();

	/* Consumer is in an ERROR state. Stop any application update. */
	if (uatomic_read(&ust_consumerd_state) == CONSUMER_ERROR) {
		/* Stop the update process since the consumer is dead. */
		return;
	}

	/* For all tracing session(s) */
	cds_list_for_each_entry_safe(sess, stmp, &session_list->head, list) {
		struct ust_app *app;

		if (!session_get(sess)) {
			continue;
		}
		session_lock(sess);
		if (!sess->active || !sess->ust_session) {
			goto unlock_session;
		}

		rcu_read_lock();
		assert(app_sock >= 0);
		app = ust_app_find_by_sock(app_sock);
		if (app == NULL) {
			/*
			 * Application can be unregistered before so
			 * this is possible hence simply stopping the
			 * update.
			 */
			DBG3("UST app update failed to find app sock %d",
				app_sock);
			goto unlock_rcu;
		}
		ust_app_global_update(sess->ust_session, app);
	unlock_rcu:
		rcu_read_unlock();
	unlock_session:
		session_unlock(sess);
		session_put(sess);
	}
}

/*
 * Sanitize the wait queue of the dispatch registration thread meaning removing
 * invalid nodes from it. This is to avoid memory leaks for the case the UST
 * notify socket is never received.
 */
static void sanitize_wait_queue(struct ust_reg_wait_queue *wait_queue)
{
	int ret, nb_fd = 0, i;
	unsigned int fd_added = 0;
	struct lttng_poll_event events;
	struct ust_reg_wait_node *wait_node = NULL, *tmp_wait_node;

	assert(wait_queue);

	lttng_poll_init(&events);

	/* Just skip everything for an empty queue. */
	if (!wait_queue->count) {
		goto end;
	}

	ret = lttng_poll_create(&events, wait_queue->count, LTTNG_CLOEXEC);
	if (ret < 0) {
		goto error_create;
	}

	cds_list_for_each_entry_safe(wait_node, tmp_wait_node,
			&wait_queue->head, head) {
		assert(wait_node->app);
		ret = lttng_poll_add(&events, wait_node->app->sock,
				LPOLLHUP | LPOLLERR);
		if (ret < 0) {
			goto error;
		}

		fd_added = 1;
	}

	if (!fd_added) {
		goto end;
	}

	/*
	 * Poll but don't block so we can quickly identify the faulty events and
	 * clean them afterwards from the wait queue.
	 */
	ret = lttng_poll_wait(&events, 0);
	if (ret < 0) {
		goto error;
	}
	nb_fd = ret;

	for (i = 0; i < nb_fd; i++) {
		/* Get faulty FD. */
		uint32_t revents = LTTNG_POLL_GETEV(&events, i);
		int pollfd = LTTNG_POLL_GETFD(&events, i);

		cds_list_for_each_entry_safe(wait_node, tmp_wait_node,
				&wait_queue->head, head) {
			if (pollfd == wait_node->app->sock &&
					(revents & (LPOLLHUP | LPOLLERR))) {
				cds_list_del(&wait_node->head);
				wait_queue->count--;
				ust_app_destroy(wait_node->app);
				free(wait_node);
				/*
				 * Silence warning of use-after-free in
				 * cds_list_for_each_entry_safe which uses
				 * __typeof__(*wait_node).
				 */
				wait_node = NULL;
				break;
			} else {
				ERR("Unexpected poll events %u for sock %d", revents, pollfd);
				goto error;
			}
		}
	}

	if (nb_fd > 0) {
		DBG("Wait queue sanitized, %d node were cleaned up", nb_fd);
	}

end:
	lttng_poll_clean(&events);
	return;

error:
	lttng_poll_clean(&events);
error_create:
	ERR("Unable to sanitize wait queue");
	return;
}

/*
 * Send a socket to a thread This is called from the dispatch UST registration
 * thread once all sockets are set for the application.
 *
 * The sock value can be invalid, we don't really care, the thread will handle
 * it and make the necessary cleanup if so.
 *
 * On success, return 0 else a negative value being the errno message of the
 * write().
 */
static int send_socket_to_thread(int fd, int sock)
{
	ssize_t ret;

	/*
	 * It's possible that the FD is set as invalid with -1 concurrently just
	 * before calling this function being a shutdown state of the thread.
	 */
	if (fd < 0) {
		ret = -EBADF;
		goto error;
	}

	ret = lttng_write(fd, &sock, sizeof(sock));
	if (ret < sizeof(sock)) {
		PERROR("write apps pipe %d", fd);
		if (ret < 0) {
			ret = -errno;
		}
		goto error;
	}

	/* All good. Don't send back the write positive ret value. */
	ret = 0;
error:
	return (int) ret;
}

static void cleanup_ust_dispatch_thread(void *data)
{
	free(data);
}

/*
 * Dispatch request from the registration threads to the application
 * communication thread.
 */
static void *thread_dispatch_ust_registration(void *data)
{
	int ret, err = -1;
	struct cds_wfcq_node *node;
	struct ust_command *ust_cmd = NULL;
	struct ust_reg_wait_node *wait_node = NULL, *tmp_wait_node;
	struct ust_reg_wait_queue wait_queue = {
		.count = 0,
	};
	struct thread_notifiers *notifiers = data;

	rcu_register_thread();

	health_register(health_sessiond, HEALTH_SESSIOND_TYPE_APP_REG_DISPATCH);

	if (testpoint(sessiond_thread_app_reg_dispatch)) {
		goto error_testpoint;
	}

	health_code_update();

	CDS_INIT_LIST_HEAD(&wait_queue.head);

	DBG("[thread] Dispatch UST command started");

	for (;;) {
		health_code_update();

		/* Atomically prepare the queue futex */
		futex_nto1_prepare(&notifiers->ust_cmd_queue->futex);

		if (CMM_LOAD_SHARED(notifiers->dispatch_thread_exit)) {
			break;
		}

		do {
			struct ust_app *app = NULL;
			ust_cmd = NULL;

			/*
			 * Make sure we don't have node(s) that have hung up before receiving
			 * the notify socket. This is to clean the list in order to avoid
			 * memory leaks from notify socket that are never seen.
			 */
			sanitize_wait_queue(&wait_queue);

			health_code_update();
			/* Dequeue command for registration */
			node = cds_wfcq_dequeue_blocking(
					&notifiers->ust_cmd_queue->head,
					&notifiers->ust_cmd_queue->tail);
			if (node == NULL) {
				DBG("Woken up but nothing in the UST command queue");
				/* Continue thread execution */
				break;
			}

			ust_cmd = caa_container_of(node, struct ust_command, node);

			DBG("Dispatching UST registration pid:%d ppid:%d uid:%d"
					" gid:%d sock:%d name:%s (version %d.%d)",
					ust_cmd->reg_msg.pid, ust_cmd->reg_msg.ppid,
					ust_cmd->reg_msg.uid, ust_cmd->reg_msg.gid,
					ust_cmd->sock, ust_cmd->reg_msg.name,
					ust_cmd->reg_msg.major, ust_cmd->reg_msg.minor);

			if (ust_cmd->reg_msg.type == USTCTL_SOCKET_CMD) {
				wait_node = zmalloc(sizeof(*wait_node));
				if (!wait_node) {
					PERROR("zmalloc wait_node dispatch");
					ret = close(ust_cmd->sock);
					if (ret < 0) {
						PERROR("close ust sock dispatch %d", ust_cmd->sock);
					}
					lttng_fd_put(LTTNG_FD_APPS, 1);
					free(ust_cmd);
					ust_cmd = NULL;
					goto error;
				}
				CDS_INIT_LIST_HEAD(&wait_node->head);

				/* Create application object if socket is CMD. */
				wait_node->app = ust_app_create(&ust_cmd->reg_msg,
						ust_cmd->sock);
				if (!wait_node->app) {
					ret = close(ust_cmd->sock);
					if (ret < 0) {
						PERROR("close ust sock dispatch %d", ust_cmd->sock);
					}
					lttng_fd_put(LTTNG_FD_APPS, 1);
					free(wait_node);
					wait_node = NULL;
					free(ust_cmd);
					ust_cmd = NULL;
					continue;
				}
				/*
				 * Add application to the wait queue so we can set the notify
				 * socket before putting this object in the global ht.
				 */
				cds_list_add(&wait_node->head, &wait_queue.head);
				wait_queue.count++;

				free(ust_cmd);
				ust_cmd = NULL;
				/*
				 * We have to continue here since we don't have the notify
				 * socket and the application MUST be added to the hash table
				 * only at that moment.
				 */
				continue;
			} else {
				/*
				 * Look for the application in the local wait queue and set the
				 * notify socket if found.
				 */
				cds_list_for_each_entry_safe(wait_node, tmp_wait_node,
						&wait_queue.head, head) {
					health_code_update();
					if (wait_node->app->pid == ust_cmd->reg_msg.pid) {
						wait_node->app->notify_sock = ust_cmd->sock;
						cds_list_del(&wait_node->head);
						wait_queue.count--;
						app = wait_node->app;
						free(wait_node);
						wait_node = NULL;
						DBG3("UST app notify socket %d is set", ust_cmd->sock);
						break;
					}
				}

				/*
				 * With no application at this stage the received socket is
				 * basically useless so close it before we free the cmd data
				 * structure for good.
				 */
				if (!app) {
					ret = close(ust_cmd->sock);
					if (ret < 0) {
						PERROR("close ust sock dispatch %d", ust_cmd->sock);
					}
					lttng_fd_put(LTTNG_FD_APPS, 1);
				}
				free(ust_cmd);
				ust_cmd = NULL;
			}

			if (app) {
				/*
				 * @session_lock_list
				 *
				 * Lock the global session list so from the register up to the
				 * registration done message, no thread can see the application
				 * and change its state.
				 */
				session_lock_list();
				rcu_read_lock();

				/*
				 * Add application to the global hash table. This needs to be
				 * done before the update to the UST registry can locate the
				 * application.
				 */
				ust_app_add(app);

				/* Set app version. This call will print an error if needed. */
				(void) ust_app_version(app);

				/* Send notify socket through the notify pipe. */
				ret = send_socket_to_thread(
						notifiers->apps_cmd_notify_pipe_write_fd,
						app->notify_sock);
				if (ret < 0) {
					rcu_read_unlock();
					session_unlock_list();
					/*
					 * No notify thread, stop the UST tracing. However, this is
					 * not an internal error of the this thread thus setting
					 * the health error code to a normal exit.
					 */
					err = 0;
					goto error;
				}

				/*
				 * Update newly registered application with the tracing
				 * registry info already enabled information.
				 */
				update_ust_app(app->sock);

				/*
				 * Don't care about return value. Let the manage apps threads
				 * handle app unregistration upon socket close.
				 */
				(void) ust_app_register_done(app);

				/*
				 * Even if the application socket has been closed, send the app
				 * to the thread and unregistration will take place at that
				 * place.
				 */
				ret = send_socket_to_thread(
						notifiers->apps_cmd_pipe_write_fd,
						app->sock);
				if (ret < 0) {
					rcu_read_unlock();
					session_unlock_list();
					/*
					 * No apps. thread, stop the UST tracing. However, this is
					 * not an internal error of the this thread thus setting
					 * the health error code to a normal exit.
					 */
					err = 0;
					goto error;
				}

				rcu_read_unlock();
				session_unlock_list();
			}
		} while (node != NULL);

		health_poll_entry();
		/* Futex wait on queue. Blocking call on futex() */
		futex_nto1_wait(&notifiers->ust_cmd_queue->futex);
		health_poll_exit();
	}
	/* Normal exit, no error */
	err = 0;

error:
	/* Clean up wait queue. */
	cds_list_for_each_entry_safe(wait_node, tmp_wait_node,
			&wait_queue.head, head) {
		cds_list_del(&wait_node->head);
		wait_queue.count--;
		free(wait_node);
	}

	/* Empty command queue. */
	for (;;) {
		/* Dequeue command for registration */
		node = cds_wfcq_dequeue_blocking(
				&notifiers->ust_cmd_queue->head,
				&notifiers->ust_cmd_queue->tail);
		if (node == NULL) {
			break;
		}
		ust_cmd = caa_container_of(node, struct ust_command, node);
		ret = close(ust_cmd->sock);
		if (ret < 0) {
			PERROR("close ust sock exit dispatch %d", ust_cmd->sock);
		}
		lttng_fd_put(LTTNG_FD_APPS, 1);
		free(ust_cmd);
	}

error_testpoint:
	DBG("Dispatch thread dying");
	if (err) {
		health_error();
		ERR("Health error occurred in %s", __func__);
	}
	health_unregister(health_sessiond);
	rcu_unregister_thread();
	return NULL;
}

static bool shutdown_ust_dispatch_thread(void *data)
{
	struct thread_notifiers *notifiers = data;

	CMM_STORE_SHARED(notifiers->dispatch_thread_exit, 1);
	futex_nto1_wake(&notifiers->ust_cmd_queue->futex);
	return true;
}

bool launch_ust_dispatch_thread(struct ust_cmd_queue *cmd_queue,
		int apps_cmd_pipe_write_fd,
		int apps_cmd_notify_pipe_write_fd)
{
	struct lttng_thread *thread;
	struct thread_notifiers *notifiers;

	notifiers = zmalloc(sizeof(*notifiers));
	if (!notifiers) {
		goto error;
	}
	notifiers->ust_cmd_queue = cmd_queue;
	notifiers->apps_cmd_pipe_write_fd = apps_cmd_pipe_write_fd;
	notifiers->apps_cmd_notify_pipe_write_fd = apps_cmd_notify_pipe_write_fd;

	thread = lttng_thread_create("UST registration dispatch",
			thread_dispatch_ust_registration,
			shutdown_ust_dispatch_thread,
			cleanup_ust_dispatch_thread,
			notifiers);
	if (!thread) {
		goto error;
	}
	lttng_thread_put(thread);
	return true;
error:
	free(notifiers);
	return false;
}
