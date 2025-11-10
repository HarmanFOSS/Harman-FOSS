#include "simple-mtpfs-db.hpp"
#include "simple-mtpfs-log.h"

using namespace std;

static int sqliteBusyHandler(void *p, int retryTimes)
{
    LogDebug("retryTimes: %d", retryTimes);
    sqlite3_sleep(50);
    return 1;
}

sqlite3 *SIMPLE_DB::doConnect( const string &path )
{
   int32_t status = 0;
   sqlite3 *db = NULL;

   if ( path.empty()) 
   {
      return NULL;
   }

   status = sqlite3_open( path.c_str(), &db );
   if ( SQLITE_OK != status ) 
   {
      sqlite3_close(db);
      return NULL;
   }

   sqlite3_busy_handler(db, sqliteBusyHandler, (void *)db);

   return db;
}

int queryDeviceCb(void* userData, int argc, char** argv, char** col)
{
   uint32_t *fileId = (uint32_t *)userData;
   int32_t i = 0;

   for(i = 0; i < argc; i++)
   {
      LogDump("%s = %s", col[i], argv[i] ? argv[i] : "NULL");
      if(argv[i])
      {
         *fileId = atoi(argv[i]);
      }
   }

   return 0;
}

int32_t SIMPLE_DB::doCommit(const string &sqlStr, DBQueryCallback cb, void *userData, uint32_t retryTime)
{
   int32_t ret = 0;
   
   if(!mPsqlite3)
   {
      LogError(" mPsqlite3 is NULL");
      return -1;
   }

   LogDebug("exe sqlStr: %s\n", sqlStr.c_str());

   ret = sqlite3_exec(mPsqlite3, sqlStr.c_str(), cb, userData, NULL);

   LogDebug("exe ret: %d\n", ret);
   return ret;
}

SIMPLE_DB::SIMPLE_DB(const string DBName)
   : mPsqlite3(NULL)
{
   string databaseName = DBName;
   //get the SMTPFS_DB_PATH by environment 
   char *dbPath = getenv(SMTPFS_DBPATH_ENV);

   if(dbPath != NULL)
   {
      databaseName = dbPath;
      LogInfo("env SMTPFS_DBPATH:%s", dbPath);
   }
   
   LogInfo("real dbbase path: %s", databaseName.c_str());

   if(!databaseName.empty())
   {
      mPsqlite3 = doConnect(databaseName);
   }
}

SIMPLE_DB::~SIMPLE_DB()
{
    if(mPsqlite3)
    {
        sqlite3_close(mPsqlite3);
    }
}

int32_t SIMPLE_DB::queryDevice(string &uuid)
{
   char *zSQL = NULL;
   int ret = 0;
   int exist = 0;

   if(!mPsqlite3)
   {
      LogError("mPsqlite3 is NULL");
      return -1;
   }

   zSQL = sqlite3_mprintf("select uuid from mtpdevices where uuid = '%q'", uuid.c_str());

   ret = doCommit(zSQL, queryDeviceCb, &exist, 0);
   if(ret)
   {
      LogError("doCommit failed zSQL: %s", zSQL);
   }
   
   if(zSQL)
   {
      sqlite3_free(zSQL);
   }

   if(exist)
   {
      return 0;
   }
   
   return -1;
}

int queryDeviceCountCb(void* userData, int argc, char** argv, char** col)
{
   int32_t *count = (int32_t *)userData;
   int32_t i = 0;

   for(i = 0; i < argc; i++)
   {
      LogDump("%s = %s", col[i], argv[i] ? argv[i] : "NULL");
      *count = atoi(argv[i]);
   }

   return 0;
}

int32_t SIMPLE_DB::getDeviceCount(int32_t *count)
{
   string sqlFmt;

   if(!mPsqlite3)
   {
      LogError("mPsqlite3 is NULL");
      return -1;
   }

   if(!count)
   {
      LogError("cout is NULL");
      return -1;
   }

   sqlFmt = "select count(*) from mtpdevices;";

   LogInfo("sql:", sqlFmt.c_str());

   if(doCommit(sqlFmt, queryDeviceCountCb, count, 0))
   {
      LogError("doCommit failed: %s", sqlFmt.c_str());
      return -1;
   }
   
   return 0;
}


int queryOldestDeviceCb(void* userData, int argc, char** argv, char** col)
{
   char *uuid = (char *)userData;
   int32_t i = 0;

   for(i = 0; i < argc; i++)
   {
      LogDump("%s = %s", col[i], argv[i] ? argv[i] : "NULL");

      if(argv[i])
      {
         memcpy(uuid, argv[i], strlen(argv[i]));
      }
   }

   return 0;
}

int32_t SIMPLE_DB::getOldestDevice(const char *uuid)
{
   string sqlFmt;

   if(!mPsqlite3)
   {
      LogError("mPsqlite3 is NULL");
      return -1;
   }

   if(!uuid)
   {
      LogError("uuid is NULL");
      return -1;
   }

   sqlFmt = "SELECT UUID FROM MTPDEVICES ORDER BY MTIME ASC LIMIT 0,1;";

   LogInfo("sqlFmt:%s", sqlFmt.c_str());

   if(doCommit(sqlFmt, queryOldestDeviceCb, (void *)uuid, 0))
   {
      LogError("doCommit failed sqlstr: %s", sqlFmt.c_str());
      return -1;
   }

   return 0;
}

