#!/bin/sh
apt update
apt install -y mingw-w64 cmake clang libuv1-dev libglfw3-dev libxinerama-dev libxcursor-dev libxi-dev
rm -rf build-win
mkdir build-win
cmake -S . -B build-win