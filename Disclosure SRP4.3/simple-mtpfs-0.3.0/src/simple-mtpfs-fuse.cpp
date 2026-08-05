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
#include <iostream>
extern "C" {
#include <errno.h>
#include <fuse/fuse_opt.h>
#include <unistd.h>
#include <stddef.h>
#include <poll.h>
}
#include "simple-mtpfs-fuse.h"
#include "simple-mtpfs-config.h"
#include "simple-mtpfs-log.h"
#include "simple-mtpfs-util.h"

#define FD_SMTP_IOCTL_NODE       ((uint64_t)0)
#define FD_SMTP_DEF_FILE_DESC    ((uint64_t)0)

int wrap_getattr(const char *path, struct stat *statbuf)
{
   LogDebug("enter getattr path:%s", path);
   return SMTPFileSystem::instance()->getattr(path, statbuf);
}

int wrap_mknod(const char *path, mode_t mode, dev_t dev)
{
   return SMTPFileSystem::instance()->mknod(path, mode, dev);
}

int wrap_mkdir(const char *path, mode_t mode)
{
   return SMTPFileSystem::instance()->mkdir(path, mode);
}

int wrap_unlink(const char *path)
{
   return SMTPFileSystem::instance()->unlink(path);
}

int wrap_rmdir(const char *path)
{
   return SMTPFileSystem::instance()->rmdir(path);
}

int wrap_rename(const char *path, const char *newpath)
{
   return SMTPFileSystem::instance()->rename(path, newpath);
}

int wrap_chmod(const char *path, mode_t mode)
{
   return SMTPFileSystem::instance()->chmod(path, mode);
}

int wrap_chown(const char *path, uid_t uid, gid_t gid)
{
   return SMTPFileSystem::instance()->chown(path, uid, gid);
}

int wrap_truncate(const char *path, off_t new_size)
{
   return SMTPFileSystem::instance()->truncate(path, new_size);
}

int wrap_utime(const char *path, struct utimbuf *ubuf)
{
   return SMTPFileSystem::instance()->utime(path, ubuf);
}

int wrap_open(const char *path, struct fuse_file_info *file_info)
{
   return SMTPFileSystem::instance()->open(path, file_info);
}

int wrap_access(const char *path, int mask)
{
   LogDebug("enter wrap_access path:%s", path);
   return SMTPFileSystem::instance()->access(path, mask);
}

int wrap_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_read[%s], size is :%d offset is:%d", path, size, offset);
   return SMTPFileSystem::instance()->read(path, buf, size, offset, file_info);
}

int wrap_readbuf(const char *path, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *fileinfo)
{
   LogDebug("enter wrap_readbuf[%s], size is :%d offset is:%d", path, size, off);
   int readsize = 0;
   struct fuse_bufvec *fbuf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
   if (!fbuf)
      return -ENOENT;
   *fbuf = FUSE_BUFVEC_INIT(size);
   void *mem = NULL;
   mem = malloc(size);
   if (!mem)
      return -ENOENT;
   fbuf->buf[0].mem = mem;
   readsize = SMTPFileSystem::instance()->read(path, (char *)mem, size, off, fileinfo);

   *bufp = fbuf;
   return readsize;
}

int wrap_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *file_info)
{
   return SMTPFileSystem::instance()->write(path, buf, size, offset, file_info);
}

int wrap_statfs(const char *path, struct statvfs *stat_info)
{
   LogDebug("enter wrap_statfs, path:%s", path);
   return SMTPFileSystem::instance()->statfs(path, stat_info);
}

int wrap_flush(const char *path, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_flush, path:%s", path);
   return SMTPFileSystem::instance()->flush(path, file_info);
}

int wrap_release(const char *path, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_release, path:%s", path);
   //return SMTPFileSystem::instance()->release(path, file_info);
   return 0;
}

int wrap_fsync(const char *path, int datasync, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_fsync, path:%s", path);
   return SMTPFileSystem::instance()->fsync(path, datasync, file_info);
}

int wrap_opendir(const char *path, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_opendir, path:%s", path);
   return SMTPFileSystem::instance()->opendir(path, file_info);
}

int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_readdir, path:%s, offset:%lld", path, offset);
   return SMTPFileSystem::instance()->readdir(path, buf, filler,
                                              offset, file_info);
}

int wrap_releasedir(const char *path, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_releasedir, path:%s", path);
   return SMTPFileSystem::instance()->releasedir(path, file_info);
}

int wrap_fsyncdir(const char *path, int datasync, struct fuse_file_info *file_info)
{
   LogDebug("enter wrap_fsyncdir, path:%s", path);
   return SMTPFileSystem::instance()->fsyncdir(path, datasync, file_info);
}

void *wrap_init(struct fuse_conn_info *conn)
{
   LIBMTP_DLT_reset();
   return SMTPFileSystem::instance()->init(conn);
}

int wrap_create(const char *path, mode_t mode, fuse_file_info *file_info)
{
   return SMTPFileSystem::instance()->create(path, mode, file_info);
}

int wrap_ftruncate(const char *path, off_t offset, struct fuse_file_info *file_info)
{
   return SMTPFileSystem::instance()->ftruncate(path, offset, file_info);
}

int wrap_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *file_info, unsigned int flags, void *data)
{
   LogDebug("path:%s", path);
   return SMTPFileSystem::instance()->ioctl(path, cmd, arg, file_info, flags, data);
}

int wrap_poll(const char *path, struct fuse_file_info *file_info, struct fuse_pollhandle *ph, unsigned *reventsp)
{
   LogDebug("path:%s", path);
   return SMTPFileSystem::instance()->poll(path, file_info, ph, reventsp);
}

