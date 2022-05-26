
# Make sure the compiler can find include files from our freepv library.
INCLUDE_DIRECTORIES(${FreePV_SOURCE_DIR}/src/libfreepv)

# Make sure the linker can find the Hello library once it is built.
LINK_DIRECTORIES(${FreePV_BINARY_DIR}/Hello)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx".  The extensions are automatically found.
ADD_EXECUTABLE(freepv-glx glx_platform.cpp main.cpp)

# Link the executable to the Hello library.
TARGET_LINK_LIBRARIES(freepv-glx freepv 
                      ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${LIBXML2_LIBRARIES}
                      ${OPENGL_LIBRARIES} ${X11_LIBRARIES} ${PNG_LIBRARIES}
                      ${GECKO_LIBRARIES} ${XF86VM_LIBRARIES}
                     )

INSTALL(TARGETS freepv-glx DESTINATION bin)

