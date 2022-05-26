#########################################################################
#												#
#				FreePV's Script						#
#												#
#***********************************************************************#
#  This FreePV script tries to find jpeg library and header files		#
#  ZLIB_INCLUDE_DIR, headers path.							#
#  ZLIB_LIBRARIES, libraries needed.						#
#  ZLIB_FOUND, either true or false.						#
#########################################################################

IF(WIN32)
	FIND_PATH(ZLIB_INCLUDE_DIR zlib.h 
		${CMAKE_INCLUDE_PATH} 
		$ENV{include}
	)
	SET(ZLIB_NAMES z zlib zlib1 zlib1d zdll)
	FIND_LIBRARY(ZLIB_LIBRARY
  		NAMES ${ZLIB_NAMES} 
		PATHS ${CMAKE_LIBRARY_PATH} $ENV{lib}
	) 
ELSE(WIN32)

	FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
  		/usr/local/include
  		/usr/include
	)
	SET(ZLIB_NAMES z zlib zdll)
	FIND_LIBRARY(ZLIB_LIBRARY
  		NAMES ${ZLIB_NAMES}
  		PATHS /usr/lib /usr/local/lib
	)

ENDIF(WIN32)

IF (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
   SET(ZLIB_FOUND TRUE)
   SET(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
ENDIF (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)

IF(NOT ZLIB_FOUND)
   IF (ZLIB_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find ZLIB")
   ENDIF (ZLIB_FIND_REQUIRED)
ENDIF(NOT ZLIB_FOUND)
