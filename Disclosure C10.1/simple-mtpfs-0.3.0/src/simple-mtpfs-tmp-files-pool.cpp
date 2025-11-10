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
#include <algorithm>
#include <sstream>

#include <sys/vfs.h>

#include "simple-mtpfs-tmp-files-pool.h"
#include "simple-mtpfs-sha1.h"
#include "simple-mtpfs-util.h"
#include "simple-mtpfs-log.h"

#define TMP_SIZE_LIMIT_PERCENT_DEF (30)

TmpFilesPool::TmpFilesPool() : m_tmp_dir(smtpfs_get_tmpdir()),
                               m_tmp_size_limit(TMP_SIZE_LIMIT_PERCENT_DEF),
                               m_pool(),
                               m_initialized(false)
{
}

TmpFilesPool::~TmpFilesPool()
{
}

void TmpFilesPool::setTmpDir(const std::string &tmp_dir)
{
   if (!tmp_dir.empty())
   {
      m_tmp_dir = tmp_dir;
   }
}

void TmpFilesPool::setTmpSizeLimit(const int32_t percent)
{
   if (percent > 0)
   {
      m_tmp_size_limit = percent;
   }
}

void TmpFilesPool::removeFile(const std::string &path)
{
   auto it = std::find(m_pool.begin(), m_pool.end(), path);
   if (it == m_pool.end())
      return;
   m_pool.erase(it);
}

void TmpFilesPool::removeOthersExcept(const std::string &std_path)
{
   std::set<TypeTmpFile>::iterator it = m_pool.begin();
   while(it != m_pool.end())
   {
      if(it->pathDevice() != std_path)
      {
         const std::string tmp_path = it->pathTmp();
         LogInfo("%s will be erased!+++", tmp_path.c_str());
         it = m_pool.erase(it);
         ::unlink(tmp_path.c_str());
         LogInfo("%s erased over!---", tmp_path.c_str());
      }
      else
      {
         it++;
      }
   }
   LogInfo("Exit removeOthersExcept function!");
}

bool TmpFilesPool::hasFreeSpace(uint64_t size)
{
   struct statfs s;
   uint64_t freeSize = 0;
   uint64_t totalSize = 0;

   memset(&s, 0, sizeof(s));
   statfs(m_tmp_dir.c_str(), &s);

   freeSize = (uint64_t)s.f_bsize * (uint64_t)s.f_bavail;
   totalSize = (uint64_t)s.f_bsize * (uint64_t)s.f_blocks;
   if (size > freeSize)
   {
      LogError("the dir has no free space; dir: %s; expect size: %" PRIu64 "; free size: %" PRIu64, m_tmp_dir.c_str(), size, freeSize);
      return false;
   }
   if (size > totalSize * m_tmp_size_limit / 100)
   {
      LogError("the size is more than the size limitation; expect size: %" PRIu64 "; limit size: %" PRIu64, size, totalSize * m_tmp_size_limit / 100);
      return false;
   }
   LogInfo("total size: %" PRIu64 "; free size: %" PRIu64 "; expect size: %" PRIu64, totalSize, freeSize, size);

   return true;
}

const TypeTmpFile *TmpFilesPool::getFile(const std::string &path) const
{
   auto it = std::find(m_pool.begin(), m_pool.end(), path);
   if (it == m_pool.end())
      return nullptr;
   return static_cast<const TypeTmpFile *>(&*it);
}

std::string TmpFilesPool::makeTmpPath(const std::string &path_device) const
{
   static int cnt = 0;
   std::stringstream ss;
   ss << path_device << ++cnt;
   return m_tmp_dir + std::string("/") + SHA1::sumString(ss.str());
}

bool TmpFilesPool::createTmpDir()
{
   bool ret = false;

   removeTmpDir();
   ret = smtpfs_create_dir(m_tmp_dir);
   if (ret)
   {
      LogInfo("create tmp dir OK; %s", m_tmp_dir.c_str());
   }
   else
   {
      LogError("create tmp dir failed; %s", m_tmp_dir.c_str());
   }
   m_initialized = true;

   return ret;
}

bool TmpFilesPool::removeTmpDir()
{
   if (m_tmp_dir.empty())
   {
      m_tmp_dir = smtpfs_get_tmpdir();
   }
   
   return smtpfs_remove_dir(m_tmp_dir);
}
