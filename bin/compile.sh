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

function checksums(){
	sha256sums ${script_dir}/../bin/$exe
	sha256sums ${script_dir}/../bin/$static
	sha256sums ${script_dir}/../bin/$tests
}

# Run this script at project root
#meson configure build
#CXX=clang++ meson compile -C build -j$(proc)

PROCS=$(nproc)
CMAKE_COMMAND="cmake -B build -S . -D CMAKE_EXPORT_COMPILE_COMMANDS=1 -D CMAKE_BUILD_TYPE=Debug -G Ninja"
NINJA_COMMAND="ninja -C build -j ${PROCS}"

# CMake/ninja build system
function build_all(){
	mkdir -p build
	$CMAKE_COMMAND
	$NINJA_COMMAND
}


function build_exe(){
	mkdir -p build
	$CMAKE_COMMAND \
		--target gdpm \
		--target gdpm.static
	$NINJA_COMMAND
}


function build_libs(){
	mkdir -p build
	$CMAKE_COMMAND \
		--target gdpm-static \
		--target gdpm-shared \
		--target gdpm-http \
		--target gdpm-restapi
	$NINJA_COMMAND
}

function build_tests(){
	mkdir -p build
	$CMAKE_COMMAND --target gdpm.tests
	$NINJA_COMMAND
}


function build_docs(){
	# Generate documentation using `doxygen`
	pushd ${script_dir}/docs/doxygen
	doxygen
	popd
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
		-a|--all) 	build_all 	shift;;
		--exe) 		build_exe 	shift;;
		--libs) 	build_libs 	shift;;
		--tests) 	build_tests shift;;
		--sums)		checksums	shift;;
		-d|--docs)  build_docs 	shift;;
		-c|--clean) clean 		shift;;
		-s|--strip) strip_all 	shift;;
		-l|--link)  link_all 	shift;;
		--procs=*)	PROCS="${i#*=}" 	shift;;
		-*|--*) echo "Bad option: $1"
	esac
	i=$((i-1))
done
exit 0


