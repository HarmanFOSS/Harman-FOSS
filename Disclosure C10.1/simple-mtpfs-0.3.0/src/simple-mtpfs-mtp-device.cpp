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
*   along with this program. If not, see <http://www.gnu.org/licenses,.
* ***** END LICENSE BLOCK ***** */

#include <config.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <pthread.h>
extern "C" {
#include <unistd.h>
#include <sys/types.h>
#define _DARWIN_USE_64_BIT_INODE
#include <sys/stat.h>
#include <stdio.h>
}

#include "simple-mtpfs-config.h"
#include "simple-mtpfs-fuse.h"
#include "simple-mtpfs-libmtp.h"
#include "simple-mtpfs-log.h"
#include "simple-mtpfs-mtp-device.h"
#include "simple-mtpfs-util.h"
#include "simple-mtpfs-fuse.h"
#include "simple-mtpfs.h"

// the time out for libmtp request
#define LIBMTP_REQUEST_TIMEOUT                  (500)
#define LIBMTP_REQUEST_TIMEOUT_ABORT            (50000)

uint32_t MTPDevice::s_root_node = ~0;

static LIBMTP_mtpdevice_t *g_device = NULL;
static MTPDevice *gMTPDev = NULL;
#define RINGBUF_SIZE (512 * 1024)

MTPDevice::MTPDevice(SMTPFileSystem &fs) :
#ifdef MTP_HAS_DB
                         SIMPLE_DB(),
#endif
                         mFileSystem(fs),
                         m_device(nullptr),
                         m_capabilities(),
                         m_device_mutex(),
                         m_root_dir(),
                         m_move_enabled(false),
                         mUUID(""),
                         m_list_all_done(0),
                         m_read_data_cmd_coming(0),
                         m_rbuf(NULL),
                         m_fileName(""),
                         m_fileId(0),
                         m_fileSize(0),
                         m_fileOffset(0),
                         m_trackList_offset(0),
                         scanThreadRun(false),
                         isIndexingDone(false),
                         mScanAbort(false),
                         m_lastMode_sync_done(false),
                         mNeedCacheFile(false),
                         mCacheNewTrackCmd(tAsynCommand(SMTPFS_EVENT_CACHE_NEW_TRACK)),
                         mFolderIgnored(0),
                         mFileIgnored(0)
{
   gMTPDev = this;
   sem_init(&device_connect_sem, 0, 0);

   StreamHelper::off();
   LIBMTP_Init();
   StreamHelper::on();
}

MTPDevice::~MTPDevice()
{
   disconnect();
   gMTPDev = NULL;
   sem_destroy(&device_connect_sem);
   scanThreadRun = false;
   isIndexingDone = false;
   mScanAbort = true;
   m_lastMode_sync_done = false;
}

bool MTPDevice::connect(LIBMTP_raw_device_t *dev)
{
   if (m_device)
   {
      LogInfo("Already connected");
      return true;
   }

   // Do not output LIBMTP debug stuff
   StreamHelper::off();
   m_device = LIBMTP_Open_Raw_Device_Uncached(dev);
   StreamHelper::on();

   if (!m_device)
   {
      LIBMTP_Dump_Errorstack(m_device);
      return false;
   }

   if (!enumStorages())
      return false;

   // Retrieve capabilities.
   m_capabilities = MTPDevice::getCapabilities(*this);

   g_device = m_device;

   char tmpStr[128] = "";

   LogInfo("vendor_id: %x, product_id: %x", dev->device_entry.vendor_id, dev->device_entry.product_id);

   //sprintf(tmpStr, "%x_%x_", dev->device_entry.vendor_id, dev->device_entry.product_id);

   string seriaID = LIBMTP_Get_Serialnumber(m_device);

   LogInfo("seriaID: %s", seriaID.c_str());

   mUUID = tmpStr + seriaID;

#ifdef MTP_HAS_DB
   writeDevice(mUUID);
#endif
   LogInfo("connected");
   return true;
}

bool MTPDevice::connect_priv(int dev_no, const std::string &dev_file)
{
   if (m_device)
   {
      LogInfo("Already connected");
      return true;
   }

   int raw_devices_cnt;
   LIBMTP_raw_device_t *raw_devices;

   // Do not output LIBMTP debug stuff
   StreamHelper::off();
   LIBMTP_error_number_t err = LIBMTP_Detect_Raw_Devices(
       &raw_devices, &raw_devices_cnt);
   StreamHelper::on();

   if (err != LIBMTP_ERROR_NONE)
   {
      switch (err)
      {
      case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
         LogError("No raw devices found.");
         break;
      case LIBMTP_ERROR_CONNECTING:
         LogError("There has been an error connecting. Exiting.");
         break;
      case LIBMTP_ERROR_MEMORY_ALLOCATION:
         LogError("Encountered a Memory Allocation Error. Exiting.");
         break;
      case LIBMTP_ERROR_GENERAL:
         LogError("General error occured. Exiting.");
         break;
      case LIBMTP_ERROR_USB_LAYER:
         LogError("USB Layer error occured. Exiting.");
         break;
      default:
         break;
      }
      return false;
   }

#ifndef HAVE_LIBUSB1
   if (!dev_file.empty())
   {
      uint8_t bnum, dnum;
      dev_no = raw_devices_cnt;

      if (smtpfs_usb_devpath(dev_file, &bnum, &dnum))
         for (dev_no = 0; dev_no < raw_devices_cnt; ++dev_no)
            if (bnum == raw_devices[dev_no].bus_location &&
                dnum == raw_devices[dev_no].devnum)
               break;

      if (dev_no == raw_devices_cnt)
      {
         LogError("Can not open such device '%s'", dev_file.c_str());
         free(static_cast<void *>(raw_devices));
         return false;
      }
   }
#endif // !HAVE_LIBUSB1

   if (dev_no < 0 || dev_no >= raw_devices_cnt)
   {
      LogError("Can not connect to device no %d", dev_no + 1);
      free(static_cast<void *>(raw_devices));
      return false;
   }

   LIBMTP_raw_device_t *raw_device = &raw_devices[dev_no];

   char tmpStr[128] = "";

   LogInfo("vendor_id: %x, product_id: %x\n", raw_device->device_entry.vendor_id, raw_device->device_entry.product_id);

   sprintf(tmpStr, "%x_%x_", raw_device->device_entry.vendor_id, raw_device->device_entry.product_id);

   //printf("tmpStr: %s\n", tmpStr);

   // Do not output LIBMTP debug stuff
   StreamHelper::off();
   m_device = LIBMTP_Open_Raw_Device_Uncached(raw_device);
   StreamHelper::on();
   free(static_cast<void *>(raw_devices));

   if (!m_device)
   {
      LIBMTP_Dump_Errorstack(m_device);
      return false;
   }

   if (!enumStorages())
      return false;

   // Retrieve capabilities.
   m_capabilities = MTPDevice::getCapabilities(*this);

   LogInfo("Connected.");
   g_device = m_device;

   string seriaID = LIBMTP_Get_Serialnumber(m_device);

   LogInfo("seriaID: %s", seriaID.c_str());

   mUUID = tmpStr + seriaID;

#ifdef MTP_HAS_DB
   writeDevice(mUUID);
#endif

   if (m_rbuf)
   {
      RingBuffer_Free(m_rbuf);
      m_rbuf = NULL;
   }
   //	else
   {
      m_rbuf = RingBuffer_Malloc(RINGBUF_SIZE);
   }

   //setLastModeSyncPath("/");

   return true;
}

bool MTPDevice::connect(int dev_no)
{
   return connect_priv(dev_no, std::string());
}

#ifdef HAVE_LIBUSB1
bool MTPDevice::connect(const std::string &dev_file)
{
   if (m_device)
   {
      LogInfo("Already connected");
      return true;
   }

   LIBMTP_raw_device_t *raw_device = smtpfs_raw_device_new(dev_file);
   if (!raw_device)
   {
      LogError("Can not open such device %s", dev_file.c_str());
      return false;
   }

   bool rval = connect(raw_device);

   // TODO:  Smart pointer with alloc, free hooks.
   smtpfs_raw_device_free(raw_device);

   return rval;
}
#else
bool MTPDevice::connect(const std::string &dev_file)
{
   return connect_priv(-1, dev_file);
}
#endif

void MTPDevice::disconnect()
{
   if (!m_device)
      return;

   LogInfo("MTPDevice::disconnect start");

   criticalEnter1();
   LIBMTP_Release_Device(m_device);
   criticalLeave1();
   if (m_rbuf)
   {
      RingBuffer_Free(m_rbuf);
      m_rbuf = NULL;
   }
   m_device = nullptr;
   g_device = m_device;
   mUUID = "";

   LogInfo("Disconnected ok");
}

