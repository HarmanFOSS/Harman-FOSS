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
#include <cstdio>
#include <cstring>
#ifdef HAVE_LIBUSB1
#  include <iomanip>
#  include <sstream>
#endif // HAVE_LIBUSB1
extern "C" {
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
}
// #include <sstream>
#ifdef HAVE_LIBUSB1
#  include <climits>
extern "C" {
#  include <libmtp.h>
#  include <libusb.h>
}
#endif // HAVE_LIBUSB1
#include "simple-mtpfs-log.h"
#include "simple-mtpfs-util.h"

#define BUILD_TIME() (__DATE__ " " __TIME__)

const std::string devnull = "/dev/null";

bool StreamHelper::s_enabled = false;
int StreamHelper::s_stdout = -1;
int StreamHelper::s_stderr = -1;

std::string getVersion()
{
   static char ver[8];
   struct tm t;
   const std::string buildTime = BUILD_TIME();
   std::stringstream ss;

   memset(ver, 0, sizeof(ver));
   memset(&t, 0, sizeof(t));

   strptime(buildTime.c_str(), "%b %d %Y %T", &t);
   strftime(ver, sizeof(ver), "%g%V%u", &t);

   ss << "[" << ver << "]" << "[" << buildTime << "]";

   return ss.str();
}

void StreamHelper::on()
{
    if (Logger::isEnableLibmtpLog())
        return;

    if (!s_enabled)
        return;

    freopen(devnull.c_str(), "w", stdout);
    freopen(devnull.c_str(), "w", stderr);
    dup2(s_stdout, fileno(stdout));
    dup2(s_stderr, fileno(stderr));
    close(s_stdout);
    close(s_stderr);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    s_enabled = false;
}

void StreamHelper::off()
{
    if (Logger::isEnableLibmtpLog())
        return;

    if (s_enabled)
        return;

    fflush(stdout);
    fflush(stderr);
    s_stdout = dup(fileno(stdout));
    s_stderr = dup(fileno(stderr));
    freopen(devnull.c_str(), "w", stdout);
    freopen(devnull.c_str(), "w", stderr);

    s_enabled = true;
}

std::string smtpfs_dirname(const std::string &path)
{
    char *str = strdup(path.c_str());
    std::string result(dirname(str));
    free(static_cast<void*>(str));
    return result;
}

std::string smtpfs_basename(const std::string &path)
{
    char *str = strdup(path.c_str());
    std::string result(basename(str));
    free(static_cast<void*>(str));
    return result;
}

std::string smtpfs_realpath(const std::string &path)
{
    char buf[PATH_MAX + 1];
    char *real_path = realpath(path.c_str(), buf);
    return std::string(real_path ? buf : "");
}

std::string smtpfs_get_tmpdir()
{
    const char *c_tmp = getenv("TMP");
    std::string tmp_dir;
    if (c_tmp) {
        tmp_dir = smtpfs_realpath(c_tmp);
    } else {
        c_tmp = getenv("TMPDIR");
        if (!c_tmp)
            c_tmp = TMPDIR;
        tmp_dir = smtpfs_realpath(c_tmp);
    }

   tmp_dir += "/simple-mtpfs";
   //  tmp_dir += "/simple-mtpfs-XXXXXX";
   //  char *c_tmp_dir = ::mkdtemp(::strdup(tmp_dir.c_str()));

   //  tmp_dir.assign(c_tmp_dir);
   //  ::free(static_cast<void*>(c_tmp_dir));

    return tmp_dir;
}

bool smtpfs_create_dir(const std::string &dirname)
{
    return ::mkdir(dirname.c_str(), S_IRWXU) == 0;
}

bool smtpfs_remove_dir(const std::string &dirname)
{
    DIR *dir;
    struct dirent *entry;
    std::string path;

    LogDebug("try to remove the dir: %s", dirname.c_str());
    dir = ::opendir(dirname.c_str());
    if (!dir)
        return false;

    while ((entry = ::readdir(dir))) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            path = dirname + "/" + entry->d_name;
            if (entry->d_type == DT_DIR)
                return smtpfs_remove_dir(path);
            ::unlink(path.c_str());
            LogDebug("unlink file: %s", path.c_str());
        }
    }
    ::closedir(dir);
    ::remove(dirname.c_str());
    LogDebug("remove dir: %s", dirname.c_str());
    return true;
}

