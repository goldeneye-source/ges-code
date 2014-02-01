#!/bin/bash
#
# This script runs a shell with the environment set up for the Steam runtime 
# development environment.

# The top level of the cross-compiler tree
#TOP=$(cd "${0%/*}" && echo "${PWD}")
(
TOP=$(cd "${0%/*}" && echo "${PWD}/../steam-runtime-sdk")

STEAM_RUNTIME_TARGET_ARCH=i386

export STEAM_RUNTIME_TARGET_ARCH

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

make -f games.mak
)
