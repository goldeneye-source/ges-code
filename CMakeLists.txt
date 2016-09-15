project(gesource)

cmake_minimum_required(VERSION 2.8.12)

set(GESOURCE_VERSION "5.0.0")

# Default to a release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE
        STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Global definitions
add_definitions(-DVPROF_LEVEL=1 -DSWDS -D_snprintf=use_Q_snprintf_instead -Dstricmp=strcasecmp -D_stricmp=strcasecmp
  -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_vsnprintf=vsnprintf -Dstrcmpi=strcasecmp -D_USRDLL -DVECTOR)

# Platform specific definitions
if (CMAKE_HOST_UNIX)
  add_definitions(-D_LINUX -DLINUX -D_alloca=alloca -DARCH=i486)
  add_compile_options(--std=c++11 -w -ffast-math -fpermissive -msse4)
else()
  add_definitions(-DWIN32 -D_WIN32 -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

# Boost package defines
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.54.0)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Could not find Boost libraries. Download and install at least version 1.54.0 to continue.")
endif()

# Python defines
set(PYTHON_VERSION 3.5.2 CACHE INTERNAL "")
set(BUILD_EXTENSIONS_AS_BUILTIN ON CACHE INTERNAL "")
set(ENABLE_CTYPES OFF CACHE INTERNAL "")
set(ENABLE_CTYPES_TEST OFF CACHE INTERNAL "")
set(ENABLE_CURSES OFF CACHE INTERNAL "")
set(ENABLE_CURSES_PANEL OFF CACHE INTERNAL "")
set(ENABLE_BZ2 OFF CACHE INTERNAL "")
set(ENABLE_SSL OFF CACHE INTERNAL "")
set(ENABLE_HASHLIB OFF CACHE INTERNAL "")
set(ENABLE_CRYPT OFF CACHE INTERNAL "")
set(INSTALL_MANUAL OFF CACHE INTERNAL "")
set(INSTALL_TEST OFF CACHE INTERNAL "")
set(WITH_THREAD OFF CACHE INTERNAL "")

# Add in the projects
add_subdirectory("thirdparty/python")
add_subdirectory("thirdparty/boost-python")
#add_subdirectory("mathlib")
#add_subdirectory("tier1")
#add_subdirectory("game")
