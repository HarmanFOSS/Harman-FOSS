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

#ifndef SMTPFS_TMP_FILES_POOL_H
#define SMTPFS_TMP_FILES_POOL_H

#include <set>
#include <string>
#include <utility>
#include "simple-mtpfs-type-tmp-file.h"

class TmpFilesPool
{
 public:
   TmpFilesPool();
   ~TmpFilesPool();

   bool isInitialized(){ return m_initialized; }
   void setTmpDir(const std::string &tmp_dir);
   void setTmpSizeLimit(const int32_t percent);

   void addFile(const TypeTmpFile &tmp) { m_pool.insert(tmp); }
   void removeFile(const std::string &path);
   void removeOthersExcept(const std::string &std_path);
   bool empty() const { return m_pool.size(); }
   bool hasFreeSpace(uint64_t size);

   const TypeTmpFile *getFile(const std::string &path) const;

   std::string makeTmpPath(const std::string &path_device) const;
   bool createTmpDir();
   bool removeTmpDir();

 private:
   std::string m_tmp_dir;
   int32_t m_tmp_size_limit;
   std::set<TypeTmpFile> m_pool;
   bool m_initialized;
};

#endif // SMTPFS_TMP_FILES_POOL_H
