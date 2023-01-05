#!/bin/sh

# Run this script at project root
#meson configure build
#CXX=clang++ meson compile -C build -j$(proc)



# CMake/ninja build system
mkdir -p build
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja
ninja -C build

# Create symlinks to executables in build folder if necessary
ln -s ../build/gdpm bin/gdpm
ln -s ../build/gdpm-tests bin/gdpm-tests