uint64_t MTPDevice::storageTotalSize() const
{
   uint64_t total = 0;
   for (LIBMTP_devicestorage_t *s = m_device->storage; s; s = s->next)
      total += s->MaxCapacity;
   return total;
}

uint64_t MTPDevice::storageFreeSize() const
{
   uint64_t free = 0;
   for (LIBMTP_devicestorage_t *s = m_device->storage; s; s = s->next)
      free += s->FreeSpaceInBytes;
   return free;
}

bool MTPDevice::enumStorages()
{
   criticalEnter();
   LIBMTP_Clear_Errorstack(m_device);
   if (LIBMTP_Get_Storage(m_device, LIBMTP_STORAGE_SORTBY_NOTSORTED) < 0)
   {
      std::cerr << "Could not retrieve device storage.\n";
      std::cerr << "For android phones make sure the screen is unlocked.\n";
      LogError("Could not retrieve device storage. Exiting");
      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
      return false;
   }
   criticalLeave();
   return true;
}

const TypeDir *MTPDevice::dirFetchContentFromCache(std::string path)
{
   LogDebug("MTPDevice::dirFetchContentFromCache: path:%s", path.c_str());

   if (false == m_root_dir.isFetched())
   {
      //int count = 0;
      for (LIBMTP_devicestorage_t *s = m_device->storage; s; s = s->next)
      {
         m_root_dir.addDir(TypeDir(s_root_node, 0, s->id, std::string(s->StorageDescription)));
         LogInfo("path:%s addDir %s, storageid:%d", path.c_str(), s->StorageDescription, s->id);
         LogDebug("path:%s setFetched", path.c_str());
         m_root_dir.setFetched();
         //count++;
      }
      //m_root_dir.setItemCount(count);
      //m_root_dir.setFetchHandleDone(true);
   }

   // if (m_root_dir.dirCount() == 1)
   //     path = '/' + m_root_dir.dirs().begin()->name() + path;

   if (path == "/")
   {
      //LogInfo("path is / , return m_root_dir");
      return &m_root_dir;
   }

   std::string member;
   std::istringstream ss(path);
   TypeDir *dir = &m_root_dir;

   while (std::getline(ss, member, '/'))
   {

      if (member.empty())
         continue;

      const TypeDir *tmp = dir->dir(member);
      if (!tmp)
      {
         //LogError("not found :%s", member.c_str());
         return nullptr;
      }

      dir = const_cast<TypeDir *>(tmp);
   }

   LogDebug("dir->name: %s, exit", dir->name().c_str());
   return dir;
}

int MTPDevice::dirCreateNew(const std::string &path)
{
   const std::string tmp_basename(smtpfs_basename(path));
   const std::string tmp_dirname(smtpfs_dirname(path));
   const TypeDir *dir_parent = dirFetchContentFromCache(tmp_dirname);
   if (!dir_parent || dir_parent->id() == 0)
   {
      LogError("Can not remove directory %s", path.c_str());
      return -EINVAL;
   }
   char *c_name = strdup(tmp_basename.c_str());
   criticalEnter();
   uint32_t new_id = LIBMTP_Create_Folder(m_device, c_name,
                                          dir_parent->id(), dir_parent->storageid());
   criticalLeave();
   if (new_id == 0)
   {
      LogError("Could not create directory %s", path.c_str());
      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
   }
   else
   {
      const_cast<TypeDir *>(dir_parent)->addDir(TypeDir(new_id, dir_parent->id(), dir_parent->storageid(), tmp_basename));
      LogInfo("Directory %s created", path.c_str());
   }
   free(static_cast<void *>(c_name));
   return new_id != 0 ? 0 : -EINVAL;
}

int MTPDevice::dirRemove(const std::string &path)
{
   const std::string tmp_basename(smtpfs_basename(path));
   const std::string tmp_dirname(smtpfs_dirname(path));
   const TypeDir *dir_parent = dirFetchContentFromCache(tmp_dirname);
   const TypeDir *dir_to_remove = dir_parent ? dir_parent->dir(tmp_basename) : nullptr;
   if (!dir_parent || !dir_to_remove || dir_parent->id() == 0)
   {
      LogError("No such directory %s to remove", path.c_str());
      return -ENOENT;
   }
   if (!dir_to_remove->isEmpty())
      return -ENOTEMPTY;
   criticalEnter();
   int rval = LIBMTP_Delete_Object(m_device, dir_to_remove->id());
   criticalLeave();
   if (rval != 0)
   {
      LogError("Could not remove the directory %s", path.c_str());
      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
      return -EINVAL;
   }
   const_cast<TypeDir *>(dir_parent)->removeDir(*dir_to_remove);
   LogInfo("Folder %s removed", path.c_str());
   return 0;
}

int MTPDevice::dirRename(const std::string &oldpath, const std::string &newpath)
{
   const std::string tmp_old_basename(smtpfs_basename(oldpath));
   const std::string tmp_old_dirname(smtpfs_dirname(oldpath));
   const std::string tmp_new_basename(smtpfs_basename(newpath));
   const std::string tmp_new_dirname(smtpfs_dirname(newpath));
   const TypeDir *dir_parent = dirFetchContentFromCache(tmp_old_dirname);
   const TypeDir *dir_to_rename = dir_parent ? dir_parent->dir(tmp_old_basename) : nullptr;
   if (!dir_parent || !dir_to_rename || dir_parent->id() == 0)
   {
      LogError("can't rename %s to %s", tmp_old_basename.c_str(), tmp_new_basename.c_str());
      return -EINVAL;
   }
   if (tmp_old_dirname != tmp_new_dirname)
   {
      LogError("can't move %s to %s", oldpath.c_str(), newpath.c_str());
      return -EINVAL;
   }

   LIBMTP_folder_t *folder = dir_to_rename->toLIBMTPFolder();
   criticalEnter();
   int ret = LIBMTP_Set_Folder_Name(m_device, folder, tmp_new_basename.c_str());
   criticalLeave();
   free(static_cast<void *>(folder->name));
   free(static_cast<void *>(folder));
   if (ret != 0)
   {
      LogError("can't rename %s to %s", oldpath.c_str(), tmp_new_basename.c_str());
      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
      return -EINVAL;
   }
   const_cast<TypeDir *>(dir_to_rename)->setName(tmp_new_basename);
   LogInfo("move %s to %s", oldpath.c_str(), tmp_new_basename.c_str());
   return 0;
}

int MTPDevice::rename(const std::string &oldpath, const std::string &newpath)
{
#ifndef SMTPFS_MOVE_BY_SET_OBJECT_PROPERTY
   const std::string tmp_old_basename(smtpfs_basename(oldpath));
   const std::string tmp_old_dirname(smtpfs_dirname(oldpath));
   const std::string tmp_new_dirname(smtpfs_dirname(newpath));
   if (tmp_old_dirname != tmp_new_dirname)
      return -EINVAL;

   const TypeDir *dir_parent = dirFetchContentFromCache(tmp_old_dirname);
   if (!dir_parent || dir_parent->id() == 0)
      return -EINVAL;
   const TypeDir *dir_to_rename = dir_parent->dir(tmp_old_basename);
   if (dir_to_rename)
      return dirRename(oldpath, newpath);
   else
      return fileRename(oldpath, newpath);
#else
   const std::string tmp_old_basename(smtpfs_basename(oldpath));
   const std::string tmp_old_dirname(smtpfs_dirname(oldpath));
   const std::string tmp_new_basename(smtpfs_basename(newpath));
   const std::string tmp_new_dirname(smtpfs_dirname(newpath));
   const TypeDir *dir_old_parent = dirFetchContentFromCache(tmp_old_dirname);
   const TypeDir *dir_new_parent = dirFetchContentFromCache(tmp_new_dirname);
   const TypeDir *dir_to_rename = dir_old_parent ? dir_old_parent->dir(tmp_old_basename) : nullptr;
   const TypeFile *file_to_rename = dir_old_parent ? dir_old_parent->file(tmp_old_basename) : nullptr;

   LogInfo("dir_to_rename:%x, file_to_rename:%x", dir_to_rename, file_to_rename);

   if (!dir_old_parent || !dir_new_parent || dir_old_parent->id() == 0)
      return -EINVAL;

   const TypeBasic *object_to_rename = dir_to_rename ? static_cast<const TypeBasic *>(dir_to_rename) : static_cast<const TypeBasic *>(file_to_rename);

   LogInfo("object_to_rename:%x, object_to_rename->id():%d", object_to_rename, object_to_rename->id());

   if (!object_to_rename)
   {
      LogError("No such file or directory to rename/move!");
      return -ENOENT;
   }

   if (tmp_old_dirname != tmp_new_dirname)
   {
      criticalEnter();
      int rval = LIBMTP_Set_Object_u32(m_device, object_to_rename->id(),
                                       LIBMTP_PROPERTY_ParentObject, dir_new_parent->id());
      criticalLeave();
      if (rval != 0)
      {
         LogError("can't move %s to %s", oldpath.c_str(), newpath.c_str());
         LIBMTP_Dump_Errorstack(m_device);
         LIBMTP_Clear_Errorstack(m_device);
         return -EINVAL;
      }
      const_cast<TypeBasic *>(object_to_rename)->setParent(dir_new_parent->id());
   }
   if (tmp_old_basename != tmp_new_basename)
   {
      criticalEnter();
      int rval = LIBMTP_Set_Object_String(m_device, object_to_rename->id(),
                                          LIBMTP_PROPERTY_Name, tmp_new_basename.c_str());
      criticalLeave();
      if (rval != 0)
      {
         LogError("can't move %s to %s", oldpath.c_str(), newpath.c_str());
         LIBMTP_Dump_Errorstack(m_device);
         LIBMTP_Clear_Errorstack(m_device);
         return -EINVAL;
      }
   }
   return 0;
#endif
}

