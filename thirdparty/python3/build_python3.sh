#!/bin/sh

if ! [ -d "./ges_build" ]; then
  mkdir ges_build
fi

cd ges_build

# Only configure if we haven't done it yet
if ! [ -f "Makefile" ]; then
  LDFLAGS="-L ../Extras/lib32" ../configure --prefix=`pwd`/bin --enable-shared --build=i386-unknown-linux-gnu
else
  echo "No need to configure!"
  sleep 1
fi

# Remove leftovers if they exist
if [ -f "../Include/pyconfig.h" ]; then
  rm ../Include/pyconfig.h
fi

# Build Python
make
make install

echo "Build complete, moving relevant files..."
sleep 1

# Copy the python config file
cp -v ./bin/include/python3.4m/pyconfig.h ../Include/pyconfig.h
chmod 0664 ../Include/pyconfig.h

# Copy the shared library (eventually deploys to $GES_PATH/bin)
cp -v ./bin/lib/libpython3.4m.so.1.0 ../../../bin/mod_ges/
chmod 0664 ../../../bin/mod_ges/libpython*

# Create a link to the library for gcc's use
ln -vfs ../../../bin/mod_ges/libpython3.4m.so ../../../lib/ges/linux32/libpython3.so

cd ../
