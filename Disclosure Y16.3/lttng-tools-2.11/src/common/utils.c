/*
 * Copyright (C) 2012 - David Goulet <dgoulet@efficios.com>
 * Copyright (C) 2013 - Raphaël Beamonte <raphael.beamonte@gmail.com>
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

#define _LGPL_SOURCE
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <grp.h>
#include <pwd.h>
#include <sys/file.h>
#include <unistd.h>

#include <common/common.h>
#include <common/readwrite.h>
#include <common/runas.h>
#include <common/compat/getenv.h>
#include <common/compat/string.h>
#include <common/compat/dirent.h>
#include <common/compat/directory-handle.h>
#include <common/dynamic-buffer.h>
#include <common/string-utils/format.h>
#include <lttng/constant.h>

#include "utils.h"
#include "defaults.h"
#include "time.h"

#define PROC_MEMINFO_PATH               "/proc/meminfo"
#define PROC_MEMINFO_MEMAVAILABLE_LINE  "MemAvailable:"
#define PROC_MEMINFO_MEMTOTAL_LINE      "MemTotal:"

/* The length of the longest field of `/proc/meminfo`. */
#define PROC_MEMINFO_FIELD_MAX_NAME_LEN	20

#if (PROC_MEMINFO_FIELD_MAX_NAME_LEN == 20)
#define MAX_NAME_LEN_SCANF_IS_A_BROKEN_API "19"
#else
#error MAX_NAME_LEN_SCANF_IS_A_BROKEN_API must be updated to match (PROC_MEMINFO_FIELD_MAX_NAME_LEN - 1)
#endif

/*
 * Return a partial realpath(3) of the path even if the full path does not
 * exist. For instance, with /tmp/test1/test2/test3, if test2/ does not exist
 * but the /tmp/test1 does, the real path for /tmp/test1 is concatened with
 * /test2/test3 then returned. In normal time, realpath(3) fails if the end
 * point directory does not exist.
 * In case resolved_path is NULL, the string returned was allocated in the
 * function and thus need to be freed by the caller. The size argument allows
 * to specify the size of the resolved_path argument if given, or the size to
 * allocate.
 */
LTTNG_HIDDEN
char *utils_partial_realpath(const char *path, char *resolved_path, size_t size)
{
	char *cut_path = NULL, *try_path = NULL, *try_path_prev = NULL;
	const char *next, *prev, *end;

	/* Safety net */
	if (path == NULL) {
		goto error;
	}

	/*
	 * Identify the end of the path, we don't want to treat the
	 * last char if it is a '/', we will just keep it on the side
	 * to be added at the end, and return a value coherent with
	 * the path given as argument
	 */
	end = path + strlen(path);
	if (*(end-1) == '/') {
		end--;
	}

	/* Initiate the values of the pointers before looping */
	next = path;
	prev = next;
	/* Only to ensure try_path is not NULL to enter the while */
	try_path = (char *)next;

	/* Resolve the canonical path of the first part of the path */
	while (try_path != NULL && next != end) {
		char *try_path_buf = NULL;

		/*
		 * If there is not any '/' left, we want to try with
		 * the full path
		 */
		next = strpbrk(next + 1, "/");
		if (next == NULL) {
			next = end;
		}

		/* Cut the part we will be trying to resolve */
		cut_path = lttng_strndup(path, next - path);
		if (cut_path == NULL) {
			PERROR("lttng_strndup");
			goto error;
		}

		try_path_buf = zmalloc(LTTNG_PATH_MAX);
		if (!try_path_buf) {
			PERROR("zmalloc");
			goto error;
		}

		/* Try to resolve this part */
		try_path = realpath((char *) cut_path, try_path_buf);
		if (try_path == NULL) {
			free(try_path_buf);
			/*
			 * There was an error, we just want to be assured it
			 * is linked to an unexistent directory, if it's another
			 * reason, we spawn an error
			 */
			switch (errno) {
			case ENOENT:
				/* Ignore the error */
				break;
			default:
				PERROR("realpath (partial_realpath)");
				goto error;
				break;
			}
		} else {
			/* Save the place we are before trying the next step */
			try_path_buf = NULL;
			free(try_path_prev);
			try_path_prev = try_path;
			prev = next;
		}

		/* Free the allocated memory */
		free(cut_path);
		cut_path = NULL;
	}

	/* Allocate memory for the resolved path if necessary */
	if (resolved_path == NULL) {
		resolved_path = zmalloc(size);
		if (resolved_path == NULL) {
			PERROR("zmalloc resolved path");
			goto error;
		}
	}

	/*
	 * If we were able to solve at least partially the path, we can concatenate
	 * what worked and what didn't work
	 */
	if (try_path_prev != NULL) {
		/* If we risk to concatenate two '/', we remove one of them */
		if (try_path_prev[strlen(try_path_prev) - 1] == '/' && prev[0] == '/') {
			try_path_prev[strlen(try_path_prev) - 1] = '\0';
		}

		/*
		 * Duplicate the memory used by prev in case resolved_path and
		 * path are pointers for the same memory space
		 */
		cut_path = strdup(prev);
		if (cut_path == NULL) {
			PERROR("strdup");
			goto error;
		}

		/* Concatenate the strings */
		snprintf(resolved_path, size, "%s%s", try_path_prev, cut_path);

		/* Free the allocated memory */
		free(cut_path);
		free(try_path_prev);
		cut_path = NULL;
		try_path_prev = NULL;
	/*
	 * Else, we just copy the path in our resolved_path to
	 * return it as is
	 */
	} else {
		strncpy(resolved_path, path, size);
	}

	/* Then we return the 'partially' resolved path */
	return resolved_path;

error:
	free(resolved_path);
	free(cut_path);
	free(try_path);
	if (try_path_prev != try_path) {
		free(try_path_prev);
	}
	return NULL;
}

