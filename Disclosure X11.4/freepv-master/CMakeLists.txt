#########################################################################
#									#
#			FreePV's CMakeLists				#
#									#
#***********************************************************************#
# Project Name:								#
#									#
#	FreePV								#
#									#
# References:								#
#                                                                       #
#	Root Source Directory: ${FreePV_SOURCE_DIR}			#
#	Root Binary Directory: ${FreePV_BINARY_DIR}			#
#									#
# CMake Vesion: 2.4.0							#
#									#
# TODO:									#
#	*Have the option to include MSVC Projects.			#
#	*Build freepv-mozilla project					#
#									#
#########################################################################

#Set the Project's Name
project (FreePV)
include(CheckFunctionExists)

if(WIN32)
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
MESSAGE(FATAL_ERROR "CMake generation for freepv is not allowed within the source directory! 
Remove the CMakeCache.txt file and try again from another folder, e.g.: 

   rm CMakeCache.txt 
   cd ..
   mkdir cmake-make 
   cd cmake-make
   cmake ../branch_leonox
")
ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
endif(WIN32)

#Check requiered cmake version
cmake_minimum_required(VERSION 2.4.0)

# set version

set(V_MAJOR 0)
set(V_MINOR 3)
set(V_PATCH 0)

# set destination dirs relative to PREFIX
SET(LIBDIR lib${LIB_SUFFIX})

#########################################################################
# MACROS								#
#***********************************************************************#
# 									#
# CMake MACROS Scripts can be located in CMake Modules directory for	#
# example under Linux, it would be /usr/local/share/CMake/Modules.	#
# The Proyect scripts and macros can be found in:			#
#	${PROJECT_SOURCE_DIR}/CMakeScripts				# 
# 									#
#########################################################################

# set cmake script directory
# we let the user decide if he wants to use the scipts inside
# CMakeScripts or use the ones that come by default with CMake.

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeScripts)

#Include MACROS
include(CheckIncludeFile)

#CFLAGS
#Position Independent Code (PIC) is needed by shared libraries
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

#########################################################################
# LIBRARIES								#
#***********************************************************************#
# We look for the needed libraries, for more details see CMakeScripts	#
#									#
# find_package uses FindLibrary.cmake scripts to look for libraries.	#
#									#
#########################################################################

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(FREEGLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(PNG REQUIRED)
find_package(XF86VM REQUIRED)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

# check for gettimeofday
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)

# set compiler flags and include directories
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/src
                    ${CMAKE_SOURCE_DIR}/src
                    ${ZLIB_INCLUDE_DIR}
                    ${JPEG_INCLUDE_DIR}
                    ${OPENGL_INCLUDE_DIR}
                    ${FREEGLUT_INCLUDE_DIR}
		    ${X11_INCLUDE_DIR}
		    ${LIBXML2_INCLUDE_DIR}
		    ${PNG_INCLUDE_DIR}
		    ${XF86VM_INCLUDE_DIR})

#########################################################################
# CONFIGURE								#
#***********************************************************************#
# We need to create suitable variables for the config.h file:		#
#	*Check for time functions headers.				#
#	*Check for GLUT headers	.					#
#	*Check which kind of GLUT we have.				#
# 									#
#########################################################################

if(WIN32)
  set(HAVE_FREEGLUT_H 1)
  set(XP_WIN 1)
else(WIN32)
 if(X11_FOUND)
 	set(MOZ_X11 1)
 endif(X11_FOUND)
 set(XP_UNIX 1)
 CHECK_INCLUDE_FILE(sys/time.h HAVE_GETTIMEOFDAY)
 set(CMAKE_REQUIRED_INCLUDES ${FREEGLUT_INCLUDE_DIR})
 CHECK_INCLUDE_FILE(GL/freeglut.h HAVE_FREEGLUT_H)
 CHECK_INCLUDE_FILE(GL/glut.h HAVE_GLUT_H)
 CHECK_INCLUDE_FILE(GLUT/glut.h HAVE_OSXGLUT_H)
 CHECK_INCLUDE_FILE(X11/extensions/xf86vmode.h HAVE_XF86VMODE_H)
endif(WIN32)

# create config.h file
CONFIGURE_FILE(src/config.h.in.cmake ${CMAKE_BINARY_DIR}/src/config.h)

# continue processing in src subdirectory
add_subdirectory (src)

SET(CPACK_PACKAGE_VERSION_MAJOR "${V_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${V_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${V_MAJOR}.${V_MINOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME
"freepv-${V_MAJOR}.${V_MINOR}.${V_PATCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES
"/_CPack_Packages/"
"/CMakeFiles/"
"/testcases/"
"/cmake_install\\\\.cmake"
"/CMakeCache.txt$"
"/CMakeOutput.log$"
"/CPackConfig.cmake$"
"/CPackSourceConfig.cmake$"
"\\\\.dir/"
"\\\\.tar\\\\.gz$"
"\\\\.tar\\\\.Z$"
"\\\\.svn/"
"\\\\.cvsignore$"
"\\\\.swp$"
"~$"
"\\\\.#"
"/#"
"/freepv-glut$"
"/freepv-glx$"
"/qtvr2img$"
"/build/"
"/CVS/"
"/\\\\.libs/"
"/\\\\.deps/"
"\\\\.o$"
"\\\\.a$"
"\\\\.lo$"
"\\\\.so$"
"\\\\.la$"
"/Makefile\\\\.in$"
"/Makefile$"
 )
INCLUDE(CPack)
