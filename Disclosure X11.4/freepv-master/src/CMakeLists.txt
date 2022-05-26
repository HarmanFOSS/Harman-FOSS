
# continue processing in the various module directories

add_subdirectory (libfreepv)
add_subdirectory (freepv-glut)
add_subdirectory (utils)
if(UNIX)
	add_subdirectory (freepv-glx)
endif(UNIX)