SMTPFileSystem::SMTPFileSystemOptions::SMTPFileSystemOptions()
    : m_good(false), m_help(false), m_version(false), m_verbose(false), m_enable_move(false), m_list_devices(false), m_device_no(1), m_device_file(nullptr), m_mount_point(nullptr)
{
}

SMTPFileSystem::SMTPFileSystemOptions::~SMTPFileSystemOptions()
{
   free(static_cast<void *>(m_device_file));
   free(static_cast<void *>(m_mount_point));
}

// -----------------------------------------------------------------------------

int SMTPFileSystem::SMTPFileSystemOptions::opt_proc(void *data, const char *arg, int key,
                                                    struct fuse_args *outargs)
{
   SMTPFileSystemOptions *options = static_cast<SMTPFileSystemOptions *>(data);

   if (key == FUSE_OPT_KEY_NONOPT)
   {
      if (options->m_mount_point && options->m_device_file)
      {
         // Unknown positional argument supplied
         return -1;
      }
      if (options->m_device_file)
      {
         fuse_opt_add_opt(&options->m_mount_point, arg);
         return 0;
      }

      fuse_opt_add_opt(&options->m_device_file, arg);
      return 0;
   }
   return 1;
}

std::unique_ptr<SMTPFileSystem> SMTPFileSystem::s_instance;

SMTPFileSystem *SMTPFileSystem::instance()
{
   if (!s_instance.get())
      s_instance.reset(new SMTPFileSystem());
   return s_instance.get();
}

SMTPFileSystem::SMTPFileSystem() : m_args(),
                                   m_tmp_files_pool(),
                                   m_options(),
                                   m_mutex_mediaone_ioctl(),
                                   m_ph_mediaone_ioctl(NULL),
                                   m_device(*this)
//  selectFd(0LL)
{
   m_fuse_operations.getattr = wrap_getattr;
   m_fuse_operations.readlink = nullptr;
   m_fuse_operations.getdir = nullptr;
   m_fuse_operations.mknod = nullptr;  //wrap_mknod;
   m_fuse_operations.mkdir = nullptr;  //wrap_mkdir;
   m_fuse_operations.unlink = nullptr; //wrap_unlink;
   m_fuse_operations.rmdir = nullptr;  //wrap_rmdir;
   m_fuse_operations.symlink = nullptr;
   m_fuse_operations.rename = nullptr; //wrap_rename;
   m_fuse_operations.link = nullptr;
   m_fuse_operations.chmod = nullptr;    //wrap_chmod;
   m_fuse_operations.chown = nullptr;    //wrap_chown;
   m_fuse_operations.truncate = nullptr; //wrap_truncate;
   m_fuse_operations.utime = nullptr;    //wrap_utime;
   m_fuse_operations.open = wrap_open;
   m_fuse_operations.read = wrap_read;
   m_fuse_operations.read_buf = wrap_readbuf; //nullptr;//wrap_readbuf;
   m_fuse_operations.write = nullptr;         //wrap_write;
   m_fuse_operations.statfs = wrap_statfs;
   m_fuse_operations.flush = wrap_flush;
   m_fuse_operations.release = wrap_release;
   m_fuse_operations.fsync = wrap_fsync;
   m_fuse_operations.setxattr = nullptr;
   m_fuse_operations.getxattr = nullptr;
   m_fuse_operations.listxattr = nullptr;
   m_fuse_operations.removexattr = nullptr;
   m_fuse_operations.opendir = wrap_opendir;
   m_fuse_operations.readdir = wrap_readdir;
   m_fuse_operations.releasedir = nullptr; //wrap_releasedir;
   m_fuse_operations.fsyncdir = nullptr;   //wrap_fsyncdir;
   m_fuse_operations.init = wrap_init;
   m_fuse_operations.destroy = nullptr;
   m_fuse_operations.access = wrap_access;
   m_fuse_operations.create = nullptr;    //wrap_create;
   m_fuse_operations.ftruncate = nullptr; //wrap_ftruncate;
   m_fuse_operations.fgetattr = nullptr;
   m_fuse_operations.ioctl = wrap_ioctl;
   m_fuse_operations.poll = wrap_poll;
}

SMTPFileSystem::~SMTPFileSystem()
{
   fuse_opt_free_args(&m_args);
   //  selectFd  = 0LL;
}

bool SMTPFileSystem::parseOptions(int argc, char **argv)
{
#define SMTPFS_OPT_KEY(t, p, v)                \
   {                                           \
      t, offsetof(SMTPFileSystemOptions, p), v \
   }

   static struct fuse_opt smtpfs_opts[] = {
       SMTPFS_OPT_KEY("enable-move", m_enable_move, 1),
       SMTPFS_OPT_KEY("--device %i", m_device_no, 0),
       SMTPFS_OPT_KEY("-l", m_list_devices, 1),
       SMTPFS_OPT_KEY("--list-devices", m_list_devices, 1),
       SMTPFS_OPT_KEY("-v", m_verbose, 1),
       SMTPFS_OPT_KEY("--verbose", m_verbose, 1),
       SMTPFS_OPT_KEY("-V", m_version, 1),
       SMTPFS_OPT_KEY("--version", m_version, 1),
       SMTPFS_OPT_KEY("-h", m_help, 1),
       SMTPFS_OPT_KEY("--help", m_help, 1),
       FUSE_OPT_END};

   if (argc < 2)
   {
      LogError("Wrong usage");
      m_options.m_good = false;
      return false;
   }

   fuse_opt_free_args(&m_args);
   m_args = FUSE_ARGS_INIT(argc, argv);
   if (fuse_opt_parse(&m_args, &m_options, smtpfs_opts, SMTPFileSystemOptions::opt_proc) == -1)
   {
      m_options.m_good = false;
      return false;
   }

   if (m_options.m_version || m_options.m_help || m_options.m_list_devices)
   {
      m_options.m_good = true;
      return true;
   }

   if (m_options.m_device_file && !m_options.m_mount_point)
   {
      m_options.m_mount_point = m_options.m_device_file;
      m_options.m_device_file = nullptr;
   }

   if (!m_options.m_mount_point)
   {
      LogError("Mount point missing");
      m_options.m_good = false;
      return false;
   }

   fuse_opt_add_arg(&m_args, m_options.m_mount_point);
   fuse_opt_add_arg(&m_args, "-s");

   if (m_options.m_verbose)
   {
      fuse_opt_add_arg(&m_args, "-f");
   }

   --m_options.m_device_no;

   // device file and -- device are mutually exclusive, fail if both set
   if (m_options.m_device_no && m_options.m_device_file)
   {
      m_options.m_good = false;
      return false;
   }

   m_options.m_good = true;
   return true;
}

