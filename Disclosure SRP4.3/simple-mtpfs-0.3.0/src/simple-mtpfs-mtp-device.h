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

#ifndef SMTPFS_MTP_DEVICE_H
#define SMTPFS_MTP_DEVICE_H

#include <map>
#include <mutex>
#include <stack>
#include <string>
#include <vector>
#include <semaphore.h>
extern "C" {
#include <libmtp.h>
}
#include "simple-mtpfs-type-dir.h"
#include "simple-mtpfs-type-file.h"
#include "simple-mtpfs-db.hpp"
#include "simple-mtpfs-ringbuf.h"
#include "simple-mtpfs-util.h"

//#define MTP_HAS_DB

// definition for SMTPFileSystem
class SMTPFileSystem;

typedef enum
{
  SMTP_ASYNC_CMD_STATUS_INIT = 0, // receive a command and initial status
  SMTP_ASYNC_CMD_STATUS_WAIT,     // put into the queue and waiting to execute
  SMTP_ASYNC_CMD_STATUS_EXECUTE,  // in executing status
  SMTP_ASYNC_CMD_STATUS_FINISH    // command execute finished and should to response to app
} eAsynCmdStatus;

#define SMTP_COMMAND_EXTRA_INFO_MAX_LEN (512)
typedef struct _AsynCommand
{
    int cmdCode;
    eAsynCmdStatus cmdStatus;
    int responseData;
    char extraInfo[SMTP_COMMAND_EXTRA_INFO_MAX_LEN];

    _AsynCommand(int code)
    {
        cmdCode = code;
        cmdStatus = SMTP_ASYNC_CMD_STATUS_INIT;
        responseData = -1;
        memset(extraInfo, 0x0, SMTP_COMMAND_EXTRA_INFO_MAX_LEN);
    }
    
    _AsynCommand(int code, char *info, int len)
    {
        cmdCode = code;
        cmdStatus = SMTP_ASYNC_CMD_STATUS_INIT;
        responseData = -1;
        memset(extraInfo, 0x0, SMTP_COMMAND_EXTRA_INFO_MAX_LEN);
        memcpy(extraInfo, info, len);
    }
} tAsynCommand;

