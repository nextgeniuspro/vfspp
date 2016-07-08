cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(vfspp)

set(VERSION "1.0.0")

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Installation directory for headers")
set(INSTALL_MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man" CACHE PATH "Installation directory for manual pages")
set(INSTALL_PKGCONFIG_DIR "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" CACHE PATH "Installation directory for pkgconfig (.pc) files")

#============================================================================
# zlib
#============================================================================

set(ZLIB_HDRS_DIR zlib/src/)
set(ZLIB_SRCS_DIR zlib/src/)
set(MINIZIP_HDRS_DIR zlib/src/contrib/minizip/)
set(MINIZIP_SRCS_DIR zlib/src/contrib/minizip/)

set(ZLIB_PUBLIC_HDRS
    ${ZLIB_HDRS_DIR}/zconf.h
    ${ZLIB_HDRS_DIR}/zlib.h
)

set(ZLIB_PRIVATE_HDRS
    ${ZLIB_HDRS_DIR}/crc32.h
    ${ZLIB_SRCS_DIR}/deflate.h
    ${ZLIB_HDRS_DIR}/gzguts.h
    ${ZLIB_HDRS_DIR}/inffast.h
    ${ZLIB_HDRS_DIR}/inffixed.h
    ${ZLIB_HDRS_DIR}/inflate.h
    ${ZLIB_HDRS_DIR}/inftrees.h
    ${ZLIB_HDRS_DIR}/trees.h
    ${ZLIB_HDRS_DIR}/zutil.h
)

set(ZLIB_SRCS
    ${ZLIB_SRCS_DIR}/adler32.c
    ${ZLIB_SRCS_DIR}/compress.c
    ${ZLIB_SRCS_DIR}/crc32.c
    ${ZLIB_SRCS_DIR}/deflate.c
    ${ZLIB_SRCS_DIR}/gzclose.c
    ${ZLIB_SRCS_DIR}/gzlib.c
    ${ZLIB_SRCS_DIR}/gzread.c
    ${ZLIB_SRCS_DIR}/gzwrite.c
    ${ZLIB_SRCS_DIR}/inflate.c
    ${ZLIB_SRCS_DIR}/infback.c
    ${ZLIB_SRCS_DIR}/inftrees.c
    ${ZLIB_SRCS_DIR}/inffast.c
    ${ZLIB_SRCS_DIR}/trees.c
    ${ZLIB_SRCS_DIR}/uncompr.c
    ${ZLIB_SRCS_DIR}/zutil.c
)

set(MINIZIP_HDRS
    ${MINIZIP_HDRS_DIR}/crypt.h
    ${MINIZIP_HDRS_DIR}/ioapi.h
    ${MINIZIP_HDRS_DIR}/iowin32.h
    ${MINIZIP_HDRS_DIR}/mztools.h
    ${MINIZIP_HDRS_DIR}/unzip.h
    ${MINIZIP_HDRS_DIR}/zip.h
)

set(MINIZIP_SRCS
    ${MINIZIP_SRCS_DIR}/ioapi.c
    ${MINIZIP_SRCS_DIR}/iowin32.c
    ${MINIZIP_SRCS_DIR}/mztools.c
    ${MINIZIP_SRCS_DIR}/unzip.c
    ${MINIZIP_SRCS_DIR}/zip.c
)

#============================================================================
# vfspp
#============================================================================

set(LIBVFSPP_PUBLIC_HDRS_DIR include/)
set(LIBVFSPP_PRIVATE_HDRS_DIR src/)
set(LIBVFSPP_SRCS_DIR src/)

set(LIBVFSPP_PUBLIC_HDRS
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CFileInfo.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CMemoryFile.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CMemoryFileSystem.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CNativeFile.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CNativeFileSystem.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CVirtualFileSystem.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CZipFile.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/CZipFileSystem.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/Global.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/IFile.h
    ${LIBVFSPP_PUBLIC_HDRS_DIR}/IFileSystem.h
)

set(LIBVFSPP_PRIVATE_HDRS
    ${LIBVFSPP_PRIVATE_HDRS_DIR}/CStringUtils.h
)

set(LIBVFSPP_SRCS
    ${LIBVFSPP_SRCS_DIR}/CFileInfo.cpp
    ${LIBVFSPP_SRCS_DIR}/CMemoryFile.cpp
    ${LIBVFSPP_SRCS_DIR}/CMemoryFileSystem.cpp
    ${LIBVFSPP_SRCS_DIR}/CNativeFile.cpp
    ${LIBVFSPP_SRCS_DIR}/CNativeFileSystem.cpp
    ${LIBVFSPP_SRCS_DIR}/CStringUtils.cpp
    ${LIBVFSPP_SRCS_DIR}/CVirtualFileSystem.cpp
    ${LIBVFSPP_SRCS_DIR}/CZipFile.cpp
    ${LIBVFSPP_SRCS_DIR}/CZipFileSystem.cpp
)

include_directories(${LIBVFSPP_PUBLIC_HDRS_DIR})
include_directories(${ZLIB_HDRS_DIR})
include_directories(${MINIZIP_HDRS_DIR})

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

#if(__APPLE__)
    set(CMAKE_MACOSX_RPATH 1)
#endif

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -stdlib=libc++ -fno-rtti -g -Wall")

add_library(${PROJECT_NAME} STATIC ${LIBVFSPP_SRCS} ${LIBVFSPP_HDRS})
add_library(zlib ${ZLIB_SRCS} ${MINIZIP_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS} ${MINIZIP_HDRS})
target_link_libraries(${PROJECT_NAME} zlib)

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
endif()
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(FILES ${LIBVFSPP_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
endif()

#============================================================================
# Example binaries
#============================================================================

#if(WITH_EXAMPLES)
    set(EXAMPLES_DIR examples/)

    add_executable(example ${EXAMPLES_DIR}/example.cpp)
    target_link_libraries(example ${PROJECT_NAME})
#endif