static
int expand_double_slashes_dot_and_dotdot(char *path)
{
	size_t expanded_path_len, path_len;
	const char *curr_char, *path_last_char, *next_slash, *prev_slash;

	path_len = strlen(path);
	path_last_char = &path[path_len];

	if (path_len == 0) {
		goto error;
	}

	expanded_path_len = 0;

	/* We iterate over the provided path to expand the "//", "../" and "./" */
	for (curr_char = path; curr_char <= path_last_char; curr_char = next_slash + 1) {
		/* Find the next forward slash. */
		size_t curr_token_len;

		if (curr_char == path_last_char) {
			expanded_path_len++;
			break;
		}

		next_slash = memchr(curr_char, '/', path_last_char - curr_char);
		if (next_slash == NULL) {
			/* Reached the end of the provided path. */
			next_slash = path_last_char;
		}

		/* Compute how long is the previous token. */
		curr_token_len = next_slash - curr_char;
		switch(curr_token_len) {
		case 0:
			/*
			 * The pointer has not move meaning that curr_char is
			 * pointing to a slash. It that case there is no token
			 * to copy, so continue the iteration to find the next
			 * token
			 */
			continue;
		case 1:
			/*
			 * The pointer moved 1 character. Check if that
			 * character is a dot ('.'), if it is: omit it, else
			 * copy the token to the normalized path.
			 */
			if (curr_char[0] == '.') {
				continue;
			}
			break;
		case 2:
			/*
			 * The pointer moved 2 characters. Check if these
			 * characters are double dots ('..'). If that is the
			 * case, we need to remove the last token of the
			 * normalized path.
			 */
			if (curr_char[0] == '.' && curr_char[1] == '.') {
				/*
				 * Find the previous path component by
				 * using the memrchr function to find the
				 * previous forward slash and substract that
				 * len to the resulting path.
				 */
				prev_slash = lttng_memrchr(path, '/', expanded_path_len);
				/*
				 * If prev_slash is NULL, we reached the
				 * beginning of the path. We can't go back any
				 * further.
				 */
				if (prev_slash != NULL) {
					expanded_path_len = prev_slash - path;
				}
				continue;
			}
			break;
		default:
			break;
		}

		/*
		 * Copy the current token which is neither a '.' nor a '..'.
		 */
		path[expanded_path_len++] = '/';
		memcpy(&path[expanded_path_len], curr_char, curr_token_len);
		expanded_path_len += curr_token_len;
	}

	if (expanded_path_len == 0) {
		path[expanded_path_len++] = '/';
	}

	path[expanded_path_len] = '\0';
	return 0;
error:
	return -1;
}

/*
 * Make a full resolution of the given path even if it doesn't exist.
 * This function uses the utils_partial_realpath function to resolve
 * symlinks and relatives paths at the start of the string, and
 * implements functionnalities to resolve the './' and '../' strings
 * in the middle of a path. This function is only necessary because
 * realpath(3) does not accept to resolve unexistent paths.
 * The returned string was allocated in the function, it is thus of
 * the responsibility of the caller to free this memory.
 */
