/******************************************************************************
 * Project         Harman Car Multimedia System
 * (c) copyright   2017-2020
 * Company         Harman Automotive Systems
 *                 All rights reserved
 * Secrecy Level   STRICTLY CONFIDENTIAL
 ******************************************************************************/
/**
 * @file           simple-mtpfs-config.h
 * @author         Xiaojun.Zou@harman.com
 * @brief          the config for simple-mtpfs
 */

#ifndef SMTPFS_CONFIG_H
#define SMTPFS_CONFIG_H

#include <string>
#include <vector>


//  config from MediaOne
class Config
{
public:
   Config();
   ~Config();

   static Config& instance();

   //  set config from MediaOne.
   int setConfig(uint32_t type, const std::string& value);

   int32_t getScanThrPolicy();
   int32_t getScanThrPriority();
   std::string getExcPattern();
   std::vector<std::string> getExtNameAudio();
   std::vector<std::string> getExtNameVideo();
   std::vector<std::string> getExtNameImage();
   std::vector<std::string> getExtNamePlaylist();
   std::vector<std::string> getExtNameAudiobook();
   std::string getSMTPFSLogLevel();
   std::string getLibMTPLogLevel();
   bool getEnableCacheFirstFile();
   std::string getTmpPath();
   int32_t getTmpSizeLimit();

private:
   int32_t mScanThrPolicy;
   int32_t mScanThrPriority;
   std::string mExcludePattern;
   std::vector<std::string> mExtNameAudio;
   std::vector<std::string> mExtNameVideo;
   std::vector<std::string> mExtNameImage;
   std::vector<std::string> mExtNamePlaylist;
   std::vector<std::string> mExtNameAudiobook;
   std::string mSMTPFSLogLevel;
   std::string mLibMTPLogLevel;
   bool mEnableCacheFirstFile;
   std::string mTmpPath;
   int32_t mTmpSizeLimit;
};

inline int32_t Config::getScanThrPolicy()
{
   return mScanThrPolicy;
}

inline int32_t Config::getScanThrPriority()
{
   return mScanThrPriority;
}

inline std::string Config::getExcPattern()
{
   return mExcludePattern;
}

inline std::vector<std::string> Config::getExtNameAudio()
{
   return mExtNameAudio;
}

inline std::vector<std::string> Config::getExtNameVideo()
{
   return mExtNameVideo;
}

inline std::vector<std::string> Config::getExtNameImage()
{
   return mExtNameImage;
}

inline std::vector<std::string> Config::getExtNamePlaylist()
{
   return mExtNamePlaylist;
}

inline std::vector<std::string> Config::getExtNameAudiobook()
{
   return mExtNameAudiobook;
}

inline std::string Config::getSMTPFSLogLevel()
{
   return mSMTPFSLogLevel;
}

inline std::string Config::getLibMTPLogLevel()
{
   return mLibMTPLogLevel;
}

inline bool Config::getEnableCacheFirstFile()
{
   return mEnableCacheFirstFile;
}

inline std::string Config::getTmpPath()
{
   return mTmpPath;
}

inline int32_t Config::getTmpSizeLimit()
{
   return mTmpSizeLimit;
}

#endif