int MTPDevice::filesplitPull(const std::string &src, const std::string &dst, uint32_t size, uint32_t offset, unsigned char *buf)
{
   bool isInCache = true;
   int readLen = 0;
   const std::string src_basename(smtpfs_basename(src));
   const std::string src_dirname(smtpfs_dirname(src));

   //LogInfo("basename: %s, dirname: %s", src_basename.c_str(), src_dirname.c_str());

   //LogInfo("src: %s, size:%u, offset:%llu", src.c_str(), , offset);

   //LogInfo("read path: %s, old path: %s", src.c_str(), m_fileName.c_str());

   if (m_fileName != src)
   {
      const TypeDir *dir_parent = dirFetchContentFromCache(src_dirname);
      const TypeFile *file_to_fetch = dir_parent ? dir_parent->file(src_basename) : nullptr;
      if (!dir_parent)
      {
         //LogDebug("dir_parent Can not fetch %s", src.c_str());
         isInCache = false;
      }

      if (!file_to_fetch)
      {
         //LogDebug("file_to_fetch No such file %s", src.c_str());
         isInCache = false;
      }

      if (isInCache == true)
      {
         m_fileId = file_to_fetch->id();
         m_fileSize = file_to_fetch->size();
      }
      else
      {
         string path = src.c_str();
#ifdef MTP_HAS_DB
         queryFileID(path, getUUID(), &m_fileId);
         if (!m_fileId)
#endif
         {
            LogError("queryFileID No such file %s", src.c_str());
            return -EINVAL;
         }

         criticalEnter1();
         LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(m_device, m_fileId);
         criticalLeave1();
         if (file)
         {
            m_fileSize = file->filesize;
         }
      }

      m_fileName = src;
      m_fileOffset = 0;
      RingBuffer_Reset(m_rbuf);
   }

   readLen = readByRingBuf(m_fileId, m_fileSize, size, offset, buf);
   if(readLen <= 0)
   {
      LogInfo("%s, return dataLen: %d, ask size: %d, ask offset: %u, fileSize: %llu", src.c_str(), readLen, size, offset, m_fileSize);
   }
   return readLen;
}

int MTPDevice::readByRingBuf(const uint32_t id, const uint64_t fileSize, const uint32_t size, const uint32_t offset, const unsigned char *buf)
{
//LogInfo("offset: %u, size: %u, m_fileOffset: %u, fileSize: %llu", offset, size, m_fileOffset, fileSize);

#if 0
    //1st, check if the ringbuf valid or not
    if(m_rbuf == NULL)
    {   
        LogError("m_rbuf is NULL");
        return -1;
    }

	if(offset >= fileSize)
	{
		LogError("offset[%u] >= fileSize[%llu] fail", offset, fileSize);
		return -1;
	}

	if(m_fileOffset > fileSize)
	{
		LogError("offset[%u] >= fileSize[%llu] fail", offset, fileSize);
		return -1;
	}

	bool readFromDevice = false;
	uint32_t rbRemainLen = RingBuffer_Len(m_rbuf);
	uint64_t rbOffsetEnd = m_fileOffset;
	uint64_t rbOffsetStart = rbOffsetEnd - rbRemainLen;

	//LogInfo("rbRemainLen: %u, rbOffsetStart: %llu, rbOffsetEnd: %llu", rbRemainLen, rbOffsetStart, rbOffsetEnd);
	//no data in ringbuf
	if(!rbRemainLen && size)
	{
		//LogInfo("rb reset: rb remail is 0");
		RingBuffer_Reset(m_rbuf);
		m_fileOffset = offset;
		readFromDevice = true;
	}
	//ask offset and size is out of ringbuf range
	else if((offset < rbOffsetStart) || (offset > rbOffsetEnd))
	{
		//LogInfo("rb reset, offset is out of range, rbOffsetLow: %u", rbOffsetLow);
		RingBuffer_Reset(m_rbuf);
		m_fileOffset = offset;
		readFromDevice = true;
	}

    //if ring buf is empty, read data from device into ringbuf
    if(readFromDevice)
    { 
        int rVal = 0;
        uint32_t tmpLen = 0;
        long diffTime;
        unsigned char* tmpBuf = NULL;

		m_read_data_cmd_coming = 1;
        long t1 = getSysRunTime();
		//LogInfo("read get lock, 1111");
        criticalEnter1();
		//LogInfo("read get lock, 2222");
        rVal = LIBMTP_GetPartialObject(m_device, id, m_fileOffset, RINGBUF_SIZE, &tmpBuf, &tmpLen);
		//LogInfo("id: %d, rVal: %d, tmpBuf: %p, tmpLen: %d, fileSize: %llu ms", id, rVal, tmpBuf, tmpLen, fileSize);
		//LogInfo("read get lock, 3333");
        criticalLeave1();
		//LogInfo("read get lock, 4444");
        long t2 = getSysRunTime();
        diffTime = t2 - t1;
        if(diffTime > 2000)
        {
            LogInfo("id: %d, rVal: %d, tmpBuf: %p, tmpLen: %d, diffTime: %ld ms", id, rVal, tmpBuf, tmpLen, diffTime);
        }

		m_read_data_cmd_coming = 0;

        //LogInfo("LIBMTP_GetPartialObject rVal:%d, tmpLen:%d, RINGBUF_SIZE:%d", rVal, tmpLen, RINGBUF_SIZE);

        if(rVal || (tmpLen > RINGBUF_SIZE)) 
        {
            LogError("LIBMTP_GetPartialObject, id: %u, rVal: %d, tmpLen[%d] < RINGBUF_SIZE[%d]", id, rVal, tmpLen, RINGBUF_SIZE);
            LIBMTP_Dump_Errorstack(m_device);
            LIBMTP_Clear_Errorstack(m_device);
			return -1;
        }
        else if(!rVal)
        { 
			if(tmpLen && tmpBuf)
			{
				//push data into ringbuf
				m_fileOffset += tmpLen;
				// if(m_fileOffset == fileSize)
				// {
				// 	m_fileOffset = 0;
				// }
				RingBuffer_In(m_rbuf, (void *)tmpBuf, tmpLen);
				free(tmpBuf);

				//LogInfo("had read from phone, m_fileOffset: %llu, tmpLen: %u, rbLen: %u", m_fileOffset, tmpLen, RingBufferGetDataSize(m_rbuf));
			}
			else 
			{
				if(m_fileOffset != fileSize)
				{
					LogWarning("read return len == 0, ask app retry");
					return -EAGAIN;
				}
				else
				{
					LogInfo("read reach eof");
					return 0;
				}
			}
        }  
		
    }

    //get data from ring buf
    size_t rBufDataSize = RingBuffer_Out(m_rbuf, (void *)buf, size);
	return rBufDataSize;
#else
   m_read_data_cmd_coming = 1;
   int rVal = 0;
   uint32_t tmpLen = 0;
   long diffTime;
   unsigned char *tmpBuf = NULL;

   long t1 = getSysRunTime();
   //LogInfo("read get lock, 1111");
   criticalEnter1();
   //LogInfo("read get lock, 2222");
   rVal = LIBMTP_GetPartialObject(m_device, id, offset, size, &tmpBuf, &tmpLen);
   //LogInfo("read get lock, 3333");
   criticalLeave1();
   //LogInfo("read get lock, 4444");
   long t2 = getSysRunTime();
   diffTime = t2 - t1;
   if (diffTime > LIBMTP_REQUEST_TIMEOUT)
   {
      LogInfo("id: %d, retLen: %d, diffTime: %ld ms", id, rVal, diffTime);
   }

   m_read_data_cmd_coming = 0;

   //LogInfo("LIBMTP_GetPartialObject rVal:%d, tmpLen:%d, willReadLen:%d", rVal, tmpLen, willReadLen);

   if (rVal || (tmpLen > size))
   {
      LogError("LIBMTP_GetPartialObject, id: %u, rVal: %d, tmpLen[%d] < willReadLen[%d]", id, rVal, tmpLen, size);

      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
      return -1;
   }
   else
   {
      if (tmpLen && tmpBuf)
      {
         //push data into ringbuf
         m_fileOffset += tmpLen;
         if (m_fileOffset == fileSize)
         {
            m_fileOffset = 0;
         }
         memcpy((void *)buf, (void *)tmpBuf, tmpLen);
         free(tmpBuf);
         return tmpLen;
      }
      else
      {
         if (m_fileOffset != fileSize)
         {
            LogWarning("read return len == 0, ask app retry; ofst: %" PRIu64 "; fileSize: %" PRIu64 "; tmpLen: %u; tmpBuf: %x; LIBMTP_GetPartialObject return: %x", m_fileOffset, fileSize, tmpLen, tmpBuf, rVal);
            return -EAGAIN;
         }
         else
         {
            LogInfo("read reach eof; ofst: %" PRIu64 "; fileSize: %" PRIu64 "; tmpLen: %u; tmpBuf: %x; LIBMTP_GetPartialObject return: %x", m_fileOffset, fileSize, tmpLen, tmpBuf, rVal);
            return 0;
         }
      }
   }
#endif
}

