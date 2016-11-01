project(gesource)

cmake_minimum_required(VERSION 2.8.12)
set(GESOURCE_VERSION "5.0.0")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include("ucm")
include("ExternalProject")

# Default to a release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE
        STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Global definitions
add_definitions(-DVPROF_LEVEL=1 -DSWDS -D_snprintf=use_Q_snprintf_instead -DRAD_TELEMETRY_DISABLED -D_USRDLL -DVECTOR)

# Platform specific definitions
if(CMAKE_HOST_UNIX)
  add_definitions(-D_LINUX -DLINUX -D_POSIX -DPOSIX -DGNUC -DCOMPILER_GCC -DVPROF_LEVEL=1 -DGNUC -DNO_HOOK_MALLOC
    -DNO_MALLOC_OVERRIDE -D_DLL_EXT=.so)

  add_compile_options(-w -std=gnu++0x -fpermissive -fno-strict-aliasing -ffast-math -fno-omit-frame-pointer
    -ftree-vectorize -fpredictive-commoning -funswitch-loops -m32 -march=core2 -mtune=core2 -msse2 -mfpmath=sse)

#  add_compile_options(-Wall -Wno-invalid-offsetof -Wno-multichar -Wno-overloaded-virtual -Wno-narrowing -Wno-write-strings
#  	-Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-local-typedefs -Wno-unused-result
#    -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-value -Wno-missing-field-initializers
#    -Wno-sign-compare -Wno-reorder -Wno-invalid-offsetof -Wno-float-equal -Werror=return-type -Wno-attributes
#    -fdiagnostics-show-option -Wformat -Wformat-security)
else()
  add_definitions(-DWIN32 -D_WIN32 -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_DLL_EXT=.dll)
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

# Configure, build, and "install" Python
ExternalProject_Add(python
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/python"
  CMAKE_ARGS "-C${CMAKE_SOURCE_DIR}/cmake/ges_python.cmake" 
	"-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/python/"
    "-DCMAKE_C_FLAGS=-m32"
  BUILD_COMMAND ""
  INSTALL_COMMAND "${CMAKE_COMMAND}"
      --build .
      --target install
      --config Release
  PREFIX "python"
)

include_directories("${CMAKE_BINARY_DIR}/python/include/python3.5/")
link_directories("${CMAKE_BINARY_DIR}/python/lib/")

# Add in the projects
add_subdirectory("thirdparty/boost-python")
#add_subdirectory("mathlib")
add_subdirectory("tier1")
add_subdirectory("vgui2")
add_subdirectory("game")