LTTNG_HIDDEN
char *_utils_expand_path(const char *path, bool keep_symlink)
{
	int ret;
	char *absolute_path = NULL;
	char *last_token;
	bool is_dot, is_dotdot;

	/* Safety net */
	if (path == NULL) {
		goto error;
	}

	/* Allocate memory for the absolute_path */
	absolute_path = zmalloc(LTTNG_PATH_MAX);
	if (absolute_path == NULL) {
		PERROR("zmalloc expand path");
		goto error;
	}

	if (path[0] == '/') {
		ret = lttng_strncpy(absolute_path, path, LTTNG_PATH_MAX);
		if (ret) {
			ERR("Path exceeds maximal size of %i bytes", LTTNG_PATH_MAX);
			goto error;
		}
	} else {
		/*
		 * This is a relative path. We need to get the present working
		 * directory and start the path walk from there.
		 */
		char current_working_dir[LTTNG_PATH_MAX];
		char *cwd_ret;

		cwd_ret = getcwd(current_working_dir, sizeof(current_working_dir));
		if (!cwd_ret) {
			goto error;
		}
		/*
		 * Get the number of character in the CWD and allocate an array
		 * to can hold it and the path provided by the caller.
		 */
		ret = snprintf(absolute_path, LTTNG_PATH_MAX, "%s/%s",
				current_working_dir, path);
		if (ret >= LTTNG_PATH_MAX) {
			ERR("Concatenating current working directory %s and path %s exceeds maximal size of %i bytes",
					current_working_dir, path, LTTNG_PATH_MAX);
			goto error;
		}
	}

	if (keep_symlink) {
		/* Resolve partially our path */
		absolute_path = utils_partial_realpath(absolute_path,
				absolute_path, LTTNG_PATH_MAX);
		if (!absolute_path) {
			goto error;
		}
	}

	ret = expand_double_slashes_dot_and_dotdot(absolute_path);
	if (ret) {
		goto error;
	}

	/* Identify the last token */
	last_token = strrchr(absolute_path, '/');

	/* Verify that this token is not a relative path */
	is_dotdot = (strcmp(last_token, "/..") == 0);
	is_dot = (strcmp(last_token, "/.") == 0);

	/* If it is, take action */
	if (is_dot || is_dotdot) {
		/* For both, remove this token */
		*last_token = '\0';

		/* If it was a reference to parent directory, go back one more time */
		if (is_dotdot) {
			last_token = strrchr(absolute_path, '/');

			/* If there was only one level left, we keep the first '/' */
			if (last_token == absolute_path) {
				last_token++;
			}

			*last_token = '\0';
		}
	}

	return absolute_path;

error:
	free(absolute_path);
	return NULL;
}
LTTNG_HIDDEN
char *utils_expand_path(const char *path)
{
	return _utils_expand_path(path, true);
}

LTTNG_HIDDEN
char *utils_expand_path_keep_symlink(const char *path)
{
	return _utils_expand_path(path, false);
}
/*
 * Create a pipe in dst.
 */
LTTNG_HIDDEN
int utils_create_pipe(int *dst)
{
	int ret;

	if (dst == NULL) {
		return -1;
	}

	ret = pipe(dst);
	if (ret < 0) {
		PERROR("create pipe");
	}

	return ret;
}

/*
 * Create pipe and set CLOEXEC flag to both fd.
 *
 * Make sure the pipe opened by this function are closed at some point. Use
 * utils_close_pipe().
 */
LTTNG_HIDDEN
int utils_create_pipe_cloexec(int *dst)
{
	int ret, i;

	if (dst == NULL) {
		return -1;
	}

	ret = utils_create_pipe(dst);
	if (ret < 0) {
		goto error;
	}

	for (i = 0; i < 2; i++) {
		ret = fcntl(dst[i], F_SETFD, FD_CLOEXEC);
		if (ret < 0) {
			PERROR("fcntl pipe cloexec");
			goto error;
		}
	}

error:
	return ret;
}

/*
 * Create pipe and set fd flags to FD_CLOEXEC and O_NONBLOCK.
 *
 * Make sure the pipe opened by this function are closed at some point. Use
 * utils_close_pipe(). Using pipe() and fcntl rather than pipe2() to
 * support OSes other than Linux 2.6.23+.
 */
LTTNG_HIDDEN
int utils_create_pipe_cloexec_nonblock(int *dst)
{
	int ret, i;

	if (dst == NULL) {
		return -1;
	}

	ret = utils_create_pipe(dst);
	if (ret < 0) {
		goto error;
	}

	for (i = 0; i < 2; i++) {
		ret = fcntl(dst[i], F_SETFD, FD_CLOEXEC);
		if (ret < 0) {
			PERROR("fcntl pipe cloexec");
			goto error;
		}
		/*
		 * Note: we override any flag that could have been
		 * previously set on the fd.
		 */
		ret = fcntl(dst[i], F_SETFL, O_NONBLOCK);
		if (ret < 0) {
			PERROR("fcntl pipe nonblock");
			goto error;
		}
	}

error:
	return ret;
}

