#!/bin/bash

pushd /opt/steam/SteamApps/common/Source\ SDK\ Base\ 2013\ Multiplayer

export GAME_DEBUGGER=gdb
export LD_LIBRARY_PATH="../../sourcemods/gesource_release/bin"

./hl2.sh -game ../../sourcemods/gesource_release/

export GAME_DEBUGGER=
export LD_LIBRARY_PATH=

popd
