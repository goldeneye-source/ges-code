#!/bin/bash

OLD_PATH=$PATH

TOP=$(cd "${0%/*}" && echo "${PWD}/../steam-runtime-sdk")

if [ -z "${STEAM_RUNTIME_HOST_ARCH}" ]; then
    STEAM_RUNTIME_HOST_ARCH=$(dpkg --print-architecture)
fi
export STEAM_RUNTIME_HOST_ARCH

export STEAM_RUNTIME_TARGET_ARCH=i386

# The top level of the Steam runtime tree
if [ -z "${STEAM_RUNTIME_ROOT}" ]; then
    if [ -d "${TOP}/runtime/${STEAM_RUNTIME_TARGET_ARCH}" ]; then
        STEAM_RUNTIME_ROOT="${TOP}/runtime/${STEAM_RUNTIME_TARGET_ARCH}"
    fi
fi
if [ ! -d "${STEAM_RUNTIME_ROOT}" ]; then
    echo "$0: ERROR: Couldn't find steam runtime directory"
    if [ ! -d "${TOP}/runtime/${STEAM_RUNTIME_TARGET_ARCH}" ]; then
        echo "Do you need to run setup.sh to download the ${STEAM_RUNTIME_TARGET_ARCH} target?" >&2
    fi
    exit 2
fi
export STEAM_RUNTIME_ROOT

export PATH="${TOP}/bin:$PATH"

cd thirdparty/python3
./build_python3.sh
cd ../../

make -f games.mak

if [ -d "$GES_PATH" ]; then
    echo "Deploying binaries to GE:S directory..."
    cp -v ./bin/mod_ges/client.so* $GES_PATH/bin/
    cp -v ./bin/mod_ges/server.so* $GES_PATH/bin/
    cp -v ./bin/mod_ges/libpython* $GES_PATH/bin/
else
    echo "Cannot deploy binaries since GES_PATH is unset or non-existant!"
fi

echo "Cleaning up..."
export PATH=$OLD_PATH
export STEAM_RUNTIME_ROOT=

echo "GE:S Build Complete!"