void SMTPFileSystem::printHelp() const
{
   struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
   struct fuse_operations tmp_operations;
   memset(&tmp_operations, 0, sizeof(tmp_operations));
   std::cerr << "usage: " << smtpfs_basename(m_args.argv[0])
             << " <source> mountpoint [options]\n\n"
             << "general options:\n"
             << "    -o opt,[opt...]        mount options\n"
             << "    -h   --help            print help\n"
             << "    -V   --version         print version\n\n"
             << "simple-mtpfs options:\n"
             << "    -v   --verbose         verbose output, implies -f\n"
             << "    -l   --list-devices    print available devices. Supports <source> option\n"
             << "         --device          select a device number to mount\n"
             << "    -o enable-move         enable the move operations\n\n";
   fuse_opt_add_arg(&args, m_args.argv[0]);
   fuse_opt_add_arg(&args, "-ho");
   fuse_main(args.argc, args.argv, &tmp_operations, nullptr);
   fuse_opt_free_args(&args);
   std::cerr << "\nReport bugs to <" << PACKAGE_BUGREPORT << ">.\n";
}

void SMTPFileSystem::printVersion() const
{
   struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
   struct fuse_operations tmp_operations;
   memset(&tmp_operations, 0, sizeof(tmp_operations));
   fuse_opt_add_arg(&args, m_args.argv[0]);
   fuse_opt_add_arg(&args, "--version");
   LogInfo("simple-mtpfs version %s",VERSION);
   fuse_main(args.argc, args.argv, &tmp_operations, nullptr);
   fuse_opt_free_args(&args);
}

bool SMTPFileSystem::listDevices() const
{
   const std::string dev_file = m_options.m_device_file ? m_options.m_device_file : "";

   return MTPDevice::listDevices(m_options.m_verbose, dev_file);
}

bool SMTPFileSystem::exec()
{
   if (!m_options.m_good)
      return false;

   if (m_options.m_version || m_options.m_help)
      return true;

   if (!smtpfs_check_dir(m_options.m_mount_point))
   {
      LogError("Can not mount the device to %s", m_options.m_mount_point);
      return false;
   }

   if (m_options.m_device_file)
   {
      // Try to use device file first, if provided
      if (!m_device.connect(m_options.m_device_file))
         return false;
   }
   else
   {
      // Connect to MTP device by order number, if no device file supplied
      if (!m_device.connect(m_options.m_device_no))
         return false;
   }
   m_device.enableMove(m_options.m_enable_move);
   if (fuse_main(m_args.argc, m_args.argv, &m_fuse_operations, nullptr) > 0)
   {
      LogInfo("fuse_main return false");
      return false;
   }

   LogInfo("m_device.disconnect");
   m_device.disconnect();
   LogInfo("m_device.disconnect ok");

   LogInfo("try to removeTmpDir");
   m_tmp_files_pool.removeTmpDir();
   LogInfo("removeTmpDir ok");

   return true;
}

void *SMTPFileSystem::init(struct fuse_conn_info *conn)
{
   LogInfo("enter init");
   m_device.launchTasks();
   return nullptr;
}

