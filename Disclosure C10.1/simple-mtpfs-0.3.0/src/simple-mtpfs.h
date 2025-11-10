/******************************************************************************
 * Project         Harman Car Multimedia System
 * (c) copyright   2017-2020
 * Company         Harman Automotive Systems
 *                 All rights reserved
 * Secrecy Level   STRICTLY CONFIDENTIAL
 ******************************************************************************/
/**
 * @file           simple-mtpfs.h
 * @author         Xiaojun.Zou@harman.com
 * @brief          the interface for simple-mtpfs
 * 
 * @ATTENTION      this file is used to define the interface for simple-mtpfs. it could only be 
 *                 modified from simple-mtpfs.
 */

#ifndef SIMPLE_MTPFS_H
#define SIMPLE_MTPFS_H

#include <stdint.h>

#include <sys/ioctl.h>

// the internal version of simple-mtpfs, it need to be updated if the interface file "simple-mtpfs.h" be modified
#define SMTPFS_INTERNAL_VERSION     ((uint64_t)1)
#define SMTPFS_EVENT_MAXLEN (8 * 1024) //128k

// SMTPFS_GETVERSION
// the versioin structure
typedef struct
{
   uint64_t versionInternal;
} smtpfs_version;

enum
{
   COMMAND_RESPONSE_STATUS_SUCESS = 0,
   COMMAND_RESPONSE_STATUS_FAILED = -1
};

typedef enum
{
   SMTPFS_EVENT_SYNC_LASTMODE = 0x01,
   SMTPFS_EVENT_TRACKLIST = 0x02,
   SMTPFS_EVENT_CACHE_NEW_TRACK = 0x03,
   SMTPFS_EVENT_CACHE_TRACK_RSP = 0x04
} eMtpEvent;

typedef struct
{ 
   int eventType;        //enum eMtpEvent
   int len;             //include the event struct itself
} smtpfs_event;

#define MTP_NAME_MAXLEN (128)
#define MTP_PATHNAME_MAXLEN (512)
#define MTP_METADATA_NAME_MAXLEN (128)

typedef enum
{
   MTP_FILE_TYPE_UNKNOWN = 0x0,
   MTP_FILE_TYPE_AUDIO = 0x01,
   MTP_FILE_TYPE_VIDEO = 0x02,
   MTP_FILE_TYPE_IMAGE = 0x04,
   MTP_FILE_TYPE_PLAYLIST = 0x08,
   MTP_FILE_TYPE_AUDIOBOOKS = 0x10
} eMtpFileType;

typedef struct
{
   int type;          //enum eMtpFileType
   char path[MTP_PATHNAME_MAXLEN]; //folder path
   //char  fileName[NAME_MAX];//filename
   char title[MTP_METADATA_NAME_MAXLEN];  /**< Track title */
   char artist[MTP_METADATA_NAME_MAXLEN]; /**< Name of recording artist */
   //char  composer[NAME_MAX]; /**< Name of recording composer */
   char genre[MTP_METADATA_NAME_MAXLEN]; /**< Genre name for track */
   char album[MTP_METADATA_NAME_MAXLEN]; /**< Album name for track */
   uint32_t duration;                    /**< Duration in milliseconds */
   uint32_t samplerate;                  /**< Sample rate of original file, min 0x1f80 max 0xbb80 */
   uint32_t nochannels;                  /**< Number of channels in this recording 0 = unknown, 1 or 2 */
   uint32_t wavecodec;                   /**< FourCC wave codec name */
   uint32_t bitrate;                     /**< (Average) bitrate for this file min=1 max=0x16e360 */
   uint64_t filesize;                    /**< Size of track file in bytes */
   time_t modificationdate;              /**< Date of last alteration of the track */
   uint16_t tracknumber;                 /**< Track number (in sequence on recording) */
   uint16_t rating;                      /**< User rating 0-100 (0x00-0x64) */
} smtpfs_trackInfo;