/*
 * Close both read and write side of the pipe.
 */
LTTNG_HIDDEN
void utils_close_pipe(int *src)
{
	int i, ret;

	if (src == NULL) {
		return;
	}

	for (i = 0; i < 2; i++) {
		/* Safety check */
		if (src[i] < 0) {
			continue;
		}

		ret = close(src[i]);
		if (ret) {
			PERROR("close pipe");
		}
	}
}

/*
 * Create a new string using two strings range.
 */
LTTNG_HIDDEN
char *utils_strdupdelim(const char *begin, const char *end)
{
	char *str;

	str = zmalloc(end - begin + 1);
	if (str == NULL) {
		PERROR("zmalloc strdupdelim");
		goto error;
	}

	memcpy(str, begin, end - begin);
	str[end - begin] = '\0';

error:
	return str;
}

/*
 * Set CLOEXEC flag to the give file descriptor.
 */
LTTNG_HIDDEN
int utils_set_fd_cloexec(int fd)
{
	int ret;

	if (fd < 0) {
		ret = -EINVAL;
		goto end;
	}

	ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (ret < 0) {
		PERROR("fcntl cloexec");
		ret = -errno;
	}

end:
	return ret;
}

/*
 * Create pid file to the given path and filename.
 */
LTTNG_HIDDEN
int utils_create_pid_file(pid_t pid, const char *filepath)
{
	int ret;
	FILE *fp;

	assert(filepath);

	fp = fopen(filepath, "w");
	if (fp == NULL) {
		PERROR("open pid file %s", filepath);
		ret = -1;
		goto error;
	}

	ret = fprintf(fp, "%d\n", (int) pid);
	if (ret < 0) {
		PERROR("fprintf pid file");
		goto error;
	}

	if (fclose(fp)) {
		PERROR("fclose");
	}
	DBG("Pid %d written in file %s", (int) pid, filepath);
	ret = 0;
error:
	return ret;
}

/*
 * Create lock file to the given path and filename.
 * Returns the associated file descriptor, -1 on error.
 */
LTTNG_HIDDEN
int utils_create_lock_file(const char *filepath)
{
	int ret;
	int fd;
	struct flock lock;

	assert(filepath);

	memset(&lock, 0, sizeof(lock));
	fd = open(filepath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR |
		S_IRGRP | S_IWGRP);
	if (fd < 0) {
		PERROR("open lock file %s", filepath);
		fd = -1;
		goto error;
	}

	/*
	 * Attempt to lock the file. If this fails, there is
	 * already a process using the same lock file running
	 * and we should exit.
	 */
	lock.l_whence = SEEK_SET;
	lock.l_type = F_WRLCK;

	ret = fcntl(fd, F_SETLK, &lock);
	if (ret == -1) {
		PERROR("fcntl lock file");
		ERR("Could not get lock file %s, another instance is running.",
			filepath);
		if (close(fd)) {
			PERROR("close lock file");
		}
		fd = ret;
		goto error;
	}

error:
	return fd;
}

/*
 * Create directory using the given path and mode.
 *
 * On success, return 0 else a negative error code.
 */
LTTNG_HIDDEN
int utils_mkdir(const char *path, mode_t mode, int uid, int gid)
{
	int ret;
	struct lttng_directory_handle handle;
	const struct lttng_credentials creds = {
		.uid = (uid_t) uid,
		.gid = (gid_t) gid,
	};

	ret = lttng_directory_handle_init(&handle, NULL);
	if (ret) {
		goto end;
	}
	ret = lttng_directory_handle_create_subdirectory_as_user(
			&handle, path, mode,
			(uid >= 0 || gid >= 0) ? &creds : NULL);
	lttng_directory_handle_fini(&handle);
end:
	return ret;
}

/*
 * Recursively create directory using the given path and mode, under the
 * provided uid and gid.
 *
 * On success, return 0 else a negative error code.
 */
LTTNG_HIDDEN
int utils_mkdir_recursive(const char *path, mode_t mode, int uid, int gid)
{
	int ret;
	struct lttng_directory_handle handle;
	const struct lttng_credentials creds = {
		.uid = (uid_t) uid,
		.gid = (gid_t) gid,
	};

	ret = lttng_directory_handle_init(&handle, NULL);
	if (ret) {
		goto end;
	}
	ret = lttng_directory_handle_create_subdirectory_recursive_as_user(
			&handle, path, mode,
			(uid >= 0 || gid >= 0) ? &creds : NULL);
	lttng_directory_handle_fini(&handle);
end:
	return ret;
}

/*
 * out_stream_path is the output parameter.
 *
 * Return 0 on success or else a negative value.
 */