int SMTPFileSystem::getattr(const char *path, struct stat *buf)
{
   string ioctlNode = "/";
   ioctlNode += SMTPFSIOCTLNODE;

   LogDebug("enter, path: %s", path);
   memset(buf, 0, sizeof(struct stat));
   struct fuse_context *fc = fuse_get_context();
   buf->st_uid = fc->uid;
   buf->st_gid = fc->gid;
   if (path == std::string("/"))
   {
      buf->st_mode = S_IFDIR | 0775;
      buf->st_nlink = 2;
      LogDebug("path == '/'");
      return 0;
   }
   else if (path == ioctlNode)
   {
      //LogInfo("smtpfs ioctl node: %s", path);
      buf->st_ino = 0;
      buf->st_size = 128 * 1024;
      buf->st_blocks = 512;
      buf->st_nlink = 1;
      buf->st_mode = S_IFREG | 0644;
      //LogInfo(" %s is fake file", path);
   }
   else
   {
      std::string tmp_path(smtpfs_dirname(path));
      std::string tmp_file(smtpfs_basename(path));
      const TypeDir *content = m_device.dirFetchContentFromCache(tmp_path);
      if (content)
      {
         if (content->dir(tmp_file))
         {
            const TypeDir *dir = content->dir(tmp_file);
            buf->st_ino = dir->id();
            buf->st_mode = S_IFDIR | 0775;
            buf->st_nlink = 2;
            buf->st_mtime = dir->modificationDate();
            LogDebug("path:%s is dir", path);
            return 0;
         }
         else if (content->file(tmp_file))
         {
            const TypeFile *file = content->file(tmp_file);
            buf->st_ino = file->id();
            buf->st_size = file->size();
            buf->st_blocks = (file->size() / 512) + (file->size() % 512 > 0 ? 1 : 0);
            buf->st_nlink = 1;
            buf->st_mode = S_IFREG | 0644;
            buf->st_mtime = file->modificationDate();
            buf->st_ctime = buf->st_mtime;
            buf->st_atime = buf->st_mtime;
            LogDebug("path:%s is file", path);
            //LogInfo("%s size: %llu", path, buf->st_size);
            return 0;
         }
         else
         {
            LogWarning("did not find the file: %s", path);
         }
      }
      else
      {
         LogWarning("did not find the dir: %s;", tmp_path.c_str());
      }

      bool isFile = m_device.isMediaType(path);

      if (isFile)
      {
// LogInfo("query file %s from db", path);
#ifdef MTP_HAS_DB
         //find the file from smtpfs.db
         uint32_t fileId = 0;
         string pathStr = path;
         m_device.queryFileID(pathStr, m_device.getUUID(), &fileId);

         //get file info by database
         if (fileId && !m_device.getFileInfoById(fileId, buf))
         {
            LogInfo("file %s exist in db", path);
            return 0;
         }
         //if the file dose not exist in database
         else
#endif
         {
            LogError("file %s not exist in db", path);
            return -ENOENT;
         }
      }
      else
      {
         //check if the dir exist in database
         buf->st_mode = S_IFDIR | 0775;
         buf->st_nlink = 2;
         buf->st_mtime = time(NULL);
         LogInfo(" %s is fake dir", path);
      }
   }

   LogDebug("exit");

   return 0;
}

int SMTPFileSystem::mknod(const char *path, mode_t mode, dev_t dev)
{
   return -EINVAL;
   // if (!S_ISREG(mode))
   //    return -EINVAL;

   // std::string tmp_path = m_tmp_files_pool.makeTmpPath(std::string(path));
   // int rval = ::open(tmp_path.c_str(), O_CREAT | O_WRONLY, mode);
   // if (rval < 0)
   //    return -errno;

   // rval = ::close(rval);
   // if (rval < 0)
   //    return -errno;

   // m_device.filePush(tmp_path, std::string(path));
   // ::unlink(tmp_path.c_str());
   // return 0;
}

int SMTPFileSystem::mkdir(const char *path, mode_t mode)
{
   return m_device.dirCreateNew(std::string(path));
}

int SMTPFileSystem::unlink(const char *path)
{
   return m_device.fileRemove(std::string(path));
}

int SMTPFileSystem::rmdir(const char *path)
{
   return m_device.dirRemove(std::string(path));
}

int SMTPFileSystem::rename(const char *path, const char *newpath)
{
   return -EINVAL;
   // const std::string tmp_old_dirname(smtpfs_dirname(std::string(path)));
   // const std::string tmp_new_dirname(smtpfs_dirname(std::string(newpath)));
   // if (tmp_old_dirname == tmp_new_dirname)
   //    return m_device.rename(std::string(path), std::string(newpath));

   // if (!m_options.m_enable_move)
   //    return -EPERM;

   // const std::string tmp_file = m_tmp_files_pool.makeTmpPath(std::string(newpath));
   // int rval = m_device.filePull(std::string(path), tmp_file);
   // if (rval != 0)
   //    return -rval;

   // rval = m_device.filePush(tmp_file, std::string(newpath));
   // if (rval != 0)
   //    return -rval;

   // rval = m_device.fileRemove(std::string(path));
   // if (rval != 0)
   //    return -rval;

   // return 0;
}

int SMTPFileSystem::chmod(const char *path, mode_t mode)
{
   return 0;
}

int SMTPFileSystem::chown(const char *path, uid_t uid, gid_t gid)
{
   return 0;
}

int SMTPFileSystem::truncate(const char *path, off_t new_size)
{
   return -EINVAL;
   // const std::string tmp_path = m_tmp_files_pool.makeTmpPath(std::string(path));
   // int rval = m_device.filePull(std::string(path), tmp_path);
   // if (rval != 0)
   // {
   //    ::unlink(tmp_path.c_str());
   //    return -rval;
   // }

   // rval = ::truncate(tmp_path.c_str(), new_size);
   // if (rval != 0)
   // {
   //    int errno_tmp = errno;
   //    ::unlink(tmp_path.c_str());
   //    return -errno_tmp;
   // }

   // rval = m_device.fileRemove(std::string(path));
   // if (rval != 0)
   // {
   //    ::unlink(tmp_path.c_str());
   //    return -rval;
   // }

   // rval = m_device.filePush(tmp_path, std::string(path));
   // ::unlink(tmp_path.c_str());

   // if (rval != 0)
   //    return -rval;

   // return 0;
}

int SMTPFileSystem::utime(const char *path, struct utimbuf *ubuf)
{
   std::string tmp_basename(smtpfs_basename(std::string(path)));
   std::string tmp_dirname(smtpfs_dirname(std::string(path)));

   const TypeDir *parent = m_device.dirFetchContentFromCache(tmp_dirname);
   if (!parent)
      return -ENOENT;

   const TypeFile *file = parent->file(tmp_basename);
   if (!file)
      return -ENOENT;

   const_cast<TypeFile *>(file)->setModificationDate(ubuf->modtime);

   return 0;
}

