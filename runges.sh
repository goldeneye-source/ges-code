#!/bin/bash

if [[ -d "$STEAMAPPS_PATH" && -d "$GES_PATH" ]]; then
  pushd "$STEAMAPPS_PATH/common/Source SDK Base 2013 Multiplayer/"

  GAME_DEBUGGER=gdb LD_LIBRARY_PATH="$GES_PATH/bin" ./hl2.sh -game "$GES_PATH"

  popd
else
  echo "You need to define STEAMAPPS_PATH and GES_PATH to use this script!"
fi