LTTNG_HIDDEN
int utils_stream_file_path(const char *path_name, const char *file_name,
		uint64_t size, uint64_t count, const char *suffix,
		char *out_stream_path, size_t stream_path_len)
{
	int ret;
        char count_str[MAX_INT_DEC_LEN(count) + 1] = {};
	const char *path_separator;

	if (path_name && path_name[strlen(path_name) - 1] == '/') {
		path_separator = "";
	} else {
		path_separator = "/";
	}

	path_name = path_name ? : "";
	suffix = suffix ? : "";
	if (size > 0) {
		ret = snprintf(count_str, sizeof(count_str), "_%" PRIu64,
				count);
		assert(ret > 0 && ret < sizeof(count_str));
	}

        ret = snprintf(out_stream_path, stream_path_len, "%s%s%s%s%s",
			path_name, path_separator, file_name, count_str,
			suffix);
	if (ret < 0 || ret >= stream_path_len) {
		ERR("Truncation occurred while formatting stream path");
		ret = -1;
	} else {
		ret = 0;
	}
	return ret;
}

/**
 * Parse a string that represents a size in human readable format. It
 * supports decimal integers suffixed by 'k', 'K', 'M' or 'G'.
 *
 * The suffix multiply the integer by:
 * 'k': 1024
 * 'M': 1024^2
 * 'G': 1024^3
 *
 * @param str	The string to parse.
 * @param size	Pointer to a uint64_t that will be filled with the
 *		resulting size.
 *
 * @return 0 on success, -1 on failure.
 */