int SMTPFileSystem::create(const char *path, mode_t mode, fuse_file_info *file_info)
{
   return -EINVAL;
   // const std::string tmp_path = m_tmp_files_pool.makeTmpPath(std::string(path));

   // int rval = ::creat(tmp_path.c_str(), mode);
   // if (rval < 0)
   //    return -errno;

   // file_info->fh = rval;
   // m_tmp_files_pool.addFile(TypeTmpFile(std::string(path), tmp_path, rval, true));
   // m_device.filePush(tmp_path, std::string(path));

   // return 0;
}

int SMTPFileSystem::open(const char *path, struct fuse_file_info *file_info)
{
#if 1
   string ioctlNode = "/";
   ioctlNode += SMTPFSIOCTLNODE;

   if (!path || !file_info)
   {
      LogError("pointer NULL, path: %p, file_info: %p", path, file_info);
      return -1;
   }

   LogDebug("open path: %s", path);
   if (path == ioctlNode)
   {
      file_info->fh = FD_SMTP_IOCTL_NODE;
      // if(!selectFd)
      // {
      // 	uint64_t handle = 0x0807060504030201;
      // 	//LogInfo("enter open path:%s, fh: %llx --> %llx,", path, file_info->fh, handle);
      // 	file_info->fh = handle;
      // 	selectFd = handle;
      // }
      // else
      // {
      // 	LogWarning("open control node had been open, please check if multi-open");
      // }
   }
   else
   {
      // check if the file be cached
      const std::string std_path(path);
      TypeTmpFile *tmp_file = const_cast<TypeTmpFile *>(m_tmp_files_pool.getFile(std_path));

      if (NULL == tmp_file)
      {
         LogDebug("open file from device directly; path:%s", path);
         // the file was not local cached, we need to request IO from the MTP device directly
         file_info->fh = FD_SMTP_DEF_FILE_DESC;
      }
      else
      {
         // the file was local cached, we could request IO from local file
         std::string tmp_path = tmp_file->pathTmp();
         int fd = ::open(tmp_path.c_str(), file_info->flags);

         if (fd < 0)
         {
            LogError("open the cached file failed, the cached file would be removed; %s; tmp file: %s", strerror(errno), tmp_path.c_str());
            //::unlink(tmp_path.c_str());
            return -errno;
         }
         file_info->fh = fd;
         tmp_file->addFileDescriptor(fd);
         LogDebug("open the cached file successfully. source: %s; tmp file: %s", std_path.c_str(), tmp_path.c_str());
      }
   }

   return 0;
#else
   if (file_info->flags & O_WRONLY)
      file_info->flags |= O_TRUNC;

   const std::string std_path(path);

   TypeTmpFile *tmp_file = const_cast<TypeTmpFile *>(
       m_tmp_files_pool.getFile(std_path));

   std::string tmp_path;
   if (tmp_file)
   {
      tmp_path = tmp_file->pathTmp();
   }
   else
   {
      tmp_path = m_tmp_files_pool.makeTmpPath(std_path);

      int rval = m_device.filePull(std_path, tmp_path);
      if (rval != 0)
         return -rval;
   }

   int fd = ::open(tmp_path.c_str(), file_info->flags);
   if (fd < 0)
   {
      ::unlink(tmp_path.c_str());
      return -errno;
   }

   file_info->fh = fd;

   if (tmp_file)
      tmp_file->addFileDescriptor(fd);
   else
      m_tmp_files_pool.addFile(TypeTmpFile(std_path, tmp_path, fd));

   return 0;
#endif
}

int SMTPFileSystem::read(const char *path, char *buf, size_t size,
                         off_t offset, struct fuse_file_info *file_info)
{
   int ret = 0;
   const std::string std_path(path);
   string ioctlNode = "/";
   ioctlNode += SMTPFSIOCTLNODE;

   LogDebug("read node: %s, size: %u, offset: %u, fh: %llx", path, size, offset, file_info->fh);

   //if(buf) memset(buf, 0x00, size);

   if (path == ioctlNode)
   {
      //LogInfo("read node: %s, size: %u, offset: %u, fh: %llx", path, size, offset, file_info->fh);
      //ret = m_device.fillEvent(buf, size);
   }
   else
   {
      const std::string std_path(path);
      TypeTmpFile *tmp_file = const_cast<TypeTmpFile *>(m_tmp_files_pool.getFile(std_path));

      if (NULL == tmp_file)
      {
         ret = m_device.filesplitPull(std_path, "", size, offset, (unsigned char *)buf);
         LogDebug("read from device directly: ret: %d", ret);
      }
      else
      {
         ret = ::pread(file_info->fh, buf, size, offset);

         LogDebug("read from cache file: ret: %d", ret);
         if (ret < 0)
         {
            return -errno;
         }
         return ret;
      }
   }
   return ret;
}

int SMTPFileSystem::write(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *file_info)
{
   return -EINVAL;
   // const TypeTmpFile *tmp_file = m_tmp_files_pool.getFile(std::string(path));
   // if (!tmp_file)
   //    return -EINVAL;

   // int rval = ::pwrite(file_info->fh, buf, size, offset);
   // if (rval < 0)
   //    return -errno;

   // const_cast<TypeTmpFile *>(tmp_file)->setModified();
   // return rval;
}

int SMTPFileSystem::release(const char *path, struct fuse_file_info *file_info)
{
   int rval = ::close(file_info->fh);
   if (rval < 0)
      return -errno;

   const std::string std_path(path);
   if (std_path == std::string("-"))
      return 0;

   TypeTmpFile *tmp_file = const_cast<TypeTmpFile *>(m_tmp_files_pool.getFile(std_path));

   if (NULL == tmp_file)
   {
      LogError("get file from pool failed; path: %s", std_path.c_str());
      return 0;
   }
   tmp_file->removeFileDescriptor(file_info->fh);
   if (tmp_file->refcnt() != 0)
   {
      return 0;
   }

   // const bool modif = tmp_file->isModified();
   const std::string tmp_path = tmp_file->pathTmp();

   m_tmp_files_pool.removeFile(std_path);
   // the file could not be modified from MediaOne's side
   // if (modif)
   // {
   //    rval = m_device.filePush(tmp_path, std_path);
   //    if (rval != 0)
   //    {
   //       ::unlink(tmp_path.c_str());
   //       return -rval;
   //    }
   // }

   ::unlink(tmp_path.c_str());

   return 0;
}

