#TODO: Be able to build freepv-mozilla under windows


add_library(npfreepv SHARED 
            plugin.cpp 
            plugin_unix.cpp
	    np_entry.cpp
	    npn_gate.cpp
	    npp_gate.cpp
	    )

TARGET_LINK_LIBRARIES(npfreepv freepv ${ZLIB_LIBRARIES} 
				      ${JPEG_LIBRARIES}
				      ${LIBXML2_LIBRARIES}
		                      ${OPENGL_LIBRARIES} 
				      ${X11_LIBRARIES}
				      ${GECKO_LIBRARIES}
					${PNG_LIBRARIES})

INSTALL(TARGETS npfreepv DESTINATION ${LIBDIR}/mozilla/plugins)
