#########################################################################
#												#
#				FreePV's Script						#
#												#
#***********************************************************************#
#  This FreePV script tries to find GECKO libraries and header files    #
#  GECKO_INCLUDE_DIR, headers path.							#
#  GECKO_LIBRARIES, libraries needed.						#
#  GECKO_FOUND, either true or false.						#
#########################################################################

IF(UNIX)
    INCLUDE(UsePkgConfig)
    PKGCONFIG(firefox-plugin _FIREFOX-PLUGINIncDir _FIREFOX-PLUGINLinkDir
       _FIREFOX-PLUGINLinkFlags _FIREFOX-PLUGINCflags)
    PKGCONFIG(mozilla-plugin _MOZILLA-PLUGINIncDir _MOZILLA-PLUGINLinkDir
       _MOZILLA-PLUGINLinkFlags _MOZILLA-PLUGINCflags)
    PKGCONFIG(xulrunner-plugin _XULRUNNER-PLUGINIncDir _XULRUNNER-PLUGINLinkDir
       _XULRUNNER-PLUGINLinkFlags _XULRUNNER-PLUGINCflags)
    PKGCONFIG(libxul _LIBXULIncDir _LIBXULLinkDir
       _LIBXULLinkFlags _LIBXULCflags)
    PKGCONFIG(nspr _NSPRIncDir _NSPRLinkDir _NSPRLinkFlags _NSPRCflags)

	FIND_PATH(GECKO_ROOT_DIR npapi.h npupp.h /usr/include/firefox /usr/local/include/firefox
           /usr/include/xulrunner /usr/include/seamonkey /usr/include/iceape ${_XULRUNNER-PLUGINIncDir}
           ${_FIREFOX-PLUGINIncDir} ${_MOZILLA-PLUGINIncDir} ${_LIBXULIncDir}/stable)
	FIND_PATH(GECKO_NSPR_DIR prthread.h ${GECKO_ROOT_DIR}/nspr
           ${_NSPRIncDir} /usr/include/nspr4 ${GECKO_ROOT_DIR})
	FIND_LIBRARY( Xt_LIBRARY NAMES Xt /usr/lib /usr/local/lib )
      SET(GECKO_INCLUDE_DIR ${GECKO_ROOT_DIR} ${GECKO_NSPR_DIR})
      SET(GECKO_LIBRARY ${Xt_LIBRARY})

IF (GECKO_INCLUDE_DIR AND GECKO_LIBRARY)
   SET(GECKO_FOUND TRUE)
   SET(GECKO_LIBRARIES ${GECKO_LIBRARY})
ENDIF (GECKO_INCLUDE_DIR AND GECKO_LIBRARY)

IF(NOT GECKO_FOUND)
   IF (GECKO_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find GECKO SDK")
   ENDIF (GECKO_FIND_REQUIRED)
ENDIF(NOT GECKO_FOUND)

ENDIF(UNIX)
