/******************************************************************************
 * Project         Harman Car Multimedia System
 * (c) copyright   2017-2020
 * Company         Harman Automotive Systems
 *                 All rights reserved
 * Secrecy Level   STRICTLY CONFIDENTIAL
 ******************************************************************************/
/**
 * @file           simple-mtpfs-config.cpp
 * @author         Xiaojun.Zou@harman.com
 * @brief          the config for simple-mtpfs
 */


#include <pthread.h>

#include <algorithm>

#include "simple-mtpfs.h"
#include "simple-mtpfs-config.h"
#include "simple-mtpfs-log.h"


Config::Config()
: mScanThrPolicy(SCHED_OTHER)
, mScanThrPriority(0)
, mExcludePattern()
, mEnableCacheFirstFile(false)
, mTmpPath()
, mTmpSizeLimit(0)
{}

Config::~Config()
{}

Config& Config::instance()
{
   static Config inst;

   return inst;
}

int Config::setConfig(uint32_t type, const std::string& value)
{
   std::string valueLower = value;

   LogInfo("config type: %x; config value: %s", type, value.c_str());
   std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);
   switch (type)
   {
      case SMTPFS_CONFIG_THREAD_POLICY:
      {
         if (!value.empty())
         {
            mScanThrPolicy = atoi(value.c_str());
         }
      }
      break;

      case SMTPFS_CONFIG_THREAD_PRIORITY:
      {
         if (!value.empty())
         {
            mScanThrPriority = atoi(value.c_str());
         }
      }
      break;

      case SMTPFS_CONFIG_EXCLUDE_PATTERN:
      {
         if (!value.empty())
         {
            mExcludePattern = value;
         }
      }
      break;

      case SMTPFS_CONFIG_EXTAUDIO:
      {
         if (!valueLower.empty())
         {
            mExtNameAudio.push_back(valueLower);
         }
      }
      break;

      case SMTPFS_CONFIG_EXTVIDEO:
      {
         if (!valueLower.empty())
         {
            mExtNameVideo.push_back(valueLower);
         }
      }
      break;

      case SMTPFS_CONFIG_EXTIMAGE:
      {
         if (!valueLower.empty())
         {
            mExtNameImage.push_back(valueLower);
         }
      }
      break;

      case SMTPFS_CONFIG_EXTPLAYLIST:
      {
         if (!valueLower.empty())
         {
            mExtNamePlaylist.push_back(valueLower);
         }
      }
      break;

      case SMTPFS_CONFIG_EXTAUDIOBOOK:
      {
         if (!valueLower.empty())
         {
            mExtNameAudiobook.push_back(valueLower);
         }
      }
      break;

      case SMTPFS_CONFIG_LOGLEVEL:
      {
         if (!value.empty())
         {
            mSMTPFSLogLevel = value;
         }
      }
      break;

      case SMTPFS_CONFIG_LIBMTP_LOGLEVEL:
      {
         if (!value.empty())
         {
            mLibMTPLogLevel = value;
         }
      }
      break;

      case SMTPFS_CONFIG_CACHEFIRSTFILE:
      {
         mEnableCacheFirstFile = true;
      }
      break;

      case SMTPFS_CONFIG_TMPPATH:
      {
         if (!value.empty())
         {
            mTmpPath = value;
         }
      }
      break;

      case SMTPFS_CONFIG_TMPSIZELIMIT:
      {
         if (!value.empty())
         {
            mTmpSizeLimit = atoi(value.c_str());
         }
      }
      break;

      default:
      {
         LogError("Unsupported config type: %x; value: %s", type, value.c_str());
         return -1;
      }
      break;
   }

   return 0;
}