int MTPDevice::filePull(const std::string &src, const std::string &dst)
{
   const std::string src_basename(smtpfs_basename(src));
   const std::string src_dirname(smtpfs_dirname(src));
   const TypeDir *dir_parent = dirFetchContentFromCache(src_dirname);
   const TypeFile *file_to_fetch = dir_parent ? dir_parent->file(src_basename) : nullptr;
   if (!dir_parent)
   {
      LogError("filePull, Can not fetch %s", src.c_str());
      return -EINVAL;
   }
   if (!file_to_fetch)
   {
      LogError("filePull, no such file %s", src.c_str());
      return -ENOENT;
   }
   if (file_to_fetch->size() == 0)
   {
      int fd = ::creat(dst.c_str(), S_IRUSR | S_IWUSR);
      ::close(fd);
   }
   else
   {
      LogInfo("Started fetching %s", src.c_str());
      criticalEnter();
      int rval = LIBMTP_Get_File_To_File(m_device, file_to_fetch->id(),
                                         dst.c_str(), nullptr, nullptr);
      criticalLeave();
      if (rval != 0)
      {
         LogError("Could not fetch file %s", src.c_str());
         LIBMTP_Dump_Errorstack(m_device);
         LIBMTP_Clear_Errorstack(m_device);
         return -ENOENT;
      }
   }
   LogInfo("File %s Fetched", src.c_str());
   return 0;
}

int MTPDevice::filePush(const std::string &src, const std::string &dst)
{
   const std::string dst_basename(smtpfs_basename(dst));
   const std::string dst_dirname(smtpfs_dirname(dst));
   const TypeDir *dir_parent = dirFetchContentFromCache(dst_dirname);
   const TypeFile *file_to_remove = dir_parent ? dir_parent->file(dst_basename) : nullptr;
   if (dir_parent && file_to_remove)
   {
      criticalEnter();
      int rval = LIBMTP_Delete_Object(m_device, file_to_remove->id());
      criticalLeave();
      if (rval != 0)
      {
         LogError("Can not upload %s to %s", src.c_str(), dst.c_str());
         return -EINVAL;
      }
   }

   struct stat file_stat;
   stat(src.c_str(), &file_stat);
   TypeFile file_to_upload(0, dir_parent->id(), dir_parent->storageid(),
                           dst_basename, static_cast<uint64_t>(file_stat.st_size), 0);
   LIBMTP_file_t *f = file_to_upload.toLIBMTPFile();
   if (file_stat.st_size)
   {
      LogInfo("Started uploading %s", dst.c_str());
   }
   criticalEnter();
   int rval = LIBMTP_Send_File_From_File(m_device, src.c_str(), f, nullptr, nullptr);
   criticalLeave();
   if (rval != 0)
   {
      LogError("Could not upload file %s", src.c_str());
      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
      rval = -EINVAL;
   }
   else
   {
      file_to_upload.setId(f->item_id);
      file_to_upload.setParent(f->parent_id);
      file_to_upload.setStorage(f->storage_id);
      file_to_upload.setName(std::string(f->filename));
      file_to_upload.setModificationDate(file_stat.st_mtime);
      if (file_to_remove)
         const_cast<TypeDir *>(dir_parent)->replaceFile(*file_to_remove, file_to_upload);
      else
         const_cast<TypeDir *>(dir_parent)->addFile(file_to_upload);
   }
   free(static_cast<void *>(f->filename));
   free(static_cast<void *>(f));
   LogInfo("File %s been %s", dst.c_str(), (file_stat.st_size ? " uploaded" : " created"));
   return rval;
}

int MTPDevice::fileRemove(const std::string &path)
{
   const std::string tmp_basename(smtpfs_basename(path));
   const std::string tmp_dirname(smtpfs_dirname(path));
   const TypeDir *dir_parent = dirFetchContentFromCache(tmp_dirname);
   const TypeFile *file_to_remove = dir_parent ? dir_parent->file(tmp_basename) : nullptr;
   if (!dir_parent || !file_to_remove)
   {
      LogError("No such file '%s' to remove", path.c_str());
      return -ENOENT;
   }
   criticalEnter();
   int rval = LIBMTP_Delete_Object(m_device, file_to_remove->id());
   criticalLeave();
   if (rval != 0)
   {
      LogError("Could not remove the directory '%s'", path.c_str());
      return -EINVAL;
   }
   const_cast<TypeDir *>(dir_parent)->removeFile(*file_to_remove);
   LogInfo("File '%s' removed", path.c_str());
   return 0;
}

int MTPDevice::fileRename(const std::string &oldpath, const std::string &newpath)
{
   const std::string tmp_old_basename(smtpfs_basename(oldpath));
   const std::string tmp_old_dirname(smtpfs_dirname(oldpath));
   const std::string tmp_new_basename(smtpfs_basename(newpath));
   const std::string tmp_new_dirname(smtpfs_dirname(newpath));
   const TypeDir *dir_parent = dirFetchContentFromCache(tmp_old_dirname);
   const TypeFile *file_to_rename = dir_parent ? dir_parent->file(tmp_old_basename) : nullptr;
   if (!dir_parent || !file_to_rename || tmp_old_dirname != tmp_new_dirname)
   {
      LogError("Can not rename %s to %s", oldpath.c_str(), tmp_new_basename.c_str());
      return -EINVAL;
   }

   LIBMTP_file_t *file = file_to_rename->toLIBMTPFile();
   criticalEnter();
   int rval = LIBMTP_Set_File_Name(m_device, file, tmp_new_basename.c_str());
   criticalLeave();
   free(static_cast<void *>(file->filename));
   free(static_cast<void *>(file));
   if (rval > 0)
   {
      LogError("Can not rename %s to %s", oldpath.c_str(), newpath.c_str());
      LIBMTP_Dump_Errorstack(m_device);
      LIBMTP_Clear_Errorstack(m_device);
      return -EINVAL;
   }
   const_cast<TypeFile *>(file_to_rename)->setName(tmp_new_basename);
   LogError("File %s renamed to %s", oldpath.c_str(), tmp_new_basename.c_str());
   return 0;
}

MTPDevice::Capabilities MTPDevice::getCapabilities() const
{
   return m_capabilities;
}

MTPDevice::Capabilities MTPDevice::getCapabilities(const MTPDevice &device)
{
   MTPDevice::Capabilities capabilities;

#ifdef HAVE_LIBMTP_CHECK_CAPABILITY
   if (device.m_device)
   {
      capabilities.setCanGetPartialObject(
          static_cast<bool>(
              LIBMTP_Check_Capability(
                  device.m_device,
                  LIBMTP_DEVICECAP_GetPartialObject)));
      capabilities.setCanSendPartialobject(
          static_cast<bool>(
              LIBMTP_Check_Capability(
                  device.m_device,
                  LIBMTP_DEVICECAP_SendPartialObject)));
      capabilities.setCanEditObjects(
          static_cast<bool>(
              LIBMTP_Check_Capability(
                  device.m_device,
                  LIBMTP_DEVICECAP_EditObjects)));
   }
#endif

   return capabilities;
}

