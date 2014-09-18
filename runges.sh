#!/bin/bash

if [[ -z "$STEAMAPPS_PATH" && -z "$GES_PATH" ]]; then
  pushd "$STEAMAPPS_PATH/common/Source SDK Base 2013 Multiplayer/"

  export GAME_DEBUGGER=gdb
  export LD_LIBRARY_PATH="$GES_PATH/bin"

  ./hl2.sh -game "$GES_PATH"

  export GAME_DEBUGGER=
  export LD_LIBRARY_PATH=

  popd
else
  echo "You need to define STEAMAPPS_PATH and GES_PATH to use this script!"
fi