LTTNG_HIDDEN
int utils_parse_size_suffix(const char * const str, uint64_t * const size)
{
	int ret;
	uint64_t base_size;
	long shift = 0;
	const char *str_end;
	char *num_end;

	if (!str) {
		DBG("utils_parse_size_suffix: received a NULL string.");
		ret = -1;
		goto end;
	}

	/* strtoull will accept a negative number, but we don't want to. */
	if (strchr(str, '-') != NULL) {
		DBG("utils_parse_size_suffix: invalid size string, should not contain '-'.");
		ret = -1;
		goto end;
	}

	/* str_end will point to the \0 */
	str_end = str + strlen(str);
	errno = 0;
	base_size = strtoull(str, &num_end, 0);
	if (errno != 0) {
		PERROR("utils_parse_size_suffix strtoull");
		ret = -1;
		goto end;
	}

	if (num_end == str) {
		/* strtoull parsed nothing, not good. */
		DBG("utils_parse_size_suffix: strtoull had nothing good to parse.");
		ret = -1;
		goto end;
	}

	/* Check if a prefix is present. */
	switch (*num_end) {
	case 'G':
		shift = GIBI_LOG2;
		num_end++;
		break;
	case 'M': /*  */
		shift = MEBI_LOG2;
		num_end++;
		break;
	case 'K':
	case 'k':
		shift = KIBI_LOG2;
		num_end++;
		break;
	case '\0':
		break;
	default:
		DBG("utils_parse_size_suffix: invalid suffix.");
		ret = -1;
		goto end;
	}

	/* Check for garbage after the valid input. */
	if (num_end != str_end) {
		DBG("utils_parse_size_suffix: Garbage after size string.");
		ret = -1;
		goto end;
	}

	*size = base_size << shift;

	/* Check for overflow */
	if ((*size >> shift) != base_size) {
		DBG("utils_parse_size_suffix: oops, overflow detected.");
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return ret;
}

/**
 * Parse a string that represents a time in human readable format. It
 * supports decimal integers suffixed by:
 *     "us" for microsecond,
 *     "ms" for millisecond,
 *     "s"  for second,
 *     "m"  for minute,
 *     "h"  for hour
 *
 * The suffix multiply the integer by:
 *     "us" : 1
 *     "ms" : 1000
 *     "s"  : 1000000
 *     "m"  : 60000000
 *     "h"  : 3600000000
 *
 * Note that unit-less numbers are assumed to be microseconds.
 *
 * @param str		The string to parse, assumed to be NULL-terminated.
 * @param time_us	Pointer to a uint64_t that will be filled with the
 *			resulting time in microseconds.
 *
 * @return 0 on success, -1 on failure.
 */
LTTNG_HIDDEN
int utils_parse_time_suffix(char const * const str, uint64_t * const time_us)
{
	int ret;
	uint64_t base_time;
	uint64_t multiplier = 1;
	const char *str_end;
	char *num_end;

	if (!str) {
		DBG("utils_parse_time_suffix: received a NULL string.");
		ret = -1;
		goto end;
	}

	/* strtoull will accept a negative number, but we don't want to. */
	if (strchr(str, '-') != NULL) {
		DBG("utils_parse_time_suffix: invalid time string, should not contain '-'.");
		ret = -1;
		goto end;
	}

	/* str_end will point to the \0 */
	str_end = str + strlen(str);
	errno = 0;
	base_time = strtoull(str, &num_end, 10);
	if (errno != 0) {
		PERROR("utils_parse_time_suffix strtoull on string \"%s\"", str);
		ret = -1;
		goto end;
	}

	if (num_end == str) {
		/* strtoull parsed nothing, not good. */
		DBG("utils_parse_time_suffix: strtoull had nothing good to parse.");
		ret = -1;
		goto end;
	}

	/* Check if a prefix is present. */
	switch (*num_end) {
	case 'u':
		/*
		 * Microsecond (us)
		 *
		 * Skip the "us" if the string matches the "us" suffix,
		 * otherwise let the check for the end of the string handle
		 * the error reporting.
		 */
		if (*(num_end + 1) == 's') {
			num_end += 2;
		}
		break;
	case 'm':
		if (*(num_end + 1) == 's') {
			/* Millisecond (ms) */
			multiplier = USEC_PER_MSEC;
			/* Skip the 's' */
			num_end++;
		} else {
			/* Minute (m) */
			multiplier = USEC_PER_MINUTE;
		}
		num_end++;
		break;
	case 's':
		/* Second */
		multiplier = USEC_PER_SEC;
		num_end++;
		break;
	case 'h':
		/* Hour */
		multiplier = USEC_PER_HOURS;
		num_end++;
		break;
	case '\0':
		break;
	default:
		DBG("utils_parse_time_suffix: invalid suffix.");
		ret = -1;
		goto end;
	}

	/* Check for garbage after the valid input. */
	if (num_end != str_end) {
		DBG("utils_parse_time_suffix: Garbage after time string.");
		ret = -1;
		goto end;
	}

	*time_us = base_time * multiplier;

	/* Check for overflow */
	if ((*time_us / multiplier) != base_time) {
		DBG("utils_parse_time_suffix: oops, overflow detected.");
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return ret;
}

/*
 * fls: returns the position of the most significant bit.
 * Returns 0 if no bit is set, else returns the position of the most
 * significant bit (from 1 to 32 on 32-bit, from 1 to 64 on 64-bit).
 */
#if defined(__i386) || defined(__x86_64)
static inline unsigned int fls_u32(uint32_t x)
{
	int r;

	asm("bsrl %1,%0\n\t"
		"jnz 1f\n\t"
		"movl $-1,%0\n\t"
		"1:\n\t"
		: "=r" (r) : "rm" (x));
	return r + 1;
}
#define HAS_FLS_U32
#endif

#if defined(__x86_64)
static inline
unsigned int fls_u64(uint64_t x)
{
	long r;

	asm("bsrq %1,%0\n\t"
	    "jnz 1f\n\t"
	    "movq $-1,%0\n\t"
	    "1:\n\t"
	    : "=r" (r) : "rm" (x));
	return r + 1;
}
#define HAS_FLS_U64
#endif

#ifndef HAS_FLS_U64
static __attribute__((unused))
unsigned int fls_u64(uint64_t x)
{
	unsigned int r = 64;

	if (!x)
		return 0;

	if (!(x & 0xFFFFFFFF00000000ULL)) {
		x <<= 32;
		r -= 32;
	}
	if (!(x & 0xFFFF000000000000ULL)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xFF00000000000000ULL)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xF000000000000000ULL)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xC000000000000000ULL)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x8000000000000000ULL)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}
#endif

#ifndef HAS_FLS_U32
static __attribute__((unused)) unsigned int fls_u32(uint32_t x)
{
	unsigned int r = 32;

	if (!x) {
		return 0;
	}
	if (!(x & 0xFFFF0000U)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xFF000000U)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xF0000000U)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xC0000000U)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000U)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}
#endif

/*
 * Return the minimum order for which x <= (1UL << order).
 * Return -1 if x is 0.
 */
LTTNG_HIDDEN
int utils_get_count_order_u32(uint32_t x)
{
	if (!x) {
		return -1;
	}

	return fls_u32(x - 1);
}

/*
 * Return the minimum order for which x <= (1UL << order).
 * Return -1 if x is 0.
 */
LTTNG_HIDDEN
int utils_get_count_order_u64(uint64_t x)
{
	if (!x) {
		return -1;
	}

	return fls_u64(x - 1);
}

/**
 * Obtain the value of LTTNG_HOME environment variable, if exists.
 * Otherwise returns the value of HOME.
 */