bool smtpfs_usb_devpath(const std::string &path, uint8_t *bnum, uint8_t *dnum)
{
    unsigned int bus, dev;
#ifdef USB_DEVPATH
    std::string realpath(smtpfs_realpath(path));
    if (realpath.empty() ||
        sscanf(realpath.c_str(), USB_DEVPATH, &bus, &dev) != 2)
#endif
        if (sscanf(path.c_str(), "%u/%u", &bus, &dev) != 2)
            return false;

    if (bus > 255 || dev > 255)
        return false;

    *bnum = bus;
    *dnum = dev;
    return true;
}

#ifdef HAVE_LIBUSB1
LIBMTP_raw_device_t *smtpfs_raw_device_new_priv(libusb_device *usb_device)
{
    if (!usb_device)
        return nullptr;

    LIBMTP_raw_device_t *device = static_cast<LIBMTP_raw_device_t*>(
        malloc(sizeof(LIBMTP_raw_device_t)));

    if (!device)
        return nullptr;

    struct libusb_device_descriptor desc;
    int err = libusb_get_device_descriptor(usb_device, &desc);
    if (err != LIBUSB_SUCCESS) {
        free(static_cast<void*>(device));
        return nullptr;
    }

    device->device_entry.vendor = nullptr;  // TODO: vendor string
    device->device_entry.vendor_id = desc.idVendor;
    device->device_entry.product = nullptr; // TODO: product string
    device->device_entry.product_id = desc.idProduct;
    device->device_entry.device_flags = 0;

    device->bus_location = static_cast<uint32_t>(libusb_get_bus_number(usb_device));
    device->devnum = libusb_get_device_address(usb_device);

    return device;
}

LIBMTP_raw_device_t *smtpfs_raw_device_new(const std::string &path)
{
    uint8_t bnum, dnum;
    if (!smtpfs_usb_devpath(path, &bnum, &dnum))
        return nullptr;

    if (libusb_init(NULL) != 0)
        return nullptr;

    libusb_device **dev_list;
    ssize_t num_devs = libusb_get_device_list(NULL, &dev_list);
    if (!num_devs) {
        libusb_exit(NULL);
        return nullptr;
    }

    libusb_device *dev = nullptr;
    for (auto i = 0; i < num_devs; ++i) {
        dev = dev_list[i];
        if (bnum == libusb_get_bus_number(dev_list[i]) &&
            dnum == libusb_get_device_address(dev_list[i]))
            break;
        dev = nullptr;
    }

    LIBMTP_raw_device_t *raw_device = smtpfs_raw_device_new_priv(dev);

    libusb_free_device_list(dev_list, 0);
    libusb_exit(NULL);

    return raw_device;
}

bool smtpfs_reset_device(LIBMTP_raw_device_t *device)
{
    if (libusb_init(NULL) != 0)
        return false;

    libusb_device **dev_list;
    ssize_t num_devs = libusb_get_device_list(NULL, &dev_list);
    if (!num_devs) {
        libusb_exit(NULL);
        return false;
    }

    libusb_device_handle *dev_handle = nullptr;
    for (auto i = 0; i < num_devs; ++i) {
        uint8_t bnum = libusb_get_bus_number(dev_list[i]);
        uint8_t dnum = libusb_get_device_address(dev_list[i]);

        if (static_cast<uint32_t>(bnum) == device->bus_location &&
            dnum == device->devnum)
        {
            libusb_open(dev_list[i], &dev_handle);
            libusb_reset_device(dev_handle);
            libusb_close(dev_handle);
            break;
        }
    }

    libusb_free_device_list(dev_list, 0);
    libusb_exit(NULL);

    return true;
}

void smtpfs_raw_device_free(LIBMTP_raw_device_t *device)
{
    if (!device)
        return;

    free(static_cast<void*>(device->device_entry.vendor));
    free(static_cast<void*>(device->device_entry.product));
    free(static_cast<void*>(device));
}
#endif // HAVE_LIBUSB1

bool smtpfs_check_dir(const std::string &path)
{
    struct stat buf;
    if (::stat(path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode)
        && ::access(path.c_str(), R_OK | W_OK | X_OK) == 0)
    {
        return true;
    }

    return false;
}