int32_t SIMPLE_DB::checkDeviceCount()
{
   //firset check device count, delete more MAX_MTPDEVICE_COUNT device
   int32_t devCount = 0;
   getDeviceCount(&devCount);

   LogInfo("devCount:%d", devCount);
   
   if(devCount >= MAX_MTPDEVICE_COUNT)
   {
      //get the device uuid with the oldest time
      char myUUID[256] = {""};
      getOldestDevice(myUUID);

      LogInfo("myUUID:%s", myUUID);

      //delete the device
      string tmpUUID = myUUID;
      deleteDevice(tmpUUID);

      //delete all the files for the device
      deleteFileID(tmpUUID);
   }

   return 0;
}

int32_t SIMPLE_DB::writeDevice(string &uuid)
{
   int32_t ret = 0;
   char *zSQL = NULL;

   ret = queryDevice(uuid);
   //insert
   if(ret)
   { 
      checkDeviceCount();
      
      zSQL = sqlite3_mprintf("insert into mtpdevices values ('%q', '%d')", uuid.c_str(), time(NULL));
   }
   //update
   else
   {   
      zSQL = sqlite3_mprintf("update mtpdevices set mtime = '%d' where uuid = '%q'", time(NULL), uuid.c_str());
   }
   
   ret = doCommit(zSQL, NULL, NULL, 0);
   if(ret)
   {
      LogError("doCommit failed: %s", zSQL);
   }
   
   if(zSQL)
   {
      sqlite3_free(zSQL);
   }

   return ret;
}

int32_t SIMPLE_DB::deleteDevice(string &uuid)
{
   int32_t ret = 0;
   char *zSQL = NULL;

   if(!mPsqlite3)
   {
      LogError("mPsqlite3 is NULL");
      return -1;
   }
   
   zSQL = sqlite3_mprintf("delete from mtpdevices where uuid = '%q'", uuid.c_str());
   
   ret = doCommit(zSQL, NULL, NULL, 0);
   if(ret)
   {
      LogError("doCommit failed: %s", zSQL);
   }
   
   if(zSQL)
   {
      sqlite3_free(zSQL);
   }

   return ret;;
}


int32_t SIMPLE_DB::queryFileID(string &path, string &uuid, uint32_t *mtpFileId)
{
   int ret = 0;
   char *zSQL = NULL;

   if(!mPsqlite3)
   {
      LogError("mPsqlite3 is NULL");
      return -1;
   }

   if(!mtpFileId)
   {
      LogError("mtpFileId is NULL");
      return -1;
   }

   zSQL = sqlite3_mprintf("select mtpId from mtpfile where path = '%q' and uuid = '%q'", path.c_str(), uuid.c_str());

   //LogInfo("zSQL:%s", zSQL);

   ret = doCommit(zSQL, queryDeviceCb, mtpFileId, 0);
   if(ret)
   {
      LogError("doCommit failed zSQL: %s", zSQL);
   }   
   
   if(zSQL)
   {
      sqlite3_free(zSQL);
   }
   
   return ret;
}

int32_t SIMPLE_DB::writeFileID(string &path, uint32_t fileId, string &uuid)
{
   int32_t ret = 0;
   char *zSQL = NULL;
   uint32_t queryFileId = 0;

   ret = queryFileID(path, uuid, &queryFileId);

   //LogInfo(" writeFileID, queryFileId: %d", queryFileId);
   
   if(!queryFileId)
   {
      zSQL = sqlite3_mprintf("insert into mtpfile values ('%q','%d', '%q')", path.c_str(), fileId, uuid.c_str());
   }
   //update
   else
   {
      zSQL = sqlite3_mprintf("update mtpfile set mtpId = '%d' where path = '%q' and uuid = '%q'", fileId, path.c_str(), uuid.c_str());
   }
   
   ret = doCommit(zSQL, NULL, NULL, 0);
   if(ret)
   {
      LogError("doCommit failed zSQL: %s", zSQL);
   }
   
   if(zSQL)
   {
      sqlite3_free(zSQL);
   }

   return ret;
}

int32_t SIMPLE_DB::deleteFileID(string &uuid)
{
   int ret = 0;
   char *zSQL = NULL;

   if(!mPsqlite3)
   {
      LogError("mPsqlite3 is NULL");
      return -1;
   }

   zSQL = sqlite3_mprintf("delete from mtpfile where uuid = '%q'", uuid.c_str());
   
   ret = doCommit(zSQL, NULL, NULL, 0);
   if(ret)
   {
      LogError("doCommit failed zSQL: %s", zSQL);
   }
   
   if(zSQL)
   {
      sqlite3_free(zSQL);
   }
   
   return ret;
}

#if 0
int main()
{
    SIMPLE_DB smtp_db = SIMPLE_DB(SMTPFSDBPATH);
    
    string str = "111'111";
    string &uuid = str;
    string path = "/tmp/mtp1.0/dir/file'1.mp3";

    smtp_db.writeDevice(uuid);
    smtp_db.writeFileID(path, 1, uuid);

    str = "222'222";
    path = "/tmp/mtp1.0/dir/file'2.mp3";
    smtp_db.writeDevice(uuid);
    smtp_db.writeFileID(path, 2, uuid);

    str = "333'333";
    path = "/tmp/mtp1.0/dir/file'3.mp3";
    smtp_db.writeDevice(uuid);
    smtp_db.writeFileID(path, 3, uuid);
    
    str = "444'444";
    path = "/tmp/mtp1.0/dir/file'4.mp3";
    smtp_db.writeDevice(uuid);
    smtp_db.writeFileID(path, 4, uuid);

    return 0;
}
#endif
