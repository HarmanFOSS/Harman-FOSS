
add_library(freepv STATIC PanoViewer.cpp Parameters.cpp Platform.cpp
                          OpenGLRenderer.cpp Image.cpp Scene.cpp
 			  SceneElement.cpp Behavior.cpp 
			  Action.cpp Subject.cpp EventProcessor.cpp
                          JpegReader.cpp utils.cpp SPiVparser.cpp
                          QTVRDecoder.cpp XMLparser.cpp
			  pngReader.cpp imageReader.cpp
			  Camera.cpp Utils/stringID.cpp
			  Utils/signatures.cpp
                          Error.cpp NodeElement.cpp
                          glutfont/freeglut_font.c
                          glutfont/freeglut_font_data.c
                          glutfont/freeglut_glutfont_definitions.c
			  Math/Matrix4.cpp Math/quaternion.cpp)

INSTALL(FILES Controller.h FPV_keysyms.h Image.h PanoViewer.h Parameters.h Platform.h QTVRDecoder.h Renderer.h Scene.h SPiVparser.h utils.h DESTINATION include/libfreepv)

INSTALL(TARGETS freepv DESTINATION ${LIBDIR})

