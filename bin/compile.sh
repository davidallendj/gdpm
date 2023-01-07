#!/bin/sh

# Run this script at project root
#meson configure build
#CXX=clang++ meson compile -C build -j$(proc)



# CMake/ninja build system
mkdir -p build
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja
ninja -C build -j $(nproc)

# Create symlinks to executables in build folder if necessary
if test -f "../build/gdpm"; then
	rm bin/gdpm
	ln -s ../build/gdpm bin/gdpm
fi

if test -f "../build/gdpm-tests"; then
	rm bin/gdpm-tests
	ln -s ../build/gdpm-tests bin/gdpm-tests
fi
