# Privexec cmake
cmake_minimum_required(VERSION 3.22)

project(Privexec)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
  message(
    FATAL_ERROR
      "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with Privexec.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(PRIVEXEC_ENABLE_LTO OFF)
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if(lto_supported)
  message(STATUS "IPO/LTO supported")
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PRIVEXEC_ENABLE_LTO ON)
    set(BELA_ENABLE_LTO ON)
    message(STATUS "IPO/LTO enabled")
  endif()
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY_DEFAULT "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS=1 -utf-8 -W3 -DUNICODE=1 -D_UNICODE=1 -wd26812")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS=1 -utf-8 -W3 -permissive- -Zc:__cplusplus -DUNICODE=1 -D_UNICODE=1 -wd26812"
  )
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c65001")
endif(MSVC)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# Gen version
include(VersionFromVCS)
get_source_info(${CMAKE_CURRENT_SOURCE_DIR} PRIVEXEC_REVISION PRIVEXEC_REMOTE_URL PRIVEXEC_REFNAME)
string(TIMESTAMP PRIVEXEC_COPYRIGHT_YEAR "%Y")

# PRIVEXEC_REFNAME maybe defined by ci
if("${PRIVEXEC_REFNAME}" STREQUAL "none")
  if(DEFINED ENV{PRIVEXEC_REFNAME})
    set(PRIVEXEC_REFNAME $ENV{PRIVEXEC_REFNAME})
  endif()
endif()

if("${PRIVEXEC_REFNAME}" MATCHES "refs/tags/*")
  string(TIMESTAMP PRIVEXEC_BUILD_TIME "%Y-%m-%dT%H:%M:%SZ")
else()
  set(PRIVEXEC_BUILD_TIME "none")
endif()

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(PRIVEXEC_VERSION_BUILD $ENV{GITHUB_RUN_NUMBER})
  message(STATUS "Privexec detect GITHUB_RUN_NUMBER: $ENV{GITHUB_RUN_NUMBER}")
else()
  set(PRIVEXEC_VERSION_BUILD 261)
endif()

#
set(PRIVEXEC_VERSION_MAJOR 5)
set(PRIVEXEC_VERSION_MINOR 0)
set(PRIVEXEC_VERSION_PATCH 4)
set(PACKAGE_VERSION "${PRIVEXEC_VERSION_MAJOR}.${PRIVEXEC_VERSION_MINOR}.${PRIVEXEC_VERSION_PATCH}")

string(TOLOWER "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" COMPILER_ARCH_ID)
set(CPACK_SYSTEM_NAME "win-${COMPILER_ARCH_ID}")
set(CPACK_PACKAGE_NAME "Privexec")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Privexec")
set(CPACK_PACKAGE_VERSION_MAJOR ${PRIVEXEC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PRIVEXEC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PRIVEXEC_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VENDOR "Privexec")
set(CPACK_PACKAGE_CONTACT "Privexec contributors")

include(CPack)
# Generate version code
configure_file(${CMAKE_SOURCE_DIR}/include/baseversion.h.cmake ${CMAKE_BINARY_DIR}/include/baseversion.h)

include_directories(
  "${CMAKE_BINARY_DIR}/include"
  ./include
  ./vendor/bela/include
  ./vendor/pugixml
  ./lib/exec)

configure_file("config/Privexec.json" "${CMAKE_BINARY_DIR}/bin/Privexec.json" COPYONLY)

install(FILES "config/Privexec.json" DESTINATION .)

# File context menu integration script
install(FILES "script/runassys.bat" DESTINATION script)
install(FILES "script/runassys.ps1" DESTINATION script)
install(FILES "script/runasti.bat" DESTINATION script)
install(FILES "script/runasti.ps1" DESTINATION script)
install(FILES "script/uninstall.bat" DESTINATION script)

add_subdirectory(vendor/bela)
add_subdirectory(lib/exec)
add_subdirectory(AppExec)
add_subdirectory(Privexec)
add_subdirectory(wsudo)
if(NOT PRIVEXEC_ENABLE_LTO)
  add_subdirectory(test/apc)
endif()