bool MTPDevice::listDevices(bool verbose, const std::string &dev_file)
{
   int raw_devices_cnt;
   LIBMTP_raw_device_t *raw_devices;

   // Do not output LIBMTP debug stuff
   StreamHelper::off();
   LIBMTP_error_number_t err = LIBMTP_Detect_Raw_Devices(
       &raw_devices, &raw_devices_cnt);
   StreamHelper::on();

   if (err != 0)
   {
      if (err == LIBMTP_ERROR_NO_DEVICE_ATTACHED)
         std::cerr << "No raw devices found.\n";
      return false;
   }

   uint8_t bnum, dnum;
   if (!dev_file.empty() && !smtpfs_usb_devpath(dev_file, &bnum, &dnum))
   {
      std::cerr << "Can not open such device '" << dev_file << "'.\n";
      return false;
   }

   for (int i = 0; i < raw_devices_cnt; ++i)
   {
      if (!dev_file.empty() &&
          !(bnum == raw_devices[i].bus_location && dnum == raw_devices[i].devnum))
         continue;

      char bus_num_str[4];
      char dev_num_str[4];
      sprintf(bus_num_str, "%03d", raw_devices[i].bus_location);
      sprintf(dev_num_str, "%03d", raw_devices[i].devnum);

      //printf("vid: %x, pid: %x\n", raw_devices[i].device_entry.vendor_id, raw_devices[i].device_entry.product_id);

      Logger::off();
      std::cout << i + 1 << ": "
                << (raw_devices[i].device_entry.vendor ? raw_devices[i].device_entry.vendor : "Unknown vendor ")
                << (raw_devices[i].device_entry.product ? raw_devices[i].device_entry.product : "Unknown product")
                << " /dev/bus/usb/" << bus_num_str << "/" << dev_num_str
                << std::endl;
#ifdef HAVE_LIBMTP_CHECK_CAPABILITY
      MTPDevice dev(*SMTPFileSystem::instance());
      if (verbose)
      {
         if (!dev.connect(&raw_devices[i]))
            return false;

         const MTPDevice::Capabilities &cap = dev.getCapabilities();
         std::cout << "  - can get  partial object: " << (cap.canGetPartialObject() ? "yes" : "no") << std::endl;
         std::cout << "  - can send partial object: " << (cap.canSendPartialObject() ? "yes" : "no") << std::endl;
         std::cout << "  - can edit objects       : " << (cap.canEditObjects() ? "yes" : "no") << std::endl;
         dev.disconnect();
      }
#endif
      Logger::on();
   }
   free(static_cast<void *>(raw_devices));

   return true;
}

int32_t MTPDevice::getFileInfoById(uint32_t id, struct stat *st)
{
   if (st == NULL)
   {
      LogError("st is NULL");
      return -1;
   }

   criticalEnter1();
   LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(m_device, id);
   criticalLeave1();
   if (file == NULL)
   {
      st->st_ino = id;
      st->st_size = 1024;
      st->st_blocks = 1;
      st->st_nlink = 1;
      st->st_mode = S_IFREG | 0644;
      st->st_mtime = time(NULL);
      st->st_ctime = st->st_mtime;
      st->st_atime = st->st_mtime;

      LogDebug("id: %d file is NULL, fake it, size: %llu", id, st->st_size);
      return 0;
   }

   //fill st with file
   st->st_ino = id;
   st->st_size = file->filesize;
   st->st_blocks = (file->filesize / 512) + (file->filesize % 512 > 0 ? 1 : 0);
   st->st_nlink = 1;
   st->st_mode = S_IFREG | 0644;
   st->st_mtime = file->modificationdate;
   st->st_ctime = st->st_mtime;
   st->st_atime = st->st_mtime;

   LogDebug("id: %d file, size: %llu", id, st->st_size);

   LIBMTP_Free_Files_And_Folders(&file);

   return 0;
}

bool MTPDevice::isFileExist(const uint32_t id)
{
   bool ret = false;

   criticalEnter1();
   ret = LIBMTP_Track_Exists(m_device, id);
   criticalLeave1();

   return ret;
}

int MTPDevice::getMediaFileType(const std::string &filename)
{
   //track and image
   int ret = MTP_FILE_TYPE_UNKNOWN;
   if (reMatch(filename, mReExtNameAudio))
   {
      ret = MTP_FILE_TYPE_AUDIO;
   }
   else if (reMatch(filename, mReExtNameVideo))
   {
      ret = MTP_FILE_TYPE_VIDEO;
   }
   else if (reMatch(filename, mReExtNameImage))
   {
      ret = MTP_FILE_TYPE_IMAGE;
   }
   else if (reMatch(filename, mReExtNamePlaylist))
   {
      ret = MTP_FILE_TYPE_PLAYLIST;
   }
   else if (reMatch(filename, mReExtNameAudiobook))
   {
      ret = MTP_FILE_TYPE_AUDIOBOOKS;
   }

   return ret;
}

int MTPDevice::handleNewFileCache(const std::string &path)
{
   LogInfo("handleNewFileCache Enter: [%s], isScanDone = %d, size = [%d]", path.c_str(), m_list_all_done, m_tmpTrackMetaList.size());
   if(m_list_all_done)
   {
      LogInfo("The scanning is done, No need to cache the track [%s]", path.c_str());
      return 0;
   }

   bool isFoundInList = false;
   bool isFoundIntmp = false;
   std::vector<smtpfs_trackInfo>::iterator iter = m_trackMetaList.begin();
   if(!path.empty())
   {
      //find from the uploaded list first
      while(iter != m_trackMetaList.end())
      {
         if(0 == strcmp(iter->path, path.c_str()))
         {
            isFoundInList = true;
            break;
         }
         iter++;
      }
   }

   int ret = 1;
   if(false == isFoundInList)
   {
      //find from pending uploaded list
      iter = m_tmpTrackMetaList.begin();
      while(iter != m_tmpTrackMetaList.end())
      {
         if((path.empty() && MTP_FILE_TYPE_AUDIO == iter->type) || (!path.empty() && 0 == strcmp(iter->path, path.c_str())))
         {
            //if path is empty, cache the first audio track here; or find the target path
            if(iter->filesize > 0)
            {
               isFoundIntmp = true;
               break;
            }
         }
         iter++;
      }
   }
   
   if(isFoundInList || isFoundIntmp)
   {
      if(0 == (ret = mFileSystem.onFileCache(iter->path)))
      {
         mCacheNewTrackCmd.cmdStatus = SMTP_ASYNC_CMD_STATUS_FINISH;
         mCacheNewTrackCmd.responseData = COMMAND_RESPONSE_STATUS_SUCESS;
         if (isFoundIntmp)
         {
            pushTrackInfo(*iter);
            m_tmpTrackMetaList.erase(iter);
         }
         
         LogInfo("handleNewFileCache Cache success: [%s]", iter->path);
      }
   }
   else
   {
      //if not found the path, the suggest path will be ignored now. if needed, please add logic later
      LogInfo("Not found in the scanned list, A audio track will be cached in following scanning process!");
   }

   LogInfo("handleNewFileCache Exit: pList->size = %d, ret = %d", m_tmpTrackMetaList.size(), ret);
   return ret;
}

