/*
 * Copyright (C) 2013 - Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 only, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <common/time.h>
#include <common/error.h>
#include <common/macros.h>
#include <common/error.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <locale.h>
#include <string.h>

static bool utf8_output_supported;

LTTNG_HIDDEN
bool locale_supports_utf8(void)
{
	return utf8_output_supported;
}

LTTNG_HIDDEN
int timespec_to_ms(struct timespec ts, unsigned long *ms)
{
	unsigned long res, remain_ms;

	if (ts.tv_sec > ULONG_MAX / MSEC_PER_SEC) {
		errno = EOVERFLOW;
		return -1;	/* multiplication overflow */
	}
	res = ts.tv_sec * MSEC_PER_SEC;
	remain_ms = ULONG_MAX - res;
	if (ts.tv_nsec / NSEC_PER_MSEC > remain_ms) {
		errno = EOVERFLOW;
		return -1;	/* addition overflow */
	}
	res += ts.tv_nsec / NSEC_PER_MSEC;
	*ms = res;
	return 0;
}

LTTNG_HIDDEN
struct timespec timespec_abs_diff(struct timespec t1, struct timespec t2)
{
	uint64_t ts1 = (uint64_t) t1.tv_sec * (uint64_t) NSEC_PER_SEC +
			(uint64_t) t1.tv_nsec;
	uint64_t ts2 = (uint64_t) t2.tv_sec * (uint64_t) NSEC_PER_SEC +
			(uint64_t) t2.tv_nsec;
	uint64_t diff = max(ts1, ts2) - min(ts1, ts2);
	struct timespec res;

	res.tv_sec = diff / (uint64_t) NSEC_PER_SEC;
	res.tv_nsec = diff % (uint64_t) NSEC_PER_SEC;
	return res;
}

static
void __attribute__((constructor)) init_locale_utf8_support(void)
{
	const char *program_locale = setlocale(LC_ALL, NULL);
	const char *lang = getenv("LANG");

	if (program_locale && strstr(program_locale, "utf8")) {
		utf8_output_supported = true;
	} else if (lang && strstr(lang, "utf8")) {
		utf8_output_supported = true;
	}
}

LTTNG_HIDDEN
int time_to_iso8601_str(time_t time, char *str, size_t len)
{
	int ret = 0;
	struct tm *tm_result;
	struct tm tm_storage;
	size_t strf_ret;

	if (len < ISO8601_STR_LEN) {
		ERR("Buffer too short to format ISO 8601 timestamp: %zu bytes provided when at least %zu are needed",
				len, ISO8601_STR_LEN);
		ret = -1;
		goto end;
	}

        tm_result = localtime_r(&time, &tm_storage);
	if (!tm_result) {
		ret = -1;
		PERROR("Failed to break down timestamp to tm structure");
		goto end;
	}

	strf_ret = strftime(str, len, "%Y%m%dT%H%M%S%z", tm_result);
	if (strf_ret == 0) {
		ret = -1;
		ERR("Failed to format timestamp as local time");
		goto end;
	}
end:
	return ret;
}