LTTNG_HIDDEN
const char *utils_get_home_dir(void)
{
	char *val = NULL;
	struct passwd *pwd;

	val = lttng_secure_getenv(DEFAULT_LTTNG_HOME_ENV_VAR);
	if (val != NULL) {
		goto end;
	}
	val = lttng_secure_getenv(DEFAULT_LTTNG_FALLBACK_HOME_ENV_VAR);
	if (val != NULL) {
		goto end;
	}

	/* Fallback on the password file entry. */
	pwd = getpwuid(getuid());
	if (!pwd) {
		goto end;
	}
	val = pwd->pw_dir;

	DBG3("Home directory is '%s'", val);

end:
	return val;
}

/**
 * Get user's home directory. Dynamically allocated, must be freed
 * by the caller.
 */
LTTNG_HIDDEN
char *utils_get_user_home_dir(uid_t uid)
{
	struct passwd pwd;
	struct passwd *result;
	char *home_dir = NULL;
	char *buf = NULL;
	long buflen;
	int ret;

	buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (buflen == -1) {
		goto end;
	}
retry:
	buf = zmalloc(buflen);
	if (!buf) {
		goto end;
	}

	ret = getpwuid_r(uid, &pwd, buf, buflen, &result);
	if (ret || !result) {
		if (ret == ERANGE) {
			free(buf);
			buflen *= 2;
			goto retry;
		}
		goto end;
	}

	home_dir = strdup(pwd.pw_dir);
end:
	free(buf);
	return home_dir;
}

/*
 * With the given format, fill dst with the time of len maximum siz.
 *
 * Return amount of bytes set in the buffer or else 0 on error.
 */
LTTNG_HIDDEN
size_t utils_get_current_time_str(const char *format, char *dst, size_t len)
{
	size_t ret;
	time_t rawtime;
	struct tm *timeinfo;

	assert(format);
	assert(dst);

	/* Get date and time for session path */
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	ret = strftime(dst, len, format, timeinfo);
	if (ret == 0) {
		ERR("Unable to strftime with format %s at dst %p of len %zu", format,
				dst, len);
	}

	return ret;
}

/*
 * Return 0 on success and set *gid to the group_ID matching the passed name.
 * Else -1 if it cannot be found or an error occurred.
 */
LTTNG_HIDDEN
int utils_get_group_id(const char *name, bool warn, gid_t *gid)
{
	static volatile int warn_once;
	int ret;
	long sys_len;
	size_t len;
	struct group grp;
	struct group *result;
	struct lttng_dynamic_buffer buffer;

	/* Get the system limit, if it exists. */
	sys_len = sysconf(_SC_GETGR_R_SIZE_MAX);
	if (sys_len == -1) {
		len = 1024;
	} else {
		len = (size_t) sys_len;
	}

	lttng_dynamic_buffer_init(&buffer);
	ret = lttng_dynamic_buffer_set_size(&buffer, len);
	if (ret) {
		ERR("Failed to allocate group info buffer");
		ret = -1;
		goto error;
	}

	while ((ret = getgrnam_r(name, &grp, buffer.data, buffer.size, &result)) == ERANGE) {
		const size_t new_len = 2 * buffer.size;

		/* Buffer is not big enough, increase its size. */
		if (new_len < buffer.size) {
			ERR("Group info buffer size overflow");
			ret = -1;
			goto error;
		}

		ret = lttng_dynamic_buffer_set_size(&buffer, new_len);
		if (ret) {
			ERR("Failed to grow group info buffer to %zu bytes",
					new_len);
			ret = -1;
			goto error;
		}
	}
	if (ret) {
		PERROR("Failed to get group file entry for group name \"%s\"",
				name);
		ret = -1;
		goto error;
	}

	/* Group not found. */
	if (!result) {
		ret = -1;
		goto error;
	}

	*gid = result->gr_gid;
	ret = 0;

error:
	if (ret && warn && !warn_once) {
		WARN("No tracing group detected");
		warn_once = 1;
	}
	lttng_dynamic_buffer_reset(&buffer);
	return ret;
}

/*
 * Return a newly allocated option string. This string is to be used as the
 * optstring argument of getopt_long(), see GETOPT(3). opt_count is the number
 * of elements in the long_options array. Returns NULL if the string's
 * allocation fails.
 */
LTTNG_HIDDEN
char *utils_generate_optstring(const struct option *long_options,
		size_t opt_count)
{
	int i;
	size_t string_len = opt_count, str_pos = 0;
	char *optstring;

	/*
	 * Compute the necessary string length. One letter per option, two when an
	 * argument is necessary, and a trailing NULL.
	 */
	for (i = 0; i < opt_count; i++) {
		string_len += long_options[i].has_arg ? 1 : 0;
	}

	optstring = zmalloc(string_len);
	if (!optstring) {
		goto end;
	}

	for (i = 0; i < opt_count; i++) {
		if (!long_options[i].name) {
			/* Got to the trailing NULL element */
			break;
		}

		if (long_options[i].val != '\0') {
			optstring[str_pos++] = (char) long_options[i].val;
			if (long_options[i].has_arg) {
				optstring[str_pos++] = ':';
			}
		}
	}

end:
	return optstring;
}

