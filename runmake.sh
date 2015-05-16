#!/bin/bash

# Stop the script if we run into any errors
set -e

if ! [ -d "${STEAM_RUNTIME_ROOT}" ]; then
    echo "You need to set STEAM_RUNTIME_ROOT to a valid directory in order to compile!" >&2
    exit 2
fi

# Store away the PATH variable for restoration
OLD_PATH=$PATH

# Set our host and target architectures
if [ -z "${STEAM_RUNTIME_HOST_ARCH}" ]; then
    STEAM_RUNTIME_HOST_ARCH=$(uname -m)
fi

if [ -z "$STEAM_RUNTIME_TARGET_ARCH" ]; then
  STEAM_RUNTIME_TARGET_ARCH=$STEAM_RUNTIME_HOST_ARCH
fi

#export STEAM_RUNTIME_HOST_ARCH
#export STEAM_RUNTIME_TARGET_ARCH

echo "Host architecture set to $STEAM_RUNTIME_HOST_ARCH"
echo "Target architecture set to $STEAM_RUNTIME_TARGET_ARCH"

# Check if our runtime is valid
if [ ! -d "${STEAM_RUNTIME_ROOT}/runtime/${STEAM_RUNTIME_TARGET_ARCH}" ]; then
    echo "$0: ERROR: Couldn't find steam runtime directory" >&2
    echo "Do you need to run setup.sh to download the ${STEAM_RUNTIME_TARGET_ARCH} target?" >&2
    exit 2
fi

export PATH="${STEAM_RUNTIME_ROOT}/bin:$PATH"

echo

# Build Python while in our runtime environment
pushd "thirdparty/python3" > /dev/null
echo "Building Python..."
./build_python3.sh
popd > /dev/null

echo

# Build GE:S
echo "Building GE:S..."
make -f games.mak

# Deploy to GES_PATH if a valid directory
if [ -d "$GES_PATH/bin" ]; then
    echo "Deploying binaries to GES_PATH..."
    cp -v ./bin/mod_ges/client.so* $GES_PATH/bin/
    cp -v ./bin/mod_ges/server.so* $GES_PATH/bin/
    cp -v ./bin/mod_ges/libpython* $GES_PATH/bin/
else
    echo "Cannot deploy binaries since GES_PATH is unset or non-existant!"
fi

echo "Cleaning up..."
export PATH=$OLD_PATH

echo "GE:S Build Complete!"
