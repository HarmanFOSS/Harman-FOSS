# CMakeLists.txt for libsndfile

cmake_minimum_required (VERSION 2.8.12)

project (libsndfile)

if (NOT (EXISTS "${CMAKE_SOURCE_DIR}/CMake/config.h.in"))
	message (FATAL_ERROR "\nError : Need to run \"./autogen.sh cmake\" first.")
	endif ()

if (DEFINED ENV{CC})
	set (CMAKE_C_COMPILER $ENV{CC})
	endif()

if (DEFINED ENV{CXX})
	set (CMAKE_CXX_COMPILER $ENV{CXX})
	endif()

message (STATUS "Using C compiler   : " ${CMAKE_C_COMPILER})
message (STATUS "Using C++ compiler : " ${CMAKE_CXX_COMPILER})

#-------------------------------------------------------------------------------

set (LIB_VERSION_MAJOR 1)
set (LIB_VERSION_MINOR 0)
set (LIB_VERSION_PATCH 27)

set (PACKAGE_NAME \"libsndfile\")
set (PACKAGE_VERSION \"${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH}pre1\")

set (BASEPATH "${CMAKE_SOURCE_DIR}")


set (LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

include (CMake/libsndfile.cmake)
include (CMake/build.cmake)

if (MSVC)
	set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4996")
	add_definitions ("/wd4244 /wd4996")

	set (COMPILER_IS_GCC 0)
	set (HAVE_EXTERNAL_XIPH_LIBS 0)
	set (OS_IS_WIN32 1)
	set (OSX_DARWIN_VERSION 0)
	set (USE_WINDOWS_API 1)
	set (WIN32_TARGET_DLL 1)
else (MSVC)
	# Assume this is Linux or some other POSIX-like system (including OS X).
	set (HAVE_EXTERNAL_XIPH_LIBS 0)
	set (OSX_DARWIN_VERSION 0)
	set (USE_WINDOWS_API 0)
	set (__USE_MINGW_ANSI_STDIO 0)

	set (CPU_CLIPS_NEGATIVE 0)
	set (CPU_CLIPS_POSITIVE 0)
	set (_POSIX_SOURCE 1)

	set (SF_COUNT_MAX 0x7fffffffffffffffll)
	set (ENABLE_EXPERIMENTAL_CODE 0)
	set (OSX_DARWIN_VERSION 0)
	set (OS_IS_OPENBSD 0)
	set (WIN32_TARGET_DLL 0)
	endif (MSVC)

if (${COMPILER_IS_GCC})
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu99 -Wall -Wextra" CACHE STRING "" FORCE)
	# set (CMAKE_CXX__FLAGS
	endif (${COMPILER_IS_GCC})

if (${Werror} MATCHES "on")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror" CACHE STRING "" FORCE)
	endif ()

add_custom_target (check
	COMMAND src/test_main && bash tests/test_wrapper.sh
	DEPENDS src/test_main tests/test_wrapper.sh
	)

#-------------------------------------------------------------------------------
# Project definitions follow.

configure_file (${CMAKE_SOURCE_DIR}/src/sndfile.h.in ${CMAKE_SOURCE_DIR}/src/sndfile.h)
configure_file (${CMAKE_SOURCE_DIR}/CMake/config.h.in ${CMAKE_SOURCE_DIR}/src/config.h)

include_directories (src)

set (libsndfile_sources
	src/ALAC/ALACBitUtilities.c
	src/ALAC/ag_dec.c
	src/ALAC/ag_enc.c
	src/ALAC/alac_decoder.c
	src/ALAC/alac_encoder.c
	src/ALAC/dp_dec.c
	src/ALAC/dp_enc.c
	src/ALAC/matrix_dec.c
	src/ALAC/matrix_enc.c

	src/G72x/g721.c
	src/G72x/g723_16.c
	src/G72x/g723_24.c
	src/G72x/g723_40.c
	src/G72x/g72x.c
	src/G72x/g72x_test.c

	src/GSM610/add.c
	src/GSM610/code.c
	src/GSM610/decode.c
	src/GSM610/gsm_create.c
	src/GSM610/gsm_decode.c
	src/GSM610/gsm_destroy.c
	src/GSM610/gsm_encode.c
	src/GSM610/gsm_option.c
	src/GSM610/long_term.c
	src/GSM610/lpc.c
	src/GSM610/preprocess.c
	src/GSM610/rpe.c
	src/GSM610/short_term.c
	src/GSM610/table.c

	src/aiff.c
	src/alac.c
	src/alaw.c
	src/au.c
	src/audio_detect.c
	src/avr.c
	src/broadcast.c
	src/caf.c
	src/cart.c
	src/chanmap.c
	src/chunk.c
	src/command.c
	src/common.c
	src/dither.c
	src/double64.c
	src/dwd.c
	src/dwvw.c
	src/file_io.c
	src/flac.c
	src/float32.c
	src/g72x.c
	src/gsm610.c
	src/htk.c
	src/id3.c
	src/ima_adpcm.c
	src/ima_oki_adpcm.c
	src/interleave.c
	src/ircam.c
	src/macos.c
	src/mat4.c
	src/mat5.c
	src/mpc2k.c
	src/ms_adpcm.c
	src/new.c
	src/nist.c
	src/ogg.c
	src/ogg_opus.c
	src/ogg_pcm.c
	src/ogg_speex.c
	src/ogg_vorbis.c
	src/paf.c
	src/pcm.c
	src/pvf.c
	src/raw.c
	src/rf64.c
	src/rx2.c
	src/sd2.c
	src/sds.c
	src/sndfile.c
	src/strings.c
	src/svx.c
	src/txw.c
	src/ulaw.c
	src/voc.c
	src/vox_adpcm.c
	src/w64.c
	src/wav.c
	src/wavlike.c
	src/windows.c
	src/wve.c
	src/xi.c
	)

add_library (sndfile SHARED
	${libsndfile_sources}
	)

target_link_libraries (sndfile m)

set_target_properties(sndfile
	PROPERTIES
	VERSION ${LIB_VERSION_STRING} SOVERSION ${LIB_VERSION_MAJOR}
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src
	)

#-------------------------------------------------------------------------------
# Programs.

lsf_build_program (sndfile-cmp)
lsf_build_program (sndfile-concat)
lsf_build_program (sndfile-convert)
lsf_build_program (sndfile-deinterleave)
lsf_build_program (sndfile-info)
lsf_build_program (sndfile-interleave)
lsf_build_program (sndfile-metadata-get)
lsf_build_program (sndfile-metadata-set)
lsf_build_program (sndfile-salvage)
lsf_build_program_extra (sndfile-play asound)

#-------------------------------------------------------------------------------
# Tests.

configure_file (tests/test_wrapper.sh.in tests/test_wrapper.sh)

# Tests from the src/ directory.
set (src_test_sources
	src/audio_detect.c
	src/broadcast.c
	src/cart.c
	src/common.c
	src/double64.c
	src/file_io.c
	src/float32.c
	src/test_audio_detect.c
	src/test_broadcast_var.c
	src/test_cart_var.c
	src/test_conversions.c
	src/test_endswap.c
	src/test_file_io.c
	src/test_float.c
	src/test_ima_oki_adpcm.c
	src/test_log_printf.c
	src/test_strncpy_crlf.c
	)

lsf_build_src_test_c (test_main "${src_test_sources}")

# Tests from the tests/ directory.

lsf_build_test_c (aiff_rw_test "")
lsf_build_test_c (alaw_test "")
lsf_build_test_c (benchmark "")
lsf_build_test_c (channel_test "")
lsf_build_test_c (checksum_test "")
lsf_build_test_c (chunk_test "")
lsf_build_test_c (command_test "")
lsf_build_test_c (compression_size_test "")
lsf_build_test_cc (cpp_test "")
lsf_build_test_c (dither_test "")
lsf_build_test_c (dwvw_test "")
lsf_build_test_c (error_test "")
lsf_build_test_c (external_libs_test "")
lsf_build_test_c (fix_this "")
lsf_build_test_c (floating_point_test "tests/dft_cmp.c")
lsf_build_test_c (format_check_test "")
lsf_build_test_c (headerless_test "")
lsf_build_test_c (header_test "")
lsf_build_test_c (largefile_test "")
lsf_build_test_c (locale_test "")
lsf_build_test_c (lossy_comp_test "")
lsf_build_test_c (misc_test "")
lsf_build_test_c (multi_file_test "")
lsf_build_test_c (ogg_test "")
lsf_build_test_c (pcm_test "")
lsf_build_test_c (peak_chunk_test "")
lsf_build_test_c (pipe_test "")
lsf_build_test_c (raw_test "")
lsf_build_test_c (rdwr_test "")
lsf_build_test_c (scale_clip_test "")
lsf_build_test_c (sfversion "")
lsf_build_test_c (stdin_test "")
lsf_build_test_c (stdio_test "")
lsf_build_test_c (stdout_test "")
lsf_build_test_c (string_test "")
lsf_build_test_c (ulaw_test "")
lsf_build_test_c (virtual_io_test "")
lsf_build_test_c (win32_ordinal_test "")
lsf_build_test_c (win32_test "")
lsf_build_test_c (write_read_test "tests/generate.c")