/*
 * Try to remove a hierarchy of empty directories, recursively. Don't unlink
 * any file. Try to rmdir any empty directory within the hierarchy.
 */
LTTNG_HIDDEN
int utils_recursive_rmdir(const char *path)
{
	int ret;
	struct lttng_directory_handle handle;

	ret = lttng_directory_handle_init(&handle, NULL);
	if (ret) {
		goto end;
	}
	ret = lttng_directory_handle_remove_subdirectory(&handle, path);
	lttng_directory_handle_fini(&handle);
end:
	return ret;
}

LTTNG_HIDDEN
int utils_truncate_stream_file(int fd, off_t length)
{
	int ret;
	off_t lseek_ret;

	ret = ftruncate(fd, length);
	if (ret < 0) {
		PERROR("ftruncate");
		goto end;
	}
	lseek_ret = lseek(fd, length, SEEK_SET);
	if (lseek_ret < 0) {
		PERROR("lseek");
		ret = -1;
		goto end;
	}
end:
	return ret;
}

static const char *get_man_bin_path(void)
{
	char *env_man_path = lttng_secure_getenv(DEFAULT_MAN_BIN_PATH_ENV);

	if (env_man_path) {
		return env_man_path;
	}

	return DEFAULT_MAN_BIN_PATH;
}

LTTNG_HIDDEN
int utils_show_help(int section, const char *page_name,
		const char *help_msg)
{
	char section_string[8];
	const char *man_bin_path = get_man_bin_path();
	int ret = 0;

	if (help_msg) {
		printf("%s", help_msg);
		goto end;
	}

	/* Section integer -> section string */
	ret = sprintf(section_string, "%d", section);
	assert(ret > 0 && ret < 8);

	/*
	 * Execute man pager.
	 *
	 * We provide -M to man here because LTTng-tools can
	 * be installed outside /usr, in which case its man pages are
	 * not located in the default /usr/share/man directory.
	 */
	ret = execlp(man_bin_path, "man", "-M", MANPATH,
		section_string, page_name, NULL);

end:
	return ret;
}

static
int read_proc_meminfo_field(const char *field, size_t *value)
{
	int ret;
	FILE *proc_meminfo;
	char name[PROC_MEMINFO_FIELD_MAX_NAME_LEN] = {};

	proc_meminfo = fopen(PROC_MEMINFO_PATH, "r");
	if (!proc_meminfo) {
		PERROR("Failed to fopen() " PROC_MEMINFO_PATH);
		ret = -1;
		goto fopen_error;
	 }

	/*
	 * Read the contents of /proc/meminfo line by line to find the right
	 * field.
	 */
	while (!feof(proc_meminfo)) {
		unsigned long value_kb;

		ret = fscanf(proc_meminfo,
				"%" MAX_NAME_LEN_SCANF_IS_A_BROKEN_API "s %lu kB\n",
				name, &value_kb);
		if (ret == EOF) {
			/*
			 * fscanf() returning EOF can indicate EOF or an error.
			 */
			if (ferror(proc_meminfo)) {
				PERROR("Failed to parse " PROC_MEMINFO_PATH);
			}
			break;
		}

		if (ret == 2 && strcmp(name, field) == 0) {
			/*
			 * This number is displayed in kilo-bytes. Return the
			 * number of bytes.
			 */
			*value = ((size_t) value_kb) * 1024;
			ret = 0;
			goto found;
		}
	}
	/* Reached the end of the file without finding the right field. */
	ret = -1;

found:
	fclose(proc_meminfo);
fopen_error:
	return ret;
}

/*
 * Returns an estimate of the number of bytes of memory available based on the
 * the information in `/proc/meminfo`. The number returned by this function is
 * a best guess.
 */
LTTNG_HIDDEN
int utils_get_memory_available(size_t *value)
{
	return read_proc_meminfo_field(PROC_MEMINFO_MEMAVAILABLE_LINE, value);
}

/*
 * Returns the total size of the memory on the system in bytes based on the
 * the information in `/proc/meminfo`.
 */
LTTNG_HIDDEN
int utils_get_memory_total(size_t *value)
{
	return read_proc_meminfo_field(PROC_MEMINFO_MEMTOTAL_LINE, value);
}
