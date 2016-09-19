MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
      PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
      OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
      PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
      OBJECT_DEPENDS "${PrecompiledBinary}")
  ENDIF(MSVC)
  # Add precompiled header to SourcesVar
  LIST(APPEND ${SourcesVar} ${PrecompiledSource})
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

set(SHARED_LIBS m z dl ldap rt util nsl)

if (${CMAKE_HOST_UNIX})
  link_directories("${CMAKE_SOURCE_DIR}/lib/public/linux32" "${CMAKE_SOURCE_DIR}/lib/common/linux32" "${CMAKE_SOURCE_DIR}/lib/ges/linux32")
else()
  link_directories("${CMAKE_SOURCE_DIR}/lib/public" "${CMAKE_SOURCE_DIR}/lib/common" "${CMAKE_SOURCE_DIR}/lib/ges")
endif()

set(CLIENT_LIBS fmod)

set(SERVER_LIBS
  choreoobjects
  particles
  dmxloader
  tier3
  tier2
  curl
  mathlib
  tier1
  boost_python
)

# Call these out separately for precompiled header on windows
ucm_add_dirs("server/ges/py" TO GEPY_SOURCES NO_HEADERS)

ucm_add_dirs("shared" TO SHARED_SOURCES NO_HEADERS RECURSIVE)
ucm_remove_directories("shared/sdk" "shared/episodic" FROM SHARED_SOURCES)
ucm_add_files(
  ../common/randoverride.cpp
  ../public/ScratchPadUtils.cpp
  ../public/SoundParametersInternal.cpp
  ../public/bone_setup.cpp
  ../public/collisionutils.cpp
  ../public/dt_utlvector_common.cpp
  ../public/filesystem_helpers.cpp
  ../public/interpolatortypes.cpp
  ../public/networkvar.cpp
  ../public/rope_physics.cpp
  ../public/scratchpad3d.cpp
  ../public/simple_physics.cpp
  ../public/stringregistry.cpp
  ../public/studio.cpp
  ../public/tier0/memoverride.cpp
TO SHARED_SOURCES)

ucm_add_dirs("server" TO SERVER_SOURCES NO_HEADERS RECURSIVE)
ucm_remove_directories("server/ges/py" "server/sdk" FROM SERVER_SOURCES)
ucm_add_files(
  ../public/dt_send.cpp
  ../public/dt_utlvector_send.cpp
  ../public/editor_sendcommand.cpp
  ../public/keyframe/keyframe.cpp
  ../public/map_utils.cpp
  ../public/registry.cpp
  ../public/server_class.cpp
TO SERVER_SOURCES)
LIST(APPEND SERVER_SOURCES ${SHARED_SOURCES})

ucm_add_dirs("client" TO CLIENT_SOURCES NO_HEADERS RECURSIVE)
ucm_remove_directories("client/sdk" FROM CLIENT_SOURCES)
ucm_add_files(
  ../common/compiledcaptionswap.cpp
  ../public/bone_accessor.cpp
  ../public/client_class.cpp
  ../public/crtmemdebug.cpp
  ../public/dt_recv.cpp
  ../public/dt_utlvector_recv.cpp
  ../public/jigglebones.cpp
  ../public/posedebugger.cpp
  ../public/sentence.cpp
  ../public/tools/bonelist.cpp
  ../public/vallocator.cpp
  ../public/vgui_controls/vgui_controls.cpp
TO CLIENT_SOURCES)
LIST(APPEND CLIENT_SOURCES ${SHARED_SOURCES})

ADD_MSVC_PRECOMPILED_HEADER("server/stdafx.h" "server/stdafx.cpp" SERVER_SOURCES)
ADD_MSVC_PRECOMPILED_HEADER("client/stdafx.h" "client/stdafx.cpp" CLIENT_SOURCES)

# Add the GE:S Python Sources to the server source list
ADD_MSVC_PRECOMPILED_HEADER("server/ges/py/ge_pyprecom.h" "server/ges/py/ge_pyprecom.cpp" GEPY_SOURCES)
LIST(APPEND SERVER_SOURCES ${GEPY_SOURCES})

# Shared definitions
add_definitions(-DHL2MP -DVERSION_SAFE_STEAM_API_INTERFACES -DGE_DLL -DGE_USE_ROLLINGEXP -DCURL_STATICLIB)

# Shared include directories
include_directories("../common" "../public" "../public/tier0" "../public/tier1" "../utils/common" "../thirdparty/curl"
  "shared/ges" "shared" "shared/hl2" "shared/hl2mp")

#######
# SERVER LIBRARY
#######
add_library(server SHARED ${SERVER_SOURCES})
add_dependencies(server python)

target_link_libraries(server -m32 ${SERVER_LIBS} ${SHARED_LIBS})
target_compile_definitions(server PUBLIC GAME_DLL HL2_DLL Py_ENABLE_SHARED GE_AI)
target_include_directories(server PUBLIC "server" "server/hl2" "server/hl2mp")
target_include_directories(server PUBLIC "server/ges" "server/ges/ai" "server/ges/py" "server/ges/ent")

if(CMAKE_HOST_UNIX)
  # Linux library name is server.so
  set_target_properties(server PROPERTIES PREFIX "")
  set_target_properties(server PROPERTIES OUTPUT_NAME server)

  # Link to pre-built libraries
  file(COPY "${CMAKE_SOURCE_DIR}/lib/public/linux32/libsteam_api.so" DESTINATION "${CMAKE_BINARY_DIR}/game/")
  add_library(steam_api SHARED IMPORTED)
  set_target_properties(steam_api PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/game/libsteam_api.so"
    IMPORTED_SONAME "libsteam_api.so")

  file(COPY "${CMAKE_SOURCE_DIR}/lib/public/linux32/libtier0.so" DESTINATION "${CMAKE_BINARY_DIR}/game/")
  add_library(tier0 SHARED IMPORTED)
  set_target_properties(tier0 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/game/libtier0.so"
  IMPORTED_SONAME "libtier0.so")

  file(COPY "${CMAKE_SOURCE_DIR}/lib/public/linux32/libvstdlib.so" DESTINATION "${CMAKE_BINARY_DIR}/game/")
  add_library(vstdlib SHARED IMPORTED)
  set_target_properties(vstdlib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/game/libvstdlib.so"
  IMPORTED_SONAME "libvstdlib.so")

  target_link_libraries(server steam_api tier0 vstdlib)
endif()

install(TARGETS server LIBRARY DESTINATION "bin")

#######
# CLIENT LIBRARY
#######
add_library(client SHARED ${CLIENT_SOURCES})
target_link_libraries(client -m32 ${SHARED_LIBS} ${CLIENT_LIBS})
target_compile_definitions(client PUBLIC CLIENT_DLL HL2_CLIENT_DLL ENABLE_HTMLWINDOW)
target_include_directories(client PUBLIC "client" "client/game_controls" "client/hl2" "client/hl2mp" "client/hl2mp/ui")
target_include_directories(client PUBLIC "client/ges" "client/ges/hud" "client/ges/vgui" "../vgui2/include" "../vgui2/controls")

if(CMAKE_HOST_UNIX)
  # Linux library name is client.so
  set_target_properties(client PROPERTIES PREFIX "")
  set_target_properties(client PROPERTIES OUTPUT_NAME client)
endif()

install(TARGETS client LIBRARY DESTINATION "bin")