bool endWith(const string& str, const string& substr) 
{
   //check the length
   if(str.length() <= substr.length())
   {
      return false;
   }

   return str.rfind(substr) == (str.length() - substr.length());
}


void mytolower(string& s)
{
    int len=s.size();
    for(int i=0;i<len;i++){
        if(s[i]>='A'&&s[i]<='Z'){
            s[i]+=32;//+32 to lowwer
        }
    }
}

void mytoupper(string& s)
{
    int len=s.size();
    for(int i=0;i<len;i++){
        if(s[i]>='a'&&s[i]<='z'){
            s[i]-=32;//-32 to upper
        }
    }
}


// bool isAudioType(const char *fileName)
// {
//    const char* extArray[] = 
//    {
// 		".mp3",
// 		".wma",
// 		".aac",
// 		".wav",
// 		".wave",
// 		".flac",
// 		".oga",
// 		".ogg",
// 		".mka",
//     };

//    if(fileName == NULL)
//    {
//       LogError("file name is NULL");
//       return false;
//    }

//    string str = fileName;
//    mytolower(str);

//    int i = 0;
//    int total = sizeof(extArray)/sizeof(char *);
//    string extStr = "";
//    for(i = 0; i < total; i++)
//    {
//       extStr = extArray[i];
//       if(endWith(str, extStr))
//       {
//          return true;
//       }
//    }

//    return false;
// }

// bool isVideoType(const char *fileName)
// {
//    const char* extArray[] = 
//    {
// 		".avi",
// 		".asf",
// 		".wmv",
// 		".mkv",
// 		".mebm",
// 		".mpg",
// 		".dat",
// 		".mpeg",
// 		".evo",
// 		".vob",
// 		".vdr",
// 		".mod",
// 		".ts",
// 		".trp",
// 		".mts",
// 		".m2t",
// 		".m2ts",
// 		".tod",
// 		".mp4",
// 		".m4v",
// 		".3gp",
// 		".mov",
// 		".qt",
//     };

//    if(fileName == NULL)
//    {
//       LogError("file name is NULL");
//       return false;
//    }

//    string str = fileName;
//    mytolower(str);

//    int i = 0;
//    int total = sizeof(extArray)/sizeof(char *);
//    string extStr = "";
//    for(i = 0; i < total; i++)
//    {
//       extStr = extArray[i];
//       if(endWith(str, extStr))
//       {
//          return true;
//       }
//    }

//    return false;
// }

// bool isImageType(const char *fileName)
// {
//    const char* extArray[] = 
//    {
// 		".bmp",
// 		".jpg",
// 		".jpeg",
// 		".gif",
// 		".tif",
// 		".png",
//     };

//    if(fileName == NULL)
//    {
//       LogError("file name is NULL");
//       return false;
//    }

//    string str = fileName;
//    mytolower(str);

//    int i = 0;
//    int total = sizeof(extArray)/sizeof(char *);
//    string extStr = "";
//    for(i = 0; i < total; i++)
//    {
//       extStr = extArray[i];
//       if(endWith(str, extStr))
//       {
//          return true;
//       }
//    }

//    return false;
// }

// bool isPlayListType(const char *fileName)
// {
//    const char* extArray[] = 
//    {
//       ".m3u",
//       ".m3u8",
//       ".pls",
//       ".asx",
//       ".wpl",
//       ".xspf",
//    };

//    if(fileName == NULL)
//    {
//       LogError("file name is NULL");
//       return false;
//    }

//    string str = fileName;
//    mytolower(str);

//    int i = 0;
//    int total = sizeof(extArray)/sizeof(char *);
//    string extStr = "";
//    for(i = 0; i < total; i++)
//    {
//       extStr = extArray[i];
//       if(endWith(str, extStr))
//       {
//          return true;
//       }
//    }

//    return false;
// }

// bool isAudioBooksType(const char *fileName)
// {
//    const char* extArray[] = 
//    {
//       ".m4a",
//       ".m4b",
//    };

//    if(fileName == NULL)
//    {
//       LogError("file name is NULL");
//       return false;
//    }

//    string str = fileName;
//    mytolower(str);

