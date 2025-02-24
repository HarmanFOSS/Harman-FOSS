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

#include "simple-mtpfs-config.h"
#include "simple-mtpfs-fuse.h"
#include "simple-mtpfs-util.h"
#include "simple-mtpfs-log.h"

int main(int argc, char **argv)
{
    LIBMTP_DLT_scope("SMFS","Simple MTP information");
    Config::instance();
    Logger::init();

    SMTPFileSystem *filesystem = SMTPFileSystem::instance();
    //  log the build time
    LogInfo("the building version of simple-mtpfs:%s",getVersion().c_str());
    if (!filesystem->parseOptions(argc, argv)) {
        LogError("Wrong usage! See  %s -h for details",smtpfs_basename(argv[0]));
        return 1;
    }

    if (filesystem->isHelp()) {
        filesystem->printHelp();
        return 0;
    }

    if (filesystem->isVersion()) {
        filesystem->printVersion();
        return 0;
    }

    if (filesystem->isListDevices())
        return !filesystem->listDevices();

    return !filesystem->exec();
}
