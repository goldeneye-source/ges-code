#!/bin/sh

mkdir ges_build
cd ges_build

# Only configure if we haven't done it yet
if ! [ -f "Makefile" ]; then
  ../configure --prefix=`pwd`/bin --enable-shared
else
  echo No need to configure!
  sleep 1
fi

# Build Python
make
make install

echo Build complete, moving relevant files...
sleep 1

# Copy the python config file
cp ./bin/include/python3.4m/pyconfig.h ../Include/pyconfig.h
chmod 0664 ../Include/pyconfig.h

# Copy the link library
cp ./bin/lib/libpython3.so ../../../lib/ges/linux32/libpython3.so
chmod 0664 ../../../lib/ges/linux32/libpython3.so

# Copy the shared library (eventually goes into gesource\bin)
cp ./bin/lib/libpython3.4m.so.1.0 ../../../bin/mod_ges/libpython3.4m.so
chmod 0664 ../../../bin/mod_ges/libpython3.4m.so
