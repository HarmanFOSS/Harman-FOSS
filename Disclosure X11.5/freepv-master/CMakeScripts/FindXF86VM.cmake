#########################################################################
#												#
#				FreePV's Script						#
#												#
#***********************************************************************#
#  This FreePV script tries to find freeglut library and header files   #
#  FREEGLUT_INCLUDE_DIR, headers path.						#
#  FREEGLUT_LIBRARIES, libraries needed.						#
#  FREEGLUT_FOUND, either true or false.						#
#########################################################################

IF(WIN32) 
ELSE(WIN32)
	FIND_PATH(XF86VM_INCLUDE_DIR X11/extensions/xf86vmode.h /usr/include /usr/local/include)
	FIND_LIBRARY(XF86VM_LIBRARY NAMES Xxf86vm
      		/usr/lib
      		/usr/local/lib
    	)
ENDIF(WIN32)

IF (XF86VM_INCLUDE_DIR AND XF86VM_LIBRARY)
   SET(XF86VM_FOUND TRUE)
   SET(XF86VM_LIBRARIES ${XF86VM_LIBRARY})
ENDIF (XF86VM_INCLUDE_DIR AND XF86VM_LIBRARY)

IF(NOT XF86VM_FOUND)
   IF (XF86VM_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Xxf86vm")
   ENDIF (XF86VM_FIND_REQUIRED)
ENDIF(NOT XF86VM_FOUND)
