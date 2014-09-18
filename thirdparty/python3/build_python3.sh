#!/bin/bash

if ! [ -d "./ges_build" ]; then
  mkdir ges_build
fi

pushd ges_build 1> /dev/null

# Only configure if we haven't done it yet
if ! [ -f "Makefile" ]; then
  LDFLAGS="-L ../Extras/lib32" ../configure --prefix=`pwd`/bin --enable-shared --build=i386-unknown-linux-gnu
else
  echo "No need to configure!"
  sleep 1
fi

if ! [ -f ./bin/lib/libpython3.4m.so.1.0 ]; then
  # Remove leftovers if they exist
  if [ -f "../Include/pyconfig.h" ]; then
    rm ../Include/pyconfig.h
  fi

  # Build Python
  make
  make install
else
  echo "No need to build Python!"
fi

echo "Deploying Python to build directories..."
sleep 1

# Link to the built pyconfig.h
ln -vfs ../ges_build/bin/include/python3.4m/pyconfig.h ../Include/pyconfig.h

# Copy the shared library (eventually deploys to $GES_PATH/bin)
cp -v ./bin/lib/libpython3.4m.so.1.0 ../../../bin/mod_ges/
chmod 0664 ../../../bin/mod_ges/libpython*

# Create a link to the library for gcc's use
ln -vfs ../../../bin/mod_ges/libpython3.4m.so.1.0 ../../../lib/ges/linux32/libpython3.so

popd 1> /dev/null
