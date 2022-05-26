/*
 * Copyright (C) 2019 - Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _COMPAT_DIRECTORY_HANDLE_H
#define _COMPAT_DIRECTORY_HANDLE_H

#include <common/macros.h>
#include <common/credentials.h>

enum lttng_directory_handle_rmdir_recursive_flags {
	LTTNG_DIRECTORY_HANDLE_FAIL_NON_EMPTY_FLAG = (1U << 0),
	LTTNG_DIRECTORY_HANDLE_SKIP_NON_EMPTY_FLAG = (1U << 1),
};

/*
 * Some platforms, such as Solaris 10, do not support directory file descriptors
 * and their associated functions (*at(...)), which are defined in POSIX.2008.
 *
 * This wrapper provides a handle that is either a copy of a directory's path
 * or a directory file descriptors, depending on the platform's capabilities.
 */
#ifdef COMPAT_DIRFD
struct lttng_directory_handle {
	int dirfd;
};

static inline
int lttng_directory_handle_get_dirfd(
		const struct lttng_directory_handle *handle)
{
	return handle->dirfd;
}

#else
struct lttng_directory_handle {
	char *base_path;
};
#endif

/*
 * Initialize a directory handle to the provided path. Passing a NULL path
 * returns a handle to the current working directory.
 *
 * An initialized directory handle must be finalized using
 * lttng_directory_handle_fini().
 */
LTTNG_HIDDEN
int lttng_directory_handle_init(struct lttng_directory_handle *handle,
		const char *path);

/*
 * Initialize a new directory handle to a path relative to an existing handle.
 *
 * The provided path must already exist. Note that the creation of a
 * subdirectory and the creation of a handle are kept as separate operations
 * to highlight the fact that there is an inherent race between the creation of
 * a directory and the creation of a handle to it.
 *
 * Passing a NULL path effectively copies the original handle.
 *
 * An initialized directory handle must be finalized using
 * lttng_directory_handle_fini().
 */
LTTNG_HIDDEN
int lttng_directory_handle_init_from_handle(
		struct lttng_directory_handle *new_handle,
		const char *path,
		const struct lttng_directory_handle *handle);

/*
 * Initialize a new directory handle from an existing directory fd.
 *
 * The new directory handle assumes the ownership of the directory fd.
 * Note that this method should only be used in very specific cases, such as
 * re-creating a directory handle from a dirfd passed over a unix socket.
 *
 * An initialized directory handle must be finalized using
 * lttng_directory_handle_fini().
 */
LTTNG_HIDDEN
int lttng_directory_handle_init_from_dirfd(
		struct lttng_directory_handle *handle, int dirfd);

/*
 * Copy a directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_copy(const struct lttng_directory_handle *handle,
		struct lttng_directory_handle *new_copy);

/*
 * Move a directory handle. The original directory handle may no longer be
 * used after this call. This call cannot fail; directly assign the
 * return value to the new directory handle.
 *
 * It is safe (but unnecessary) to call lttng_directory_handle_fini on the
 * original handle.
 */
LTTNG_HIDDEN
struct lttng_directory_handle
lttng_directory_handle_move(struct lttng_directory_handle *original);

/*
 * Release the resources of a directory handle.
 */
LTTNG_HIDDEN
void lttng_directory_handle_fini(struct lttng_directory_handle *handle);

/*
 * Create a subdirectory relative to a directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_create_subdirectory(
		const struct lttng_directory_handle *handle,
		const char *subdirectory,
		mode_t mode);

/*
 * Create a subdirectory relative to a directory handle
 * as a given user.
 */
LTTNG_HIDDEN
int lttng_directory_handle_create_subdirectory_as_user(
		const struct lttng_directory_handle *handle,
		const char *subdirectory,
		mode_t mode, const struct lttng_credentials *creds);

/*
 * Recursively create a directory relative to a directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_create_subdirectory_recursive(
		const struct lttng_directory_handle *handle,
		const char *subdirectory_path,
		mode_t mode);

/*
 * Recursively create a directory relative to a directory handle
 * as a given user.
 */
LTTNG_HIDDEN
int lttng_directory_handle_create_subdirectory_recursive_as_user(
		const struct lttng_directory_handle *handle,
		const char *subdirectory_path,
		mode_t mode, const struct lttng_credentials *creds);

/*
 * Open a file descriptor to a path relative to a directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_open_file(
		const struct lttng_directory_handle *handle,
		const char *filename,
		int flags, mode_t mode);

/*
 * Open a file descriptor to a path relative to a directory handle
 * as a given user.
 */
LTTNG_HIDDEN
int lttng_directory_handle_open_file_as_user(
		const struct lttng_directory_handle *handle,
		const char *filename,
		int flags, mode_t mode,
		const struct lttng_credentials *creds);

/*
 * Unlink a file to a path relative to a directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_unlink_file(
		const struct lttng_directory_handle *handle,
		const char *filename);

/*
 * Unlink a file to a path relative to a directory handle as a given user.
 */
LTTNG_HIDDEN
int lttng_directory_handle_unlink_file_as_user(
		const struct lttng_directory_handle *handle,
		const char *filename,
		const struct lttng_credentials *creds);

/*
 * Rename a file from a path relative to a directory handle to a new
 * name relative to another directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_rename(
		const struct lttng_directory_handle *old_handle,
		const char *old_name,
		const struct lttng_directory_handle *new_handle,
		const char *new_name);

/*
 * Rename a file from a path relative to a directory handle to a new
 * name relative to another directory handle as a given user.
 */
LTTNG_HIDDEN
int lttng_directory_handle_rename_as_user(
		const struct lttng_directory_handle *old_handle,
		const char *old_name,
		const struct lttng_directory_handle *new_handle,
		const char *new_name,
		const struct lttng_credentials *creds);

/*
 * Remove a subdirectory relative to a directory handle.
 */
LTTNG_HIDDEN
int lttng_directory_handle_remove_subdirectory(
		const struct lttng_directory_handle *handle,
		const char *name);

/*
 * Remove a subdirectory relative to a directory handle as a given user.
 */
LTTNG_HIDDEN
int lttng_directory_handle_remove_subdirectory_as_user(
		const struct lttng_directory_handle *handle,
		const char *name,
		const struct lttng_credentials *creds);

/*
 * Remove a subdirectory and remove its contents if it only
 * consists in empty directories.
 * @flags: enum lttng_directory_handle_rmdir_recursive_flags
 */
LTTNG_HIDDEN
int lttng_directory_handle_remove_subdirectory_recursive(
		const struct lttng_directory_handle *handle,
		const char *name, int flags);

/*
 * Remove a subdirectory and remove its contents if it only
 * consists in empty directories as a given user.
 * @flags: enum lttng_directory_handle_rmdir_recursive_flags
 */
LTTNG_HIDDEN
int lttng_directory_handle_remove_subdirectory_recursive_as_user(
		const struct lttng_directory_handle *handle,
		const char *name,
		const struct lttng_credentials *creds,
		int flags);

#endif /* _COMPAT_PATH_HANDLE_H */