int MTPDevice::getAllFolderAndFiles(LIBMTP_mtpdevice_t *device, LIBMTP_devicestorage_t *storage, uint32_t id, string &path, const string &lastModePath)
{
   LogDebug("enter scan for folder: %s", path.c_str());
   if (mScanAbort)
   {
      LogInfo("mtp scanning was aborted");
      return -1;
   }
   if (NULL == device)
   {
      LogError("the device is NULL");
      return -1;
   }
   if (NULL == storage)
   {
      LogError("the storage is NULL");
      return -1;
   }
   if (NULL == storage->StorageDescription)
   {
      LogError("the StorageDescription is NULL");
      return -1;
   }
   // get the exclude regex
   std::regex reExclude;
   auto it = mReMapExclude.find(std::string(storage->StorageDescription));

   if (it == mReMapExclude.end())
   {
      LogWarning("found no regex for storage: %s", storage->StorageDescription);
   }
   else
   {
      reExclude = it->second;
   }

   int ret = 0;
   ObjectHandles objHlds;

   objHlds.n = 0;
   objHlds.Handler = NULL;

   //LogInfo("enter\n");
   //LogInfo("search cache path: %s\n", path.c_str());

   TypeDir *parentDir = (TypeDir *)dirFetchContentFromCache(path);
   if (parentDir == NULL)
   {
      LogError("dir is NULL");
      return -1;
   }

   while (m_read_data_cmd_coming)
   {
      if (mScanAbort)
      {
         LogInfo("mtp scanning was aborted");
         return -1;
      }
      LogInfo("in syncing, wait, let read cmd first");
      usleep(200000);
   }

   /**
    * Because the bellow API may cost long time, so check whether to cache a new track firstly.
   */
   if(lastModePath.empty() && (SMTP_ASYNC_CMD_STATUS_WAIT == mCacheNewTrackCmd.cmdStatus))
   {
      LogInfo("handleNewFileCache before get files and folders count!");
      mCacheNewTrackCmd.cmdStatus = SMTP_ASYNC_CMD_STATUS_EXECUTE;
      mNeedCacheFile = handleNewFileCache();
   }

   long t1 = 0, t2 = 0;
   criticalEnter1();
   if (device)
   {
      t1 = getSysRunTime();
      ret = LIBMTP_Get_Files_And_Folders_Count(device, storage->id, id, &objHlds);
      t2 = getSysRunTime();
   }
   criticalLeave1();
   if ((t2 - t1) >= LIBMTP_REQUEST_TIMEOUT)
   {
      LogInfo("get(%s)count[%u] took: %lu ms", path.c_str(), objHlds.n, (t2 - t1));
   }

   if (ret < 0)
   {
      LogError("LIBMTP_Get_Files_And_Folders_Count: %s failed", path.c_str());
      return -1;
   }

   uint32_t timeoutCnt = 0;
   for (uint32_t i = 0; i < objHlds.n; i++)
   {
      while (m_read_data_cmd_coming)
      {
         if (mScanAbort)
         {
            LogInfo("mtp scanning was aborted");
            break;
         }
         LogInfo("in syncing, wait, let read cmd first");
         usleep(200000);
      }

      if (device == NULL)
         break;
      //if a large numer of file in the folder it will costs long time
      //so check whether to cache new file here
      if(lastModePath.empty() && (SMTP_ASYNC_CMD_STATUS_WAIT == mCacheNewTrackCmd.cmdStatus))
      {
         LogInfo("handleNewFileCache in loop before get objInfo by Handle!");
         mCacheNewTrackCmd.cmdStatus = SMTP_ASYNC_CMD_STATUS_EXECUTE;
         mNeedCacheFile = handleNewFileCache();
      }

      long t3 = 0, t4 = 0;
      criticalEnter1();
      t3 = getSysRunTime();
      LIBMTP_file_t *mtpFile = LIBMTP_Get_Files_And_Folders_ByHandle(device, objHlds.Handler[i]);
      t4 = getSysRunTime();
      criticalLeave1();

      if ((t4 - t3) >= LIBMTP_REQUEST_TIMEOUT)
      {
         if (mtpFile != NULL)
         {
            LogInfo("get (%s: %s) items took: %lu ms", path.c_str(), mtpFile->filename, (t4 - t3));
            timeoutCnt = 0;
         }
         else
         {
            LogInfo("get (%s: NULL) items took: %lu ms", path.c_str(), (t4 - t3));

            if ((t4 - t3) >= LIBMTP_REQUEST_TIMEOUT_ABORT)
            {
               timeoutCnt++;
            }

            //for Samsung s7 and s9, there is a
            if (timeoutCnt >= 2)
            {
               LogWarning("can not get: %s items, skip it", path.c_str());
               break;
            }
         }
      }

      while (mtpFile != NULL)
      {
         if (mScanAbort)
         {
            LogInfo("mtp scanning was aborted");
            break;
         }
         if (m_lastMode_sync_done && !lastModePath.empty())
         {
            LogInfo("----lastmode scan done----");
            break;
         }

         if (device == NULL)
            break;
         LIBMTP_file_t *tmpFile = mtpFile;

         if (tmpFile->filetype == LIBMTP_FILETYPE_FOLDER)
         {
            string folderPath = path + "/" + tmpFile->filename;

            LogDebug("scan folder: %s, path: %s---------", tmpFile->filename, folderPath.c_str());
            if (reMatch(folderPath + "/", reExclude))
            {
               LogInfo("ignore folder: %s", folderPath.c_str());
               ++ mFolderIgnored;
            }
            else
            {
               if (lastModePath.empty() || (!lastModePath.empty() && !lastModePath.find(folderPath)))
               {
                  parentDir->addDir(TypeDir(tmpFile));
                  getAllFolderAndFiles(device, storage, tmpFile->item_id, folderPath, lastModePath);
               }
               else
               {
                  LogInfo("lastmode skip folder: %s", folderPath.c_str());
               }
            }
         }
         else
         {
            string filePath = path + "/" + tmpFile->filename;
            LogDebug("scan file: %s, path: %s, size: %llu---------", tmpFile->filename, filePath.c_str(), tmpFile->filesize);

            //save into db
            int fileType = getMediaFileType(tmpFile->filename);
            if (MTP_FILE_TYPE_UNKNOWN != fileType)
            {
               //LogInfo("writeFileID, filePath: %s", filePath.c_str());
               parentDir->addFile(TypeFile(tmpFile));
#ifdef MTP_HAS_DB
               writeFileID(filePath, tmpFile->item_id, mUUID);
#endif

               criticalEnter1();
               t1 = getSysRunTime();
               LIBMTP_track_t *deviceTrack = LIBMTP_Get_Trackmetadata(m_device, tmpFile->item_id);
               t2 = getSysRunTime();
               criticalLeave1();
               if ((t2 - t1) >= LIBMTP_REQUEST_TIMEOUT)
               {
                  LogInfo("LIBMTP_Get_Trackmetadata, id: %u; path: %s; took: %lu ms", tmpFile->item_id, filePath.c_str(), (t2 - t1));
               }

               smtpfs_trackInfo smtpfsTrack;
               memset(&smtpfsTrack, 0x00, sizeof(smtpfs_trackInfo));
               if (deviceTrack)
               {
                  //LogInfo("have track info");
                  fillTrackInfo(&smtpfsTrack, deviceTrack, filePath.c_str(), fileType);
               }
               else
               {
                  //LogInfo("no track info");
                  fillTrackInfoByFile(&smtpfsTrack, tmpFile, filePath.c_str(), fileType);
               }

               if (deviceTrack)
               {
                  criticalEnter1();
                  LIBMTP_destroy_track_t(deviceTrack);
                  criticalLeave1();
               }

               ret = -1;
               if (lastModePath.empty() && (m_lastmode_path != filePath))
               {
                  // for non-lastmode, need to cache the first audio file successfully, and notify it to MediaOne.
                  // but others files should not be notified to MediaOne, until the mtp scanning completed.
                  if (mNeedCacheFile && MTP_FILE_TYPE_AUDIO == fileType)
                  {
                     ret = mFileSystem.onFileCache(filePath);
                  }
                  if (0 == ret)
                  {
                     if(SMTP_ASYNC_CMD_STATUS_EXECUTE == mCacheNewTrackCmd.cmdStatus)
                     {
                        LogInfo("handleNewFileCache Cache success(new scanned track): [%s]", filePath.c_str());
                        mCacheNewTrackCmd.cmdStatus = SMTP_ASYNC_CMD_STATUS_FINISH;
                        mCacheNewTrackCmd.responseData = COMMAND_RESPONSE_STATUS_SUCESS;
                     }
                     pushTrackInfo(smtpfsTrack);
                     mNeedCacheFile = false;
                  }
                  else
                  {
                     if (Config::instance().getEnableCacheFirstFile())
                     {
                        m_tmpTrackMetaList.push_back(smtpfsTrack);
                     }
                     else
                     {
                        pushTrackInfo(smtpfsTrack);
                     }
                  }
               }
               if (!lastModePath.empty() && (lastModePath == filePath))
               {
                  // for lastmode, just cache the lastmode file.
                  // even if cache failed, notify the lastmode track to MediaOne.
                  if (mNeedCacheFile)
                  {
                     ret = mFileSystem.onFileCache(filePath);
                  }
                  pushTrackInfo(smtpfsTrack);
                  mNeedCacheFile = false;
                  LogInfo("----find lastModePath: %s---", lastModePath.c_str());
                  m_lastMode_sync_done = true;
                  break;
               }
            }
            else
            {
               LogDebug("%s is unknown type; ignored", tmpFile->filename);
               ++ mFileIgnored;
            }
         }

         mtpFile = tmpFile->next;
         criticalEnter1();
         LIBMTP_destroy_file_t(tmpFile);
         criticalLeave1();
      }

      if (m_lastMode_sync_done && !lastModePath.empty())
      {
         LogInfo("----lastmode scan done----");
         break;
      }
   }

   criticalEnter1();
   LIBMTP_Free_ObjHandles(&objHlds);
   criticalLeave1();
   parentDir->setFetched();

   return 0;
}

