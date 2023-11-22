#!/bin/sh
git submodule init
git submodule update

export CMAKE_GENERATOR="Unix Makefiles"

mkdir -p vendor/sdsl-lite-built
cd vendor/sdsl-lite/
./install.sh ../sdsl-lite-built
cd ../../

mkdir -p vendor/kahip-built/lib
cd vendor/KaHIP/
./compile_withcmake.sh
cp deploy/libkahip.a ../kahip-built/lib/libkahip.a
cd ../../