#ifdef MTP_HAS_DB
class MTPDevice : public SIMPLE_DB
#else
class MTPDevice
#endif
{
 public:
   class Capabilities
   {
    public:
      Capabilities()
          : m_get_partial_object(false), m_send_partial_object(false), m_edit_objects(false)
      {
      }

      void setCanGetPartialObject(bool b) { m_get_partial_object = b; }
      void setCanSendPartialobject(bool b) { m_send_partial_object = b; }
      void setCanEditObjects(bool b) { m_edit_objects = b; }

      bool canGetPartialObject() const { return m_get_partial_object; }
      bool canSendPartialObject() const { return m_send_partial_object; }
      bool canEditObjects() const { return m_edit_objects; }

    private:
      bool m_get_partial_object;
      bool m_send_partial_object;
      bool m_edit_objects;
   };

   // -------------------------------------------------------------------------

   MTPDevice(SMTPFileSystem &fs);
   ~MTPDevice();

   bool connect(LIBMTP_raw_device_t *dev);
   bool connect(int dev_no = 0);
   bool connect(const std::string &dev_file);
   void disconnect();

   void enableMove(bool e = true) { m_move_enabled = e; }

   uint64_t storageTotalSize() const;
   uint64_t storageFreeSize() const;

   int dirCreateNew(const std::string &path);
   int dirRemove(const std::string &path);
   int dirRename(const std::string &oldpath, const std::string &newpath);
   const TypeDir *dirFetchContentFromCache(std::string path);

   //the item type: 0 unknown, 1:folder, 2:file
   //#define MTP_ITEMTYPE_UNKOWN (0)
   //#define MTP_ITEMTYPE_FOLDER (1)
   //#define MTP_ITEMTYPE_FILE (2)
   //const TypeDir *dirFetchContent(std::string path, int *itemType);
   //int dirFetchCount(std::string path);

   int rename(const std::string &oldpath, const std::string &newpath);

   int filePull(const std::string &src, const std::string &dst);
   int filePush(const std::string &src, const std::string &dst);
   int fileRemove(const std::string &path);
   int fileRename(const std::string &oldpath, const std::string &newpath);
   int filesplitPull(const std::string &src, const std::string &dst, uint32_t size, uint32_t offset, unsigned char *buf);
   Capabilities getCapabilities() const;

   static bool listDevices(bool verbose, const std::string &dev_file);
   string &getUUID() { return mUUID; }
   int32_t getFileInfoById(uint32_t id, struct stat *st);
   void criticalEnter1() { m_device_mutex.lock(); }
   void criticalLeave1() { m_device_mutex.unlock(); }
   bool isFileExist(const uint32_t id);
   int32_t autoListAll();
   int32_t getTrackList(std::vector<smtpfs_trackInfo> &trackList, int32_t count);
   int32_t getFriendlyName(char *name, uint32_t len);
   int setEvent(char *buf, uint32_t bufLen);
   int getEvent(char *data, uint32_t len);
   int launchTasks();
   int abortTask();
   bool isListAllDone() { return m_list_all_done; };
   bool hasAvailableTrack();
   bool isMediaType(const std::string &path);

 private:
#if 0
    void criticalEnter() { m_device_mutex.lock(); }
    void criticalLeave() { m_device_mutex.unlock(); }
#else
   void criticalEnter()
   {
   }
   void criticalLeave() {}
#endif

   bool enumStorages();
   static Capabilities getCapabilities(const MTPDevice &device);
   bool connect_priv(int dev_no, const std::string &dev_file);
   int getAllFolderAndFiles(LIBMTP_mtpdevice_t *device, LIBMTP_devicestorage_t *storage, const uint32_t id, string &path, const string &lastModePath = "");
   int readByRingBuf(const uint32_t id, const uint64_t fileSize, const uint32_t size, const uint32_t offset, const unsigned char *buf);
   int handleNewFileCache(const std::string &path = "");
   void printTrackInfo(const smtpfs_trackInfo *track);
   int fillTrackInfo(smtpfs_trackInfo *dst, const LIBMTP_track_t *src, const char *path, int type);
   int fillTrackInfoByFile(smtpfs_trackInfo *dst, const LIBMTP_file_t *src, const char *path, int type);
   int getMediaFileType(const std::string &filename);
   int setLastModeSyncPath(string path);
   // for metadata track list
   void clearTrackList();
   void pushTrackInfo(const smtpfs_trackInfo &trackInfo);
   void pushTrackList(const std::vector<smtpfs_trackInfo> &trackList);
   int32_t popTrackList(std::vector<smtpfs_trackInfo> &trackList, int32_t count);

 private:
   // the reference of the owner, SMTPFileSystem
   SMTPFileSystem &mFileSystem;

   LIBMTP_mtpdevice_t *m_device;
   Capabilities m_capabilities;
   std::mutex m_device_mutex;
   TypeDir m_root_dir;
   bool m_move_enabled;
   static uint32_t s_root_node;
   string mUUID;
   int m_list_all_done;
   int m_read_data_cmd_coming;
   RingBuffer *m_rbuf;
   //below variable is used for current playing file
   string m_fileName;
   uint32_t m_fileId;
   uint64_t m_fileSize;
   uint64_t m_fileOffset;

   std::vector<smtpfs_trackInfo> m_tmpTrackMetaList;
   //for event notify, make it be thread-safty
   std::mutex m_mutexTrackList;
   uint32_t m_trackList_offset;
   std::vector<smtpfs_trackInfo> m_trackMetaList;

   // for thread flag
   bool scanThreadRun;
   //scan track done
   bool isIndexingDone;
   // abort MTP scanning
   bool mScanAbort;
   //last mode path
   string m_lastmode_path;

   //lastmode sync done
   bool m_lastMode_sync_done;

   // need cache the scanned file
   bool mNeedCacheFile;
   tAsynCommand mCacheNewTrackCmd;

   int32_t mScanThPolicy;
   int32_t mScanThPriority;
   std::map<std::string, std::regex> mReMapExclude;
   std::regex mReExtNameAudio;
   std::regex mReExtNameVideo;
   std::regex mReExtNameImage;
   std::regex mReExtNamePlaylist;
   std::regex mReExtNameAudiobook;

   // performance statistics
   uint32_t mFolderIgnored;
   uint32_t mFileIgnored;

 public:
   //for device connected
   sem_t device_connect_sem;
};

#endif // SMTPFS_MTP_DEVICE_H
