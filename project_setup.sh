#!/bin/sh
PREFIX=${PWD}/bin
CONF_OPTS+="-DPCH=0 "
CONF_OPTS+="-DDEBUG=1 "
CONF_OPTS+="-DACE_USE_EXTERNAL=1 "
CONF_OPTS+="-DUSE_STD_MALLOC=1 "
#CONF_OPTS+="-DUSE_TBB_MALLOC=1"
LIB_INSTALL_DIR=${PWD}/lib
INCLUDE_INSTALL_DIR=${PWD}/include

CFLAGS="-march=native -O2 -DNDEBUG"
#CFLAGS="-O1 -fno-inline"
CFLAGS+=" -pipe -ggdb -fno-strict-aliasing -fno-delete-null-pointer-checks -D_LARGEFILE_SOURCE"
CXXFLAGS="${CFLAGS}"

rm -Rf build &&
mkdir build &&
cd build &&

cmake .. ${CONF_OPTS} \
    -DPREFIX="${PREFIX}" \
    -DLIB_INSTALL_DIR="${LIB_INSTALL_DIR}" \
    -DINCLUDE_INSTALL_DIR="${INCLUDE_INSTALL_DIR}" \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_C_COMPILER="gcc" \
    -DCMAKE_CXX_COMPILER="g++"