int32_t MTPDevice::autoListAll()
{
   setThreadPriority(mScanThPolicy, mScanThPriority);
   if (m_device == NULL)
   {
      LogError("m_device is NULL\n");
      return -1;
   }

   LogInfo("autoListAll enter\n");

   LIBMTP_devicestorage_t *storage = NULL;
   string path = "/";

   long whole_t1 = getSysRunTime();

   dirFetchContentFromCache(path);

   m_lastMode_sync_done = false;

   // make the regex for all of the storages
   for (storage = m_device->storage; storage != 0; storage = storage->next)
   {
      // make the regex for the specified storage
      std::string pattern = std::string("\\/") + std::string(storage->StorageDescription) + Config::instance().getExcPattern();
      std::regex re;

      if (makeRegEx(re, pattern))
      {
         LogError("has made the regex for the storage : %s; pattern: %s", storage->StorageDescription, pattern.c_str());
         mReMapExclude[std::string(storage->StorageDescription)] = re;
      }
      else
      {
         LogError("make the regex for the storage failed; sotrage: %s; pattern: %s", storage->StorageDescription, pattern.c_str());
      }
   }

   //check if sync lastmode firstly
   if (!m_lastmode_path.empty() && (m_lastmode_path != path))
   {
      for (storage = m_device->storage; storage != 0; storage = storage->next)
      {
         //get all items
         LogInfo("start lastmode scan Storage: %s", storage->StorageDescription);
         string tmpPath = path + storage->StorageDescription;

         if (m_lastmode_path.find(tmpPath))
         {
            LogInfo("lastmode, m_lastmode_path: %s, tmpPath: %s", m_lastmode_path.c_str(), tmpPath.c_str());
            continue;
         }

         long t1 = getSysRunTime();

         if (m_device)
         {
            getAllFolderAndFiles(m_device, storage, LIBMTP_FILES_AND_FOLDERS_ROOT, tmpPath, m_lastmode_path);
         }
         long t2 = getSysRunTime();

         LogInfo("end lastmode scan Storage: %s, storage->id: %x, took: %lu ms", storage->StorageDescription, storage->id, (t2 - t1));
      }
   }

   for (storage = m_device->storage; storage != 0; storage = storage->next)
   {
      long t1 = getSysRunTime();
      //get all items
      LogInfo("start scan Storage: %s", storage->StorageDescription);
      string tmpPath = path + storage->StorageDescription;
      if (m_device)
      {
         getAllFolderAndFiles(m_device, storage, LIBMTP_FILES_AND_FOLDERS_ROOT, tmpPath);
      }
      long t2 = getSysRunTime();

      LogInfo("end scan Storage: %s, storage->id: %x, took: %lu ms", storage->StorageDescription, storage->id, (t2 - t1));
   }

   // before list all done, copy the tmp track list to the track list
   pushTrackList(m_tmpTrackMetaList);
   std::vector<smtpfs_trackInfo>().swap(m_tmpTrackMetaList);

   m_list_all_done = 1;
   //if sync done, there is no response to cache command, need to notify
   if(mCacheNewTrackCmd.cmdStatus > SMTP_ASYNC_CMD_STATUS_INIT)
   {
      mFileSystem.onFileInfoAvailable();
   }

   long whole_t2 = getSysRunTime();

   LogInfo("scan whole phone took: %lu ms, total file: %d; file ignored: %u; folder ignored: %u", 
      (whole_t2 - whole_t1), m_trackMetaList.size(), mFileIgnored, mFolderIgnored);

   return 0;
}

static void *autoScanItemsRun(void *arg)
{
   LogInfo("Scan Thread Enter");
   if (!gMTPDev)
   {
      LogInfo("gMTPDev is NULL");
      return NULL;
   }

   do
   {
      LogInfo("wait sem");
      sem_wait(&gMTPDev->device_connect_sem);
      LogInfo("wait sem, reach");
      if (gMTPDev)
         gMTPDev->autoListAll();
   } while (0);

   LogInfo("Scan Thread Exit");

   return NULL;
}

int MTPDevice::launchTasks()
{
   LogInfo("enter");

   if (!scanThreadRun)
   {
      //detached thread
      pthread_attr_t attr;
      int ret = 0;
      pthread_t pid;

      pthread_attr_init(&attr);

      ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      if (ret)
      {
         LogInfo("pthread_attr_setdetachstate fail");
         return -1;
      }

      scanThreadRun = true;
      mScanAbort = false;
      ret = pthread_create(&pid, &attr, autoScanItemsRun, this);
      if (ret)
      {
         LogError("pthread_create failed\n");
         return -1;
      }

      pthread_attr_destroy(&attr);
   }

   LogInfo("exit");
   return 0;
}

int MTPDevice::abortTask()
{
   LogInfo("try to abort the MTP scanning thread");
   mScanAbort = true;

   return 0;
}

bool MTPDevice::isMediaType(const std::string &path)
{
   return (MTP_FILE_TYPE_UNKNOWN != getMediaFileType(path));
}

void MTPDevice::printTrackInfo(const smtpfs_trackInfo *track)
{
   if (track == NULL)
      return;

   LogInfo("---------------track Info-------------");

   string typeStr = "unknown";
   if (track->type == MTP_FILE_TYPE_AUDIO)
   {
      typeStr = "audio";
   }
   else if (track->type == MTP_FILE_TYPE_VIDEO)
   {
      typeStr = "video";
   }
   else if (track->type == MTP_FILE_TYPE_IMAGE)
   {
      typeStr = "image";
   }
   else if (track->type == MTP_FILE_TYPE_PLAYLIST)
   {
      typeStr = "playlist";
   }
   else if (track->type == MTP_FILE_TYPE_AUDIOBOOKS)
   {
      typeStr = "audiobooks";
   }
   else
   {
      typeStr = "unknown";
   }

   LogInfo("path: %s", track->path);
   LogInfo("type: %s", typeStr.c_str());
   LogInfo("title: %s", track->title);
   LogInfo("artist: %s", track->artist);
   //LogInfo("composer: %s", track->composer);
   LogInfo("genre: %s", track->genre);
   LogInfo("album: %s", track->album);
   LogInfo("duration: %u", track->duration);
   LogInfo("samplerate: %u", track->samplerate);
   LogInfo("nochannels: %u", track->nochannels);
   LogInfo("wavecodec: %u", track->wavecodec);
   LogInfo("bitrate: %u", track->bitrate);
   LogInfo("filesize: %llu", track->filesize);
   LogInfo("modificationdate: %lu", track->modificationdate);
   LogInfo("tracknumber: %d", track->tracknumber);
   LogInfo("rating: %d", track->rating);
}

int MTPDevice::fillTrackInfo(smtpfs_trackInfo *dst, const LIBMTP_track_t *src, const char *path, int type)
{
   const char *ignoreKey = "<unknown>";
   if (!dst || !path || !src)
   {
      LogError("dst or path is NULL");
      return -1;
   }

   dst->type = type;

   if (path)
      strncpy(dst->path, path, (sizeof(dst->path) - 1));

   // if(src->filename)
   // 	strncpy(dst->fileName, src->filename, (sizeof(dst->fileName) - 1));

   if (src->title)
      strncpy(dst->title, src->title, (sizeof(dst->title) - 1));

   if (src->artist && strcmp(src->artist, ignoreKey))
      strncpy(dst->artist, src->artist, (sizeof(dst->artist) - 1));

   // if(src->composer)
   // 	strncpy(dst->composer, src->composer, (sizeof(dst->composer) - 1));

   if (src->genre && strcmp(src->genre, ignoreKey))
      strncpy(dst->genre, src->genre, (sizeof(dst->genre) - 1));

   if (src->album && strcmp(src->album, ignoreKey))
      strncpy(dst->album, src->album, (sizeof(dst->album) - 1));

   dst->duration = src->duration;
   dst->samplerate = src->samplerate;
   dst->nochannels = src->nochannels;
   dst->wavecodec = src->wavecodec;
   dst->bitrate = src->bitrate;
   dst->filesize = src->filesize;
   dst->modificationdate = src->modificationdate;
   dst->tracknumber = src->tracknumber;
   dst->rating = src->rating;

   //printTrackInfo(dst);

   return 0;
}

int MTPDevice::fillTrackInfoByFile(smtpfs_trackInfo *dst, const LIBMTP_file_t *src, const char *path, int type)
{
   if (!dst || !path || !src)
   {
      LogError("dst or path is NULL");
      return -1;
   }

   dst->type = type;

   if (path)
      strncpy(dst->path, path, (sizeof(dst->path) - 1));

   dst->filesize = src->filesize;
   dst->modificationdate = src->modificationdate;

   //printTrackInfo(dst);

   return 0;
}

int32_t MTPDevice::getTrackList(std::vector<smtpfs_trackInfo> &trackList, int32_t count)
{
   trackList.clear();
   if (isIndexingDone)
   {
      LogInfo("the scanning is done, and all of the track info were consumed");
      return -1;
   }
   else
   {
      if (!m_list_all_done && !hasAvailableTrack())
      {
         // pending
         return 0;
      }
      popTrackList(trackList, count);
      if (m_list_all_done && !hasAvailableTrack())
      {
         LogInfo("isIndexingDone is true");
         isIndexingDone = true;
         clearTrackList();
      }
   }

   return (int32_t)trackList.size();
}

