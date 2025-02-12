#!/usr/bin/sh

builddir=build

cxx=c++
std=-std=gnu++20 # designated initializers
libs=-l:'libraylib.a'
wflags=-Wno-missing-field-initializers
iflags=-Ithirdparty/raylib/include
libpaths=-Lthirdparty/raylib/lib
cflags="$std $wflags $iflags -Wall -Wextra -Wpedantic"

set -xe

$cxx $cflags -o $builddir/draw.o -c draw.cpp
$cxx $cflags -o $builddir/draw $builddir/draw.o $libs $libpaths