//    int i = 0;
//    int total = sizeof(extArray)/sizeof(char *);
//    string extStr = "";
//    for(i = 0; i < total; i++)
//    {
//       extStr = extArray[i];
//       if(endWith(str, extStr))
//       {
//          return true;
//       }
//    }

//    return false;
// }

// bool isMediaType(const char *fileName)
// {
//    if(   isAudioType(fileName) 
//       || isVideoType(fileName) 
// 	  || isImageType(fileName) 
// 	  || isPlayListType(fileName)
// 	  || isAudioBooksType(fileName)
// 	)
// 	{
// 		return true;
// 	}
// 	else
// 	{
// 		return false;
// 	}
// }

// bool isMatchExtName(const std::string &name, const std::regex& re)
// {
//    if (name.empty())
//    {
//       LogError("file name is empty");
//       return false;
//    }
//    std::string nameLower = name;

//    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
//    auto it = std::find_if(extNameLowerList.cbegin(), extNameLowerList.cend(), [nameLower](const std::string &extNameLower) {
//       if (endWith(nameLower, extNameLower))
//       {
//          return true;
//       }
//       else
//       {
//          return false;
//       }
//    });

//    return (it != extNameLowerList.end());
// }

bool makePatternStrArray(std::string &pattern, const std::vector<std::string> &arr)
{
   if (arr.empty())
   {
      LogError("the string vector is empty");
      return false;
   }
   try
   {
      std::regex reExtName("(\\.).+", std::regex_constants::icase);
      std::stringstream ss;

      for (auto it = arr.begin(); it != arr.end(); ++ it)
      {
         if (!std::regex_match(*it, reExtName))
         {
            LogInfo("not a extension string: %s", it->c_str());
         }
         else
         {
            if (it != arr.begin())
            {
               ss << "|";
            }
            ss << ".+(\\.)" << it->substr(1, it->length());
         }
      }
      pattern = ss.str();
      LogInfo("make the regex pattern: %s", ss.str().c_str());

      return true;
   }
   catch (std::regex_error e)
   {
      LogError("regex exception: %d; %s", e.code(), e.what());
   }
   catch (...)
   {
      LogError("catch unknown exception while match excluding string");
   }

   return false;
}

bool makeRegEx(std::regex &re, const std::string &pattern)
{
   if (pattern.empty())
   {
      LogWarning("the pattern is empty");
   }
   try
   {
      LogInfo("make regular expression for pattern: %s", pattern.c_str());
      re = std::regex(pattern, std::regex_constants::icase);
      return true;
   }
   catch (std::regex_error e)
   {
      LogError("regex exception: %d; %s", e.code(), e.what());
   }
   catch (...)
   {
      LogError("catch unknown exception while match excluding string");
   }

   return false;
}

bool makeRegExStrArray(std::regex &re, const std::vector<std::string> &strVec)
{
   std::string pattern;

   if (makePatternStrArray(pattern, strVec))
   {
      if (makeRegEx(re, pattern))
      {
         return true;
      }
      else
      {
         LogError("make regEx for audio extension name failed");
      }
   }
   else
   {
      LogError("make pattern failed");
   }

   return false;
}

bool reMatch(const std::string& str, const std::regex& re)
{
    //  use c++ regex match
    try
    {
        return std::regex_match(str, re);
    }
    catch (std::regex_error e)
    {
        LogError("regex exception: %d; %s; str: %s;", e.code(), e.what(), str.c_str());
    }
    catch (...)
    {
        LogError("catch unknown exception while match excluding string; str: %s", str.c_str());
    }

    return false;
}

bool reSearch(const std::string &str, const std::regex &re)
{
   //  use c++ regex search
    try
    {
        return std::regex_search(str, re);
    }
    catch (std::regex_error e)
    {
        LogError("regex exception: %d; %s; str: %s;", e.code(), e.what(), str.c_str());
    }
    catch (...)
    {
        LogError("catch unknown exception while search excluding string; str: %s", str.c_str());
    }

    return false;
}

int setThreadPriority(int32_t policy, int32_t priority)
{
   sched_param param;

   LogInfo("set thread sched policy: %d, priority: %d", policy, priority);

   memset(&param, 0, sizeof(param));

   if (policy != SCHED_OTHER)
   {
      param.sched_priority = (int)priority;
      if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
      {
         LogError("pthread_setschedparam failed!");
         return -1;
      }
   }

   return 0;
}