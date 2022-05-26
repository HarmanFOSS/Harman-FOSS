
# Make sure the compiler can find include files from our freepv library.
INCLUDE_DIRECTORIES(${FreePV_SOURCE_DIR}/src/libfreepv)

# Make sure the linker can find the Hello library once it is built.
LINK_DIRECTORIES(${FreePV_BINARY_DIR}/Hello)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx".  The extensions are automatically found.
ADD_EXECUTABLE(freepv-glut freepv-glut.cpp glut_platform.cpp)

# Link the executable to the Hello library.
TARGET_LINK_LIBRARIES(freepv-glut freepv 
                      ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${LIBXML2_LIBRARIES}
                      ${OPENGL_LIBRARIES} ${FREEGLUT_LIBRARIES} ${PNG_LIBRARIES})

INSTALL(TARGETS freepv-glut DESTINATION bin)
INSTALL(FILES freepv-glut.desktop DESTINATION share/applications)