int SMTPFileSystem::statfs(const char *path, struct statvfs *stat_info)
{
   uint64_t bs = 1024;
   // XXX: linux coreutils still use bsize member to calculate free space
   stat_info->f_bsize = static_cast<unsigned long>(bs);
   stat_info->f_frsize = static_cast<unsigned long>(bs);
   stat_info->f_blocks = m_device.storageTotalSize() / bs;
   stat_info->f_bavail = m_device.storageFreeSize() / bs;
   stat_info->f_bfree = stat_info->f_bavail;

   LogInfo(" statfs");
   return 0;
}

int SMTPFileSystem::flush(const char *path, struct fuse_file_info *file_info)
{
   return 0;
}

int SMTPFileSystem::fsync(const char *path, int datasync,
                          struct fuse_file_info *fi)
{
   int rval = -1;
#ifdef HAVE_FDATASYNC
   if (datasync)
      rval = ::fdatasync(fi->fh);
   else
#else
   rval = ::fsync(fi->fh);
#endif
       if (rval != 0)
      return -errno;
   return 0;
}

int SMTPFileSystem::opendir(const char *path, struct fuse_file_info *file_info)
{
   LogDebug(" enter opendir, path:%s", path);

   // string ioctlNode = "/";
   // ioctlNode += SMTPFSIOCTLNODE;
   // if(path == ioctlNode)
   // {
   //    LogInfo("ioctl node: %s", path);
   //    return 0;
   // }

   const TypeDir *content = m_device.dirFetchContentFromCache(std::string(path));
   if (!content)
   {
      LogError(" query path: %s is not cached", path);
      return -ENOENT;
   }

   LogDebug(" exit opendir");
   return 0;
}

int SMTPFileSystem::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *file_info)
{
   LogDebug(" enter readdir, path:%s", path);

   const TypeDir *content = m_device.dirFetchContentFromCache(std::string(path));
   if (!content)
   {
      LogError(" query path: %s is not cached", path);
      return -ENOENT;
   }

   const std::vector<TypeDir> dirs = content->dirs();
   const std::vector<TypeFile> files = content->files();

   for (const TypeDir &d : dirs)
   {
      struct stat st;
      memset(&st, 0, sizeof(st));
      st.st_ino = d.id();
      st.st_mode = S_IFDIR | 0775;
      filler(buf, d.name().c_str(), &st, 0);
   }

   for (const TypeFile &f : files)
   {
      struct stat st;
      memset(&st, 0, sizeof(st));
      st.st_ino = f.id();
      st.st_mode = S_IFREG | 0644;
      filler(buf, f.name().c_str(), &st, 0);
   }

   LogDebug("exit readdir");
   return 0;
}

int SMTPFileSystem::releasedir(const char *path, struct fuse_file_info *file_info)
{
   return 0;
}

int SMTPFileSystem::fsyncdir(const char *path, int datasync,
                             struct fuse_file_info *file_info)
{
   return 0;
}

int SMTPFileSystem::ftruncate(const char *path, off_t offset,
                              struct fuse_file_info *file_info)
{
   return -EINVAL;
   // const TypeTmpFile *tmp_file = m_tmp_files_pool.getFile(std::string(path));
   // if (::ftruncate(file_info->fh, offset) != 0)
   //    return -errno;
   // const_cast<TypeTmpFile *>(tmp_file)->setModified();
   // return 0;
}

int SMTPFileSystem::access(const char *path, int mask)
{
   //LogInfo(" enter access, path:%s", path);

   if (!path)
   {
      LogError("path is NULL");
      return -EINVAL;
   }

   bool isInCache = false;
   //1st,check if the path is exist or not from cache pool
   std::string tmp_basename(smtpfs_basename(std::string(path)));
   std::string tmp_dirname(smtpfs_dirname(std::string(path)));
   if (tmp_basename == tmp_dirname)
   {
      LogInfo("basename == dirname");
      return 0;
   }

   if (tmp_basename.empty())
   {
      LogInfo("basename basename is empty");
      return 0;
   }

   const TypeDir *parent = m_device.dirFetchContentFromCache(tmp_dirname);
   if (parent)
   {
      const TypeFile *file = parent->file(tmp_basename);
      if (file)
      {
         LogDebug("path:%s exist in cache", path);
         isInCache = true;
      }
      else
      {
         LogDebug("path:%s not exist in cache", path);
         isInCache = false;
      }
   }

   if (isInCache == false)
   {
      //LogInfo("query from db path:%s ", path);
      uint32_t fileId = 0;
#ifdef MTP_HAS_DB
      string pathStr = path;
      m_device.queryFileID(pathStr, m_device.getUUID(), &fileId);
      if (!fileId)
      {
         LogError("not exist in db, path:%s", path);
         return -ENOENT;
      }

      // while(!m_device.indexingDone())
      // {
      //    if(m_device.isFileExist(fileId))
      //    {
      //       LogInfo(" fileId: %u exist from mtp phone ", fileId);
      //       break;
      //    }
      //    else
      //    {
      //       sleep(1);
      //       LogInfo(" wait fileId: %u exist from mtp phone ", fileId);
      //    }
      // }

      if (!m_device.isFileExist(fileId))
#endif
      {
         LogInfo(" fileId: %u not exist from mtp phone, access failed ", fileId);
         return -ENOENT;
      }
   }

   if (mask & W_OK)
   {
      LogInfo(" access, query W_OK return EINVAL");
      return -EINVAL;
   }

   if (mask & X_OK)
   {
      LogInfo(" access, query X_OK return EINVAL");
      return -EINVAL;
   }

   //LogInfo("exit access");
   return 0;
}

