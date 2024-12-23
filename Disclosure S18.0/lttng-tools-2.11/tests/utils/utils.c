/*
 * Copyright (C) - 2015 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <assert.h>
#include <common/compat/time.h>
#include <common/time.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

static inline
int64_t elapsed_time_ns(struct timespec *t1, struct timespec *t2)
{
	struct timespec delta;

	assert(t1 && t2);
	delta.tv_sec = t2->tv_sec - t1->tv_sec;
	delta.tv_nsec = t2->tv_nsec - t1->tv_nsec;
	return ((int64_t) NSEC_PER_SEC * (int64_t) delta.tv_sec) +
			(int64_t) delta.tv_nsec;
}

int usleep_safe(useconds_t usec)
{
	int ret = 0;
	struct timespec t1, t2;
	int64_t time_remaining_ns = (int64_t) usec * (int64_t) NSEC_PER_USEC;

	ret = lttng_clock_gettime(CLOCK_MONOTONIC, &t1);
	if (ret) {
		ret = -1;
		perror("clock_gettime");
		goto end;
	}

	while (time_remaining_ns > 0) {
		ret = usleep(time_remaining_ns / (int64_t) NSEC_PER_USEC);
		if (ret && errno != EINTR) {
			perror("usleep");
			goto end;
		}

		ret = lttng_clock_gettime(CLOCK_MONOTONIC, &t2);
		if (ret) {
			perror("clock_gettime");
			goto end;
		}

		time_remaining_ns -= elapsed_time_ns(&t1, &t2);
	}
end:
	return ret;
}

int create_file(const char *path)
{
	int ret;

	if (!path) {
		return -1;
	}

	ret = creat(path, S_IRWXU);
	if (ret < 0) {
		perror("creat");
		return -1;
	}

	ret = close(ret);
	if (ret < 0) {
		perror("close");
		return -1;
	}

	return 0;
}

int wait_on_file(const char *path)
{
	int ret;
	struct stat buf;

	if (!path) {
		return -1;
	}

	for (;;) {
		ret = stat(path, &buf);
		if (ret == -1 && errno == ENOENT) {
			ret = poll(NULL, 0, 10);	/* 10 ms delay */
			/* Should return 0 everytime */
			if (ret) {
				if (ret < 0) {
					perror("perror");
				} else {
					fprintf(stderr,
						"poll return value is larger than zero\n");
				}
				return -1;
			}
			continue;			/* retry */
		}
		if (ret) {
			perror("stat");
			return -1;
		}
		break;	/* found */
	}

	return 0;
}
