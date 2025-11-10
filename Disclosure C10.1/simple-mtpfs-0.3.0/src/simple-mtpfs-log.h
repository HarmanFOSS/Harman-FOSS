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

#ifndef SMTPFS_LOG_H
#define SMTPFS_LOG_H

#include <iostream>
#include <iomanip>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <dlt/dlt.h>
#include <libmtp.h>
class Logger;

using namespace std;

#pragma GCC system_header

#define gettid() syscall(__NR_gettid)

#define DLT_LOG_ENABLE 1

#if DLT_LOG_ENABLE
#define  LogDump(format,...) do{if(Logger::getLogLevel() <= Logger::SMTP_LOG_LEVEL_VERBOSE) {LIBMTP_DLT_log(DLT_LOG_DEFAULT,format,##__VA_ARGS__);}} while(0)
#define  LogDebug(format,...) do{if(Logger::getLogLevel() <= Logger::SMTP_LOG_LEVEL_DEBUG) {LIBMTP_DLT_log(DLT_LOG_DEBUG,format,##__VA_ARGS__);}} while(0)
#define  LogInfo(format,...) do{if(Logger::getLogLevel() <= Logger::SMTP_LOG_LEVEL_INFO) {LIBMTP_DLT_log(DLT_LOG_INFO,format,##__VA_ARGS__);}} while(0)
#define  LogWarning(format,...) do{if(Logger::getLogLevel() <= Logger::SMTP_LOG_LEVEL_WARNING) {LIBMTP_DLT_log(DLT_LOG_WARN,format,##__VA_ARGS__);}} while(0)
#define  LogError(format,...) do{if(Logger::getLogLevel() <= Logger::SMTP_LOG_LEVEL_ERROR) {LIBMTP_DLT_log(DLT_LOG_ERROR,format,##__VA_ARGS__);}} while(0)
#else
#define  LogDump(format, ...) Logger::Log(format, Logger::SMTP_LOG_LEVEL_VERBOSE, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define  LogDebug(format, ...) Logger::Log(format, Logger::SMTP_LOG_LEVEL_DEBUG, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define  LogInfo(format, ...) Logger::Log(format, Logger::SMTP_LOG_LEVEL_INFO, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define  LogWarning(format, ...) Logger::Log(format, Logger::SMTP_LOG_LEVEL_WARNING, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define  LogError(format, ...) Logger::Log(format, Logger::SMTP_LOG_LEVEL_ERROR, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif
class Logger
{
public:
    typedef enum {
        SMTP_LOG_LEVEL_VERBOSE = 0,
        SMTP_LOG_LEVEL_DEBUG,
        SMTP_LOG_LEVEL_INFO,
        SMTP_LOG_LEVEL_WARNING,
        SMTP_LOG_LEVEL_ERROR,
        SMTP_LOG_LEVEL_OFF
    } SMTP_LOG_LEVEL;

public:
    static void init();
    static void setLogLevel(const std::string &logLevel);
    static int getLogLevel();
    static void Log(const char *format, SMTP_LOG_LEVEL logLevel, const char* funcName, int line, ...);
    static bool isEnableLibmtpLog();
    static void off();
    static void on();

private:
    static std::string timestamp();
    //static long getSysRunTime();
    static string toString(SMTP_LOG_LEVEL level);

private: 
    static bool m_isInited;
    static SMTP_LOG_LEVEL m_logLevel;
    //static string m_logPath;
    //static int m_stdout;
};

#endif // SMTPFS_LOG_H