int32_t MTPDevice::getFriendlyName(char *name, uint32_t len)
{
   char* mtpName = LIBMTP_Get_Friendlyname(m_device);
   if (NULL == mtpName)
   {
      LogError("getFriendlyName LIBMTP_Get_Friendlyname with NULL!");
      return -1;
   }
   LogInfo("MTPDevice::getFriendlyName, %s", mtpName);
   uint32_t length = strlen(mtpName);
   if (length >= len)
   {
      memcpy(name, mtpName, len-1);
      name[len-1]='\0';
      length = len;
   }
   else
   {
      memcpy(name, mtpName, length+1);
   }
   free(mtpName);
   
   return length;
}

int MTPDevice::getEvent(char *data, uint32_t len)
{
   if (!data || (len < (int)sizeof(smtpfs_event)))
   {
      LogError("data: %p, len: %d", data, len);
      return -1;
   }

   uint32_t offset = 0;
   //fill cache command response data
   if(SMTP_ASYNC_CMD_STATUS_FINISH == mCacheNewTrackCmd.cmdStatus)
   {
      if(len < (sizeof(smtpfs_event) + sizeof(int)))
      {
         LogError("len[%u] is too small", len);
         return -1;
      }
      smtpfs_event *pEvent = (smtpfs_event *)(data + offset);
      pEvent->eventType = SMTPFS_EVENT_CACHE_TRACK_RSP;
      pEvent->len = sizeof(smtpfs_event);

      int *status = (int *)(data + offset + pEvent->len);
      *status = mCacheNewTrackCmd.responseData;
      pEvent->len += sizeof(int);
      offset = pEvent->len;
      mCacheNewTrackCmd = tAsynCommand(SMTPFS_EVENT_CACHE_NEW_TRACK);
   }

   //fill the track list
   {
      uint32_t rLen = len - offset;
      if (rLen < (sizeof(smtpfs_event) + sizeof(smtpfs_trackList) + sizeof(smtpfs_trackInfo)))
      {
         LogError("rLen[%u] is too small", rLen);
         return (offset > 0) ? offset : -1;
      }
      
      int32_t trackCount = (rLen - sizeof(smtpfs_event) - sizeof(smtpfs_trackList)) / sizeof(smtpfs_trackInfo);
      std::vector<smtpfs_trackInfo> trackList;
      int32_t ret = 0;

      ret = getTrackList(trackList, trackCount);

      smtpfs_event *pEvent = (smtpfs_event *)(data + offset);
      pEvent->eventType = SMTPFS_EVENT_TRACKLIST;
      pEvent->len = sizeof(smtpfs_event);

      smtpfs_trackList *trackListHeader = (smtpfs_trackList *)(data + offset + pEvent->len);
      trackListHeader->fragment = 1;
      trackListHeader->count = 0;
      pEvent->len += sizeof(smtpfs_trackList);

      if (ret < 0)
      {
         LogInfo("getTrackList failed; there is no track info any more; ret: %d", ret);
         trackListHeader->fragment = 0;
      }
      else if (trackList.empty())
      {
         LogDebug("not track info available, pending");
         pEvent->len = 0;
      }
      else
      {
         // fill the track list
         smtpfs_trackInfo *track = (smtpfs_trackInfo *)(data + offset + pEvent->len);
         for (auto it = trackList.begin(); it != trackList.end(); ++ it)
         {
            *track = *it;
            ++ track;
            ++ trackListHeader->count;
            pEvent->len += sizeof(smtpfs_trackInfo);
         }
         LogDebug("track info count: %d; len: %u", trackListHeader->count, pEvent->len);
      }
      offset += pEvent->len;
   }
   return offset;
}

int MTPDevice::setEvent(char *buf, uint32_t bufLen)
{
   if (!buf || (bufLen < (int)sizeof(smtpfs_event)))
   {
      LogError("buf: %p, bufLen: %d", buf, bufLen);
      return -1;
   }

   //event header
   smtpfs_event *pEvent = (smtpfs_event *)buf;
   if (!pEvent)
   {
      LogError("pEvent is NULL");
      return -1;
   }

   if (pEvent->eventType == SMTPFS_EVENT_SYNC_LASTMODE)
   {
      smtpfs_sync_lastmode *lastModeEvent = (smtpfs_sync_lastmode *)(buf + sizeof(smtpfs_event));
      if (!lastModeEvent)
      {
         LogError("lastModeEvent is NULL");
         return -1;
      }

      setLastModeSyncPath(lastModeEvent->path);
   }
   else if (pEvent->eventType == SMTPFS_EVENT_CACHE_NEW_TRACK)
   {
      if(Config::instance().getEnableCacheFirstFile())
      {
         if(m_list_all_done)
         {
            LogInfo("The scanning is done, there is no need to cache the track!");
            return -1;
         }
         if(mNeedCacheFile && (0 == m_tmpTrackMetaList.size()))
         {
            LogInfo("LastMode or First track is on going, no need to cache the track!");
            return -1;
         }

         if(SMTP_ASYNC_CMD_STATUS_INIT == mCacheNewTrackCmd.cmdStatus)
         {
            //smtpfs_sync_lastmode *cacheEvent = (smtpfs_sync_lastmode *)(buf + sizeof(smtpfs_event));
            mCacheNewTrackCmd.cmdStatus = SMTP_ASYNC_CMD_STATUS_WAIT;
         }
      }
      else
      {
         LogInfo("Not enable filecache solution,so no need to cache the track!");
         return -1;
      }
   }

   return 0;
}

int MTPDevice::setLastModeSyncPath(string path)
{
   if (path.empty())
   {
      LogError("path is empty");
      return -1;
   }
   // initialize scanning, get config from MediaOne
   std::string logLevelSMTP;
   std::string pattern;
   std::vector<std::string> extNameAudio = Config::instance().getExtNameAudio();
   std::vector<std::string> extNameVideo = Config::instance().getExtNameVideo();
   std::vector<std::string> extNameImage = Config::instance().getExtNameImage();
   std::vector<std::string> extNamePlaylist = Config::instance().getExtNamePlaylist();
   std::vector<std::string> extNameAudiobook = Config::instance().getExtNameAudiobook();

   // set the log level
   Logger::setLogLevel(Config::instance().getSMTPFSLogLevel());

   if (!makeRegExStrArray(mReExtNameAudio, extNameAudio))
   {
      LogError("make regEx for audio extension name failed");
   }
   if (!makeRegExStrArray(mReExtNameVideo, extNameVideo))
   {
      LogError("make regEx for video extension name failed");
   }
   if (!makeRegExStrArray(mReExtNameImage, extNameImage))
   {
      LogError("make regEx for image extension name failed");
   }
   if (!makeRegExStrArray(mReExtNamePlaylist, extNamePlaylist))
   {
      LogError("make regEx for playlist extension name failed");
   }
   if (!makeRegExStrArray(mReExtNameAudiobook, extNameAudiobook))
   {
      LogError("make regEx for audiobook extension name failed");
   }

   mScanThPolicy = Config::instance().getScanThrPolicy();
   mScanThPriority = Config::instance().getScanThrPriority();

   mNeedCacheFile = Config::instance().getEnableCacheFirstFile();
   // end of initialize


   LogInfo("lastmode path: %s", path.c_str());

   m_lastmode_path = path;

   sem_post(&device_connect_sem);

   return 0;
}

void MTPDevice::clearTrackList()
{
   m_mutexTrackList.lock();

   m_trackMetaList.clear();
   vector<smtpfs_trackInfo>().swap(m_trackMetaList);

   m_mutexTrackList.unlock();
}

void MTPDevice::pushTrackInfo(const smtpfs_trackInfo &trackInfo)
{
   pushTrackList(std::vector<smtpfs_trackInfo>(1, trackInfo));
}

void MTPDevice::pushTrackList(const std::vector<smtpfs_trackInfo> &trackList)
{
   m_mutexTrackList.lock();

   m_trackMetaList.insert(m_trackMetaList.end(), trackList.begin(), trackList.end());

   m_mutexTrackList.unlock();

   // notify filesystem, there might be some io request waiting
   mFileSystem.onFileInfoAvailable();
}

int32_t MTPDevice::popTrackList(std::vector<smtpfs_trackInfo> &trackList, int32_t count)
{
   trackList.clear();
   m_mutexTrackList.lock();

   int32_t cntAvaMeta = (int32_t)m_trackMetaList.size() - (int32_t)m_trackList_offset;

   count = std::min(count, cntAvaMeta);
   if (count > 0)
   {
      auto it = m_trackMetaList.begin() + m_trackList_offset;

      trackList.assign(it, it + count);
      m_trackList_offset += count;
   }

   m_mutexTrackList.unlock();

   return count;
}

bool MTPDevice::hasAvailableTrack()
{
   bool ava = false;

   m_mutexTrackList.lock();

   ava = (m_trackList_offset < m_trackMetaList.size());

   m_mutexTrackList.unlock();

   return ava;
}