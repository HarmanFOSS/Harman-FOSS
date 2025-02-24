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

#include <config.h>
#include <sstream>
#include <ctime>
#include "simple-mtpfs-log.h"
#include "simple-mtpfs-util.h"

bool Logger::m_isInited = false;
Logger::SMTP_LOG_LEVEL Logger::m_logLevel = SMTP_LOG_LEVEL_INFO;
//string Logger::m_logPath = "/tmp/smtp.log";
//int Logger::m_stdout = -1;

void Logger::init()
{
    if (m_isInited) {
        return;
    }
    //m_stdout = dup(fileno(stdout));
    //freopen(m_logPath.c_str(), "w", stdout);

    /* before smtp running, we can set log level by set the environment variable  SMTP_LOG_LEVEL */
    const char *logLevelEnv = getenv("SMTP_LOG_LEVEL");

   if (logLevelEnv)
   {
      setLogLevel(logLevelEnv);
   }

    /* -- set libmtp log level start --
    Setting libmtp debug level according to the SMTP_LOG_LEVEL, libmtp log and smtp log will output to one log file
    SMTP_LOG_LEVEL_INFO will enable libmtp basic log //basic log is not controlled by the LIBMTP_DEBUG env, it's controlled by Logger::isEnableLibmtpLog()
    SMTP_LOG_LEVEL_DEBUG will enable basic log and PTP/PLAYLIST/USB debug log
    SMTP_LOG_LEVEL_VERBOSE will enable basic log, PTP/PLAYLIST/USB debug log and usb data dump
    other SMTP_LOG_LEVEL will disable libmtp debug log */
    int ret = -1;
    if (m_logLevel == SMTP_LOG_LEVEL_DEBUG) {
        ret = setenv("LIBMTP_DEBUG", "7", 1);
    } else if (m_logLevel == SMTP_LOG_LEVEL_VERBOSE) {
        ret = setenv("LIBMTP_DEBUG", "9", 1);
    } else {
        ret = setenv("LIBMTP_DEBUG", "0", 1);
    }
    if (ret != 0) {
        LogError("failed to set libmtp debug env");
    }

    const char *libmtpenv = getenv("LIBMTP_DEBUG");
    LogInfo("get setted libmtp env:%s", libmtpenv);
    /* -- set libmtp log level end -- */

    m_isInited = true;
    
    LogInfo("logger initialized, loglevel is:%s", toString(m_logLevel).c_str());
}

int Logger::getLogLevel()
{
    return m_logLevel;
}

void Logger::setLogLevel(const std::string &logLevel)
{
   // FIXME. it's not a good idea to set log level for simple-mtpfs, the log level should read config while init.
   // And, it need a libmtp interface to set log level of libmtp.
   if (!logLevel.empty())
   {
      if (logLevel == "VERBOSE")
         m_logLevel = SMTP_LOG_LEVEL_VERBOSE;
      else if (logLevel == "DEBUG")
         m_logLevel = SMTP_LOG_LEVEL_DEBUG;
      else if (logLevel == "INFO")
         m_logLevel = SMTP_LOG_LEVEL_INFO;
      else if (logLevel == "WARNING")
         m_logLevel = SMTP_LOG_LEVEL_WARNING;
      else if (logLevel == "ERROR")
         m_logLevel = SMTP_LOG_LEVEL_ERROR;
      else if (logLevel == "OFF")
         m_logLevel = SMTP_LOG_LEVEL_OFF;
      else
         m_logLevel = SMTP_LOG_LEVEL_INFO;
   }
   else
   {
      m_logLevel = SMTP_LOG_LEVEL_INFO;
   }
   LogInfo("logger set log level to: %s", toString(m_logLevel).c_str());
}

void Logger::Log(const char *format, SMTP_LOG_LEVEL logLevel, const char* funcName, int line, ...)
{
    if (m_logLevel > logLevel)
        return;
#if 0
    std::string logTag;
    if (logLevel == SMTP_LOG_LEVEL_VERBOSE) {
        logTag = "SMTP DUMP ";
    } else if (logLevel == SMTP_LOG_LEVEL_DEBUG) {
        logTag = "SMTP DEBUG ";
    } else if (logLevel == SMTP_LOG_LEVEL_INFO) {
        logTag = "SMTP INFO ";
    } else if (logLevel == SMTP_LOG_LEVEL_WARNING) {
        logTag = "SMTP WARN ";
    } else if (logLevel == SMTP_LOG_LEVEL_ERROR) {
        logTag = "SMTP ERROR ";
    } else {
        logTag = "SMTP LOG ";
    }
    
    va_list args;
    va_start(args, line);
        
    freopen(m_logPath.c_str(), "a", stdout);
    stringstream appendFormat;
    appendFormat << logTag
    // << timestamp() << " " 
    << getSysRunTime()/1000 << "." << std::setw(3) << getSysRunTime()%1000
    << "s T" << gettid() 
    << " " << funcName 
    << "[" << line <<"] " 
    << format << "\n";
    vfprintf(stdout, appendFormat.str().c_str(), args); 
    fflush(stdout);

    va_end(args);
#endif
}

bool Logger::isEnableLibmtpLog() 
{
    if (m_logLevel == SMTP_LOG_LEVEL_VERBOSE || 
        m_logLevel == SMTP_LOG_LEVEL_DEBUG ||
        m_logLevel == SMTP_LOG_LEVEL_INFO) {
        return true;
    } else {
        return false;
    }
}

std::string Logger::timestamp()
{
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    std::stringstream ss;
    ss  << std::setfill('0') << std::setw(4) << time_info->tm_year + 1900
        << '/' << std::setw(2) << time_info->tm_mon + 1
        << '/' << std::setw(2) << time_info->tm_mday
        << ' ' << std::setw(2) << time_info->tm_hour
        << ':' << std::setw(2) << time_info->tm_min
        << ':' << std::setw(2) << time_info->tm_sec;
    return ss.str();
}

void Logger::off()
{
    //dup2(m_stdout, fileno(stdout));
    //close(m_stdout);
    //setvbuf(stdout, NULL, _IOLBF, 0);
}

void Logger::on()
{
    //fflush(stdout);
    //m_stdout = dup(fileno(stdout));
    //freopen(m_logPath.c_str(), "a", stdout);
}


string Logger::toString(SMTP_LOG_LEVEL level){
    switch (level) {
        case SMTP_LOG_LEVEL_VERBOSE:
            return "SMTP_LOG_LEVEL_VERBOSE";
        case SMTP_LOG_LEVEL_DEBUG:
            return "SMTP_LOG_LEVEL_DEBUG";
        case SMTP_LOG_LEVEL_INFO:
            return "SMTP_LOG_LEVEL_INFO";
        case SMTP_LOG_LEVEL_ERROR:
            return "SMTP_LOG_LEVEL_ERROR";
        case SMTP_LOG_LEVEL_OFF:
            return "SMTP_LOG_LEVEL_OFF";
        default:
            return "SMTP_LOG_LEVEL_UNKNOWN";
    }
}


