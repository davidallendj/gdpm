#!/bin/sh

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
exe=gdpm
static=gdpm.static
tests=gdpm.tests

function link_exe(){
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

function strip(){
	path=$1
	if test -f "$path" 
	then
		echo "Stripping debug symbols from '$path'" 
		strip "$path"
	fi
}

function generate_docs(){
	# Generate documentation using `doxygen`
	cd ${script_dir}/..
	doxygen
}

function strip_all(){
	# Strip debug symbols
	strip ${script_dir}/../build/gdpm
	strip ${script_dir}/../build/gdpm.static
	strip ${script_dir}/../build/gdpm.tests
}

function link_all(){
	# Create symlinks to executables in build folder if necessary
	link_exe $script_dir/../build/gdpm $script_dir/../bin/$exe
	link_exe $script_dir/../build/gdpm.static $script_dir/../bin/$static
	link_exe $script_dir/../build/gdpm.tests $script_dir/../bin/$tests
}

function clean(){
	rm ${script_dir}/../bin/$exe
	rm ${script_dir}/../bin/$static
	rm ${script_dir}/../bin/$tests
}

# Run this script at project root
#meson configure build
#CXX=clang++ meson compile -C build -j$(proc)


# CMake/ninja build system
function build_binaries(){
	mkdir -p build
	cmake -B build -S . -D CMAKE_EXPORT_COMPILE_COMMANDS=1 -D CMAKE_BUILD_TYPE=Debug -G Ninja
	ninja -C build -j $(nproc)
}


# Stop if no args
if [ $# -eq 0 ]
then
	exit 1
fi

# Handle arguemnts passed in
i=$(($#-1))
while [ $i -ge 0 ];
do
	case "$1" in
		-a|--all) build_binaries shift;;
		-c|--clean) clean shift;;
		-d|--docs)  generate_docs shift;;
		-s|--strip) strip_all shift;;
		-l|--link)  link_all shift;;
		--*) echo "Bad option: $1"
	esac
	i=$((i-1))
done
exit 0


