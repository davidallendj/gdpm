#!/bin/sh

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
exe=gdpm
static=gdpm.static
tests=gdpm.tests

function test_link(){
	path=$1
	link=$2
	if test -f "$path"
	then
		echo "Creating link from '$path' to '$link')"
		if test -f "$link"
		then
			rm $link
		fi
		ln -s $path $link
	fi
}

function test_strip(){
	path=$1
	if test -f "$path" 
	then
		echo "Stripping debug symbols from '$path'" 
		strip "$path"
	fi
}

# Run this script at project root
#meson configure build
#CXX=clang++ meson compile -C build -j$(proc)


# CMake/ninja build system
mkdir -p build
cmake -B build -S . -D CMAKE_EXPORT_COMPILE_COMMANDS=1 -D CMAKE_BUILD_TYPE=Debug -G Ninja
ninja -C build -j $(nproc)


# Create symlinks to executables in build folder if necessary
test_link $script_dir/../build/gdpm $script_dir/../bin/$exe
test_link $script_dir/../build/gdpm.static $script_dir/../bin/$static
test_link $script_dir/../build/gdpm.tests $script_dir/../bin/$tests


# Strip debug symbols
test_strip ${script_dir}/../build/gdpm
test_strip ${script_dir}/../build/gdpm.static
test_strip ${script_dir}/../build/gdpm.tests


# Generate documentation using `doxygen`
cd ${script_dir}/..
doxygen