//for track list: smtpfs_event + smtpfs_trackList + N*smtpfs_trackInfo
typedef struct
{
   int fragment;
   int count;
   // fix gcc warning
   // smtpfs_trackInfo trackList[0];
} smtpfs_trackList;

//for sync lastmode: smtpfs_event + smtpfs_sync_lastmode
typedef struct
{
   char path[MTP_PATHNAME_MAXLEN];
} smtpfs_sync_lastmode;

//  for SMTPFS_SETCONFIG.
//  use this IO to set config from MediaOne.
//  TODO. please consider the config content is passed twice or more; because the
//  length of the config content is more that the limitation: SMTPFS_EVENT_MAXLEN.
//	for smtpfs config, there are only two field for every one: key, and value.
#define SMTPFS_CONFIG_UNKNOWN (~0)
#define SMTPFS_CONFIG_THREAD_POLICY (0x1)
#define SMTPFS_CONFIG_THREAD_PRIORITY (0x2)
#define SMTPFS_CONFIG_EXCLUDE_PATTERN (0x3)
// for these following support extension name config, it should push the extension name to supported list for per ioctl calling.
#define SMTPFS_CONFIG_EXTAUDIO (0x4)
#define SMTPFS_CONFIG_EXTVIDEO (0x5)
#define SMTPFS_CONFIG_EXTIMAGE (0x6)
#define SMTPFS_CONFIG_EXTPLAYLIST (0x7)
#define SMTPFS_CONFIG_EXTAUDIOBOOK (0x8)
// for log config
#define SMTPFS_CONFIG_LOGLEVEL (0x9)
#define SMTPFS_CONFIG_LIBMTP_LOGLEVEL (0xa)
// for file cache tmp folder
#define SMTPFS_CONFIG_CACHEFIRSTFILE (0xb)
#define SMTPFS_CONFIG_TMPPATH (0xc)
#define SMTPFS_CONFIG_TMPSIZELIMIT (0xd)

typedef struct
{
   //	the config type
   uint32_t type;
   //  including the smtpfs_config itself
   uint32_t len;
   //  the value string are followed; no zero-terminate.
   //  char value[0];
} smtpfs_config;

#define SMTPFSIOCTLNODE "mediaone_smtpfs_node"
#define SMTPFSMAGIC 'm'

#define DCMD_SMTP_IOCTL_CHECK_VERSION  1
#define DCMD_SMTP_IOCTL_SET_EVENT      2
#define DCMD_SMTP_IOCTL_GET_EVENT      3
#define DCMD_SMTP_IOCTL_SET_CONFIG     4
#define DCMD_SMTP_IOCTL_ABORT_SCAN     5
#define DCMD_SMTP_IOCTL_GET_LABLE      6

//define the ioctl command for mtp operations
#define SMTPFS_CHECKVERSION      _IOW(SMTPFSMAGIC, DCMD_SMTP_IOCTL_CHECK_VERSION, char[SMTPFS_EVENT_MAXLEN])
#define SMTPFS_SETEVENT          _IOW(SMTPFSMAGIC, DCMD_SMTP_IOCTL_SET_EVENT, char[SMTPFS_EVENT_MAXLEN])
#define SMTPFS_GETEVENT          _IOR(SMTPFSMAGIC, DCMD_SMTP_IOCTL_GET_EVENT, char[SMTPFS_EVENT_MAXLEN])
#define SMTPFS_SETCONFIG         _IOW(SMTPFSMAGIC, DCMD_SMTP_IOCTL_SET_CONFIG, char[SMTPFS_EVENT_MAXLEN])
#define SMTPFS_ABORTSCAN         _IOW(SMTPFSMAGIC, DCMD_SMTP_IOCTL_ABORT_SCAN, char[SMTPFS_EVENT_MAXLEN])
#define SMTPFS_GETFRIENDLYNAME   _IOR(SMTPFSMAGIC, DCMD_SMTP_IOCTL_GET_LABLE, char[MTP_NAME_MAXLEN])

#endif