int SMTPFileSystem::ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *file_info, unsigned int flags, void *data)
{
   int ret = 0;
   string ioctlNode = "/";
   ioctlNode += SMTPFSIOCTLNODE;

   LogDebug("enter path:%s", path);

   if (!path || !file_info)
   {
      LogError(" invalid input pointer");
      return -1;
   }

   if (ioctlNode != path)
   {
      return -ENOSYS;
   }

   //LogInfo("path: %s, data: %p, data: %x", path, data, data);

   if (flags & FUSE_IOCTL_COMPAT)
   {
      LogError("not support FUSE_IOCTL_COMPAT");
      return -ENOSYS;
   }

   switch (cmd)
   {
      case SMTPFS_CHECKVERSION:
      {
         ret = ioctlCheckVersion((char *)data, SMTPFS_EVENT_MAXLEN);
      }
      break;

      case SMTPFS_GETEVENT:
      {
         ret = m_device.getEvent((char *)data, SMTPFS_EVENT_MAXLEN);
      }
      break;

      case SMTPFS_SETEVENT:
      {
         LogInfo(" cmd: SMTPFS_SETEVENT, %p, %p", data, arg);
         ret = m_device.setEvent((char *)data, SMTPFS_EVENT_MAXLEN);
      }
      break;

      case SMTPFS_SETCONFIG:
      {
         LogInfo(" cmd: SMTPFS_SETCONFIG, %p, %p", data, arg);
         ret = ioctlSetConfig((char *)data, SMTPFS_EVENT_MAXLEN);
      }
      break;

      case SMTPFS_ABORTSCAN:
      {
         LogInfo("cmd: SMTPFS_ABORTSCAN, abort the MTP scanning");
         ret = m_device.abortTask();
      }
      break;
      
      case SMTPFS_GETFRIENDLYNAME:
      {
         LogInfo("cmd: SMTPFS_GETFRIENDLYNAME, get MTP friendlyname");
         ret = m_device.getFriendlyName((char *)data, MTP_NAME_MAXLEN);
      }
      break;

      default:
      {
         LogWarning(" unsupported cmd: %x", cmd);
         ret = -EINVAL;
      }
      break;
   }
   //LogInfo(" ioctl, ret: %d", ret);

   return ret;
}

int SMTPFileSystem::poll(const char *path, struct fuse_file_info *file_info, struct fuse_pollhandle *ph, unsigned *reventsp)
{
   int ret = 0;
   string ioctlNode = "/";
   ioctlNode += SMTPFSIOCTLNODE;

   //LogInfo("enter path:%s, fh: %llx", path, file_info->fh);

   if (!path || !file_info || !ph || !reventsp)
   {
      LogError(" invalid input pointer");
      return -1;
   }

   if (ioctlNode != path)
   {
      // only process ioctl for mediaone ioctl node
      return -ENOSYS;
   }

   // if(0)//(file_info->fh != selectFd)
   // {
   // 	LogInfo("selectFd[%llx] != fh[%llx], not trigger poll", selectFd, file_info->fh);
   // 	fuse_pollhandle_destroy(ph);
   // 	return 0;
   // }

   //    if(!m_device.canTriggerNotify())
   //    {
   // 	   LogInfo("not trigget poll");
   // 	   fuse_pollhandle_destroy(ph);
   // 	   return 0;
   //    }

   if (m_device.isListAllDone())
   {
      LogDebug("the scanning task was exited, let upper-layer call ioctl.");
      *reventsp |= POLLIN;
      pollClear();
   }
   else
   {
      if (m_device.hasAvailableTrack())
      {
         *reventsp |= POLLIN;
         LogDebug("has available track info, notify app, add POLLIN, *reventsp: %x", *reventsp);
      }
      else
      {
         LogDebug("no track available, do not notify; *reventsp: %x", *reventsp);
         pollRequest(ph);
      }
   }

   //LogInfo("exit path:%s", path);
   return ret;
}

int SMTPFileSystem::onFileCache(const std::string& std_path)
{
   int ret = -1;

   if (!Config::instance().getEnableCacheFirstFile())
   {
      LogInfo("Not enable cache first file");
      return 0;
   }
   if (!m_tmp_files_pool.isInitialized())
   {
      ret = initTmpDir();
      if (0 != ret || !m_tmp_files_pool.isInitialized())
      {
         LogError("initialize tmp dir failed, could not cache file to local; ret: %d", ret);
         return 0;
      }
   }

   std::string local_path = m_tmp_files_pool.makeTmpPath(std_path);
   std::string dir_path(smtpfs_dirname(std_path));
   std::string file_name(smtpfs_basename(std_path));
   const TypeDir *content = m_device.dirFetchContentFromCache(dir_path);
   const TypeFile *file = NULL;

   if (NULL == content)
   {
      LogError("Did not find the dir for cache; %s", dir_path.c_str());
      return -1;
   }
   file = content->file(file_name);
   if (NULL == file)
   {
      LogError("Did not find the file for cache; name: %s; dir path: %s", file_name.c_str(), dir_path.c_str());
      return -1;
   }
   // check if we could pull the file from device to local first
   if (!m_tmp_files_pool.hasFreeSpace(file->size()))
   {
      LogError("Has no free space to cache file: source: %s; local: %s; size: %" PRIu64, std_path.c_str(), local_path.c_str(), file->size());
      return -1;
   }
   ret = m_device.filePull(std_path, local_path);
   if (0 == ret)
   {
      // use the fd as -1 to persist the local cache file, even if the no app call open for this file.
      m_tmp_files_pool.addFile(TypeTmpFile(std_path, local_path, -1));
      LogInfo("Have pulled the file from MTP device to local; source: %s; local: %s; size: %" PRIu64, std_path.c_str(), local_path.c_str(), file->size());
      m_tmp_files_pool.removeOthersExcept(std_path);
   }
   else
   {
      LogError("Pull file to local failed; source: %s; local: %s; size: %" PRIu64 "ret: %x", std_path.c_str(), local_path.c_str(), file->size(), ret);
   }

   return ret;
}

int SMTPFileSystem::onFileInfoAvailable()
{
   return pollNotify();
}

int SMTPFileSystem::pollRequest(struct fuse_pollhandle *ph)
{
   m_mutex_mediaone_ioctl.lock();
   if (m_ph_mediaone_ioctl)
   {
      fuse_pollhandle_destroy(m_ph_mediaone_ioctl);
      m_ph_mediaone_ioctl = NULL;
   }
   m_ph_mediaone_ioctl = ph;
   m_mutex_mediaone_ioctl.unlock();
   return 0;
}

int SMTPFileSystem::pollNotify()
{
   m_mutex_mediaone_ioctl.lock();
   if (m_ph_mediaone_ioctl)
   {
      fuse_notify_poll(m_ph_mediaone_ioctl);
      fuse_pollhandle_destroy(m_ph_mediaone_ioctl);
      m_ph_mediaone_ioctl = NULL;
   }
   m_mutex_mediaone_ioctl.unlock();
   return 0;
}

int SMTPFileSystem::pollClear()
{
   m_mutex_mediaone_ioctl.lock();
   if (m_ph_mediaone_ioctl)
   {
      fuse_pollhandle_destroy(m_ph_mediaone_ioctl);
      m_ph_mediaone_ioctl = NULL;
   }
   m_mutex_mediaone_ioctl.unlock();
   return 0;
}

int SMTPFileSystem::initTmpDir()
{
   if (m_tmp_files_pool.isInitialized())
   {
      LogInfo("the tmp files pool was initialized already");
      return 0;
   }
   m_tmp_files_pool.setTmpDir(Config::instance().getTmpPath());
   m_tmp_files_pool.setTmpSizeLimit(Config::instance().getTmpSizeLimit());
   if (!m_tmp_files_pool.createTmpDir())
   {
      LogError("createTmpDir for tmp files pool failed");
      return -1;
   }
   return 0;
}

int SMTPFileSystem::ioctlCheckVersion(char *data, uint32_t len)
{
   if (NULL == data || (len < sizeof(smtpfs_version)))
   {
      LogError("data: %p, len: %d", data, len);
      return -1;
   }
   smtpfs_version *version = (smtpfs_version *)data;
   if (version->versionInternal == SMTPFS_INTERNAL_VERSION)
   {
      LogInfo("the version is correct: %" PRIu64, version->versionInternal);
   }
   else
   {
      LogError("incorrect version: %" PRIu64 "; simple-mtpfs internal version: %" PRIu64, version->versionInternal, SMTPFS_INTERNAL_VERSION);
      return -1;
   }

   return 0;
}

int SMTPFileSystem::ioctlGetTrackList(char *data, uint32_t len)
{
   if (!data || (len < (int)sizeof(smtpfs_event)))
   {
      LogError("data: %p, len: %d", data, len);
      return -1;
   }
   //LogInfo(" track list");
   if (len < (sizeof(smtpfs_event) + sizeof(smtpfs_trackList) + sizeof(smtpfs_trackInfo)))
   {
      LogError("len[%u] is too small", len);
      return -1;
   }
   
   int32_t trackCount = (len - sizeof(smtpfs_event) - sizeof(smtpfs_trackList)) / sizeof(smtpfs_trackInfo);
   std::vector<smtpfs_trackInfo> trackList;
   int32_t ret = 0;

   ret = m_device.getTrackList(trackList, trackCount);

   //event header
   smtpfs_event *pEvent = (smtpfs_event *)data;

   //LogInfo("-----trackList, trackNum: %d------------", trackNum);
   //fill event header
   pEvent->eventType = SMTPFS_EVENT_TRACKLIST;
   pEvent->len = sizeof(smtpfs_event);

   //fill smtpfs_trackList
   smtpfs_trackList *trackListHeader = (smtpfs_trackList *)(data + pEvent->len);

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
      smtpfs_trackInfo *track = (smtpfs_trackInfo *)(data + pEvent->len);

      for (auto it = trackList.begin(); it != trackList.end(); ++ it)
      {
         //fill track info
         *track = *it;

         ++ track;
         ++ trackListHeader->count;
         pEvent->len += sizeof(smtpfs_trackInfo);
      }
      LogDebug("track info count: %d; len: %u", trackListHeader->count, pEvent->len);
   }

   return pEvent->len;
}

int SMTPFileSystem::ioctlSetConfig(char *data, uint32_t len)
{
   if (NULL == data || (len < sizeof(smtpfs_config)))
   {
      LogError("data: %p, len: %d", data, len);
      return -1;
   }
   smtpfs_config *config = (smtpfs_config *)data;

   if (len < config->len)
   {
      LogError("data too short; len: %u; expect len: %u", len, config->len);
      return -1;
   }

   return Config::instance().setConfig(config->type, std::string(data + sizeof(smtpfs_config), config->len - sizeof(smtpfs_config)));
}