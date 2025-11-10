/* ***** BEGIN LICENSE BLOCK *****
*   Copyright (C) 2012-2016, Peter Hatina <phatina@gmail.com>
*
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License as
*   published by the Free Software Foundation; either version 2 of
*   the License, or (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <http://www.gnu.org/licenses/>.
* ***** END LICENSE BLOCK ***** */

#ifndef SIMPLE_MTPFS_UTIL
#define SIMPLE_MTPFS_UTIL

#include <config.h>
#include <stdint.h>
#include <inttypes.h>
#include <cstdint>
#include <string>
#include <regex>
#include <sys/ioctl.h>
#include <linux/limits.h>

#ifdef HAVE_LIBUSB1
#  include <libmtp.h>
#endif // HAVE_LIBUSB1

#include "simple-mtpfs.h"

// typedef char int8_t;
// typedef unsigned char uint8_t;
// typedef short int16_t;
// typedef unsigned short uint16_t;
// typedef int int32_t;
// typedef unsigned int uint32_t;
// typedef unsigned long long uint64_t;

std::string getVersion();

class StreamHelper
{
public:
    StreamHelper() = delete;

    static void on();
    static void off();

private:
    static bool s_enabled;
    static int s_stdout;
    static int s_stderr;
};

std::string smtpfs_dirname(const std::string &path);
std::string smtpfs_basename(const std::string &path);
std::string smtpfs_realpath(const std::string &path);
std::string smtpfs_get_tmpdir();

bool smtpfs_create_dir(const std::string &dirname);
bool smtpfs_remove_dir(const std::string &dirname);
bool smtpfs_check_dir(const std::string &path);
bool smtpfs_usb_devpath(const std::string &path, uint8_t *bnum, uint8_t *dnum);

#ifdef HAVE_LIBUSB1
LIBMTP_raw_device_t *smtpfs_raw_device_new(const std::string &path);
void smtpfs_raw_device_free(LIBMTP_raw_device_t *device);
bool smtpfs_reset_device(LIBMTP_raw_device_t *device);
#endif // HAVE_LIBUSB1

bool endWith(const std::string& str, const std::string& substr);
// bool isAudioType(const char *fileName);
// bool isVideoType(const char *fileName);
// bool isImageType(const char *fileName);
// bool isPlayListType(const char *fileName);
// bool isAudioBooksType(const char *fileName);
// bool isMediaType(const char *fileName);
// bool isMatchExtName(const std::string &name, const std::regex& re);
bool makePatternStrArray(std::string &pattern, const std::vector<std::string> &strVec);
bool makeRegEx(std::regex &re, const std::string &pattern);
bool makeRegExStrArray(std::regex &re, const std::vector<std::string> &strVec);
bool reMatch(const std::string &str, const std::regex &re);
bool reSearch(const std::string &str, const std::regex &re);

inline long getSysRunTime()
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1000L +
         ts.tv_nsec / 1000000L;
}

int setThreadPriority(int32_t policy, int32_t priority);

#endif // SIMPLE_MTPFS_UTIL
