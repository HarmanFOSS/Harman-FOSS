#########################################################################
#												#
#				FreePV's Script						#
#												#
#***********************************************************************#
#  This FreePV script tries to find jpeg library and header files		#
#  JPEG_INCLUDE_DIR, headers path.							#
#  JPEG_LIBRARIES, libraries needed.						#
#  JPEG_FOUND, either true or false.						#
#########################################################################

IF(WIN32)
	FIND_PATH(JPEG_INCLUDE_DIR jpeglib.h
		${CMAKE_INCLUDE_PATH} 
		$ENV{include}
	)
	SET(JPEG_NAMES jpeg-6b)
	FIND_LIBRARY(JPEG_LIBRARY
  		NAMES ${JPEG_NAMES} 
		PATHS ${CMAKE_LIBRARY_PATH} $ENV{lib}
	) 
ELSE(WIN32)

	FIND_PATH(JPEG_INCLUDE_DIR jpeglib.h
		/usr/local/include
		/usr/include
	)
	SET(JPEG_NAMES ${JPEG_NAMES} jpeg)
	FIND_LIBRARY(JPEG_LIBRARY
  		NAMES ${JPEG_NAMES}
  		PATHS /usr/lib /usr/local/lib
  	)

ENDIF(WIN32)

IF (JPEG_INCLUDE_DIR AND JPEG_LIBRARY)
   SET(JPEG_FOUND TRUE)
   SET(JPEG_LIBRARIES ${JPEG_LIBRARY})
ENDIF (JPEG_INCLUDE_DIR AND JPEG_LIBRARY)

IF(NOT JPEG_FOUND)
   IF (JPEG_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find JPEG")
   ENDIF (JPEG_FIND_REQUIRED)
ENDIF(NOT JPEG_FOUND)
