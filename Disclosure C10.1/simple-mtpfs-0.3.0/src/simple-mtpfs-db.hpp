/******************************************************************************
 * Project         Harman Car Multimedia System
 * (c) copyright   2017-2020
 * Company         Harman Automotive Systems
 *                 All rights reserved
 * Secrecy Level   STRICTLY CONFIDENTIAL
 ******************************************************************************/
/**
 * @file           simple-mtpfs-db.hpp
 * @author         Jing.Deng@harman.com
 * @brief          the database class for simple-mtpfs
 */

#ifndef SIMPLE_MTPFS_DB_HPP
#define SIMPLE_MTPFS_DB_HPP

//header files
#include <string>
#include <cstdlib>
#include <sqlite3.h>
#include <cstdio>
#include <sstream>
#include <cstring>
#include <unistd.h>

typedef unsigned int uint32_t;
typedef int int32_t;
typedef int (*DBQueryCallback)(void*, int, char**, char**);

//default path of mtp stack db is /tmp/media/smtpfs.db
//if you want to change it, you can config the environment SMTPFS_DB_PATH=xxxx
#define SMTPFSDBPATH "/tmp/media/smtpfs.db"
//smtpfs database env
#define SMTPFS_DBPATH_ENV "SMTPFS_DBPATH"

#define MAX_MTPDEVICE_COUNT (5)

using namespace std;

class SIMPLE_DB
{
public:
   SIMPLE_DB(const string DBName = SMTPFSDBPATH);
   ~SIMPLE_DB();

   int32_t queryDevice(string &uuid);
   int32_t writeDevice(string &uuid);
   int32_t deleteDevice(string &uuid);
   int32_t getDeviceCount(int32_t *count);
   int32_t getOldestDevice(const char *uuid);
   int32_t checkDeviceCount();

   int32_t queryFileID(string &path, string &uuid, uint32_t *mtpFileId);
   int32_t writeFileID(string &path, uint32_t fileId, string &uuid);
   int32_t deleteFileID(string &uuid);

private:
   sqlite3 *mPsqlite3;

private:
   sqlite3 *doConnect( const string &url);
   int32_t doCommit(const string &sqlStr, DBQueryCallback cb, void *data, uint32_t retryTime);

};

#endif
