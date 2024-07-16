# Godot Package Manager (GDPM)

[demo.webm](https://github.com/davidallendj/gdpm/assets/16520934/6aa82627-3be6-43e3-b7c7-8a38af20591c)


GDPM is an attempt to make a simple, front-end, command-line, package manager designed to handle assets from the Godot Game Engine's official asset library. It is written in C++ to be lightwight and fast with a few common dependencies found in most Linux distributions and can be used completely independent of Godot. It is designed to add more functionality not included with the official AssetLib with the ability to automate downloads for different platforms. So far, the package manager is capable of searching, downloading, installing, and removing packages and makes managing Godot assets across multiple projects much easier. It stores "packages" in a global directories and is capable of linking or copying global packages to local projects.

\*GDPM has not been tested for Windows or Mac.

-   [Quick Start](#quick-start)
-   [Rational](#rationale)
-   [How It Works](#how-it-works)
-   [Features](#features)
-   [Building from Source](#building-from-source)
	-   [Prerequisites](#prerequisites)
	-   [Macro Definitions](#macro-definitions)
	-   [API Documentation](#api-documentation)
-   [How to use the CLI](#how-to-use-the-cli)
	-   [Installing, Removing, Updating, and Listing](#installing-removing-updating-and-listing)
	-   [Searching](#searching)
	-   [Linking and Cloning](#linking-and-cloning)
	-   [Cleaning Temporary Files](#cleaning-temporary-files)
	-   [Managing Remote Sources](#managing-remote-sources)
	-   [Managing Configuration Properties](#managing-configuration-properties)
-   [Planned Features](#planned-features)
-   [Known Issues](#known-issues)
-   [License](#license)



## Quick Start

Common commands for searching, installing, listing, and removing assets.

```bash
# fetch updates local database if "no packages to install" error
gdpm fetch

# search and install "Godot Jolt" globally
gdpm search jolt
gdpm install "Godot Jolt" --skip-prompt

# list installed packages and remove
gdpm list --style=table
gdpm remove "Godot Jolt" --clean
```

Use `gdpm help` to see full list of commands.

```bash
$ gdpm help
Usage: gdpm [--help] [--verbose VAR] {clean,clone,config,export,fetch,get,help,install,link,list,remote,remove,search,ui,update,version}

Manage Godot engine assets from CLI

Optional arguments:
  -h, --help    shows help message and exits 
  -v, --verbose set verbosity level [nargs=0..1] [default: false]

Subcommands:
  clean         clean package(s) temporary files
  clone         clone package(s) to path
  config        manage config properties
  export        export install package(s) list
  fetch         fetch and sync asset data
  get           add package to project
  help          show help message and exit
  install       install package(s)
  link          link package(s) to path
  list          show install package(s) and remotes
  remote        manage remote(s)
  remove        remove package(s)
  search        search for package(s)
  ui            show user interface (WIP)
  update        update package(s)
  version       show version and exit
```

## Rationale

The AssetLib actually works well enough for most use cases. However, it's fairly common to download the same asset multiple times for different projects. If there isn't a need to modify the asset itself, it makes more sense to download the asset *once and only once*. That way, there will only be one global, read-only copy of the asset that is accessable and it can be linked to more than one project. The AssetLib does not have a built-in way to manage assets like this, and therefore requires redownloading assets for each new project. Additionally, the AssetLib is integrated into the engine itself with no command-line interface to manage assets. When downloading multiple assets, it is much more convenient to be able to automate the downloads using a list from a text file. If the user knows, which assets to download, they can make a list and point to it.

Some assets are more common such as the "First Person Controller" are likely to be reused with little to no modification. Therefore, it made sense to provide some method to download and install the assets once and then provide a symbolic link back to the stored packages. GDPM attempts to fix this by storing the assets in a single location and creating symlinks/shortcuts to each project. By having a separate, simple binary executable, the package manager can be used in shell scripts to download assets more quickly.

Currently, there is no dependency management as it is not needed yet. There are future plans to implement a system to handle dependencies, but the implementation is still being fleshed out.

## How It Works

GDPM is built to work with an instance of [Godot's Asset API](https://github.com/godotengine/godot-asset-library). When installing packages, GDPM will first make HTTP requests to retrieve required asset data to download the asset and sync it in with a local database. Then, it will make another API request to retrieve the rest of an asset's data, update it in the database, then download it from the remote repository. Unfortunately, the package manager requires at least 2 request for a single asset. Therefore, there's an intentional 200 ms delay by default between each request to not bombard the API server, but is configurable at compile time (see `[Building from Source](#building-from-source)` section below).

When removing a package, the unzip files are remove but the temporary files are keep in the specified `.tmp` directory unless using the `--clean` option. This allows the user to remove the uncompressed files, but easily reinstall the package without having to download it remotely again.

The local database stores all the information sent by the Asset API with additional information like "install\_path" and "remote\_source" for the API used to gather information.

## Features

*   Download and install packages from Godot's official AssetLib or from custom repositories. (Note: Multithread download support planned.)

*   Stores downloads in single global location to use across multiple Godot projects using symlinks, thus reducing space and bandwidth.

*   Copy packages into a Godot project to make modifications when needed.

*   Automate installing packages in scripts.

*   Export list of installed packages to reinstall later.

*   Parallel downloads using `libcurl`.

## Building from Source

The project uses the CMake or Meson build system and has been tested with GCC and Clang on Arch/Manjaro Linux. CMake is preferred, but a `meson.build` script is provided and should work with some tweaking. Building on Windows or Mac has not been tested yet so it's not guaranteed to work. Compiling with CMake will build 2 executables, 3 shared libraries, and an archive library to link, while compiling with Meson only builds an executable that links with a shared library.

### Prerequisites

*   Meson or CMake (version 3.12 or later)

*   C++20 compiler (GCC/Clang/MSVC??)

*   libcurl (or curlpp)

*   libzip

*   RapidJson

*   fmt (may be removed later in favor of C++20 std::format)

*   SQLite 3 and SQLite C++ wrapper

*   cxxopts

*   doctest (optional; for tests, but still WIP)

*   Doxygen (optional; to generate API docs)

Arch Linux users can simply install required libs through `pacman` and/or `yay`:

```bash
pacman -S base-devel fmt sqlite rapidjson cmake libzip curl catch2 cxxopts doctest
yay -S sqlitecpp libcurlpp
```

After installing the packages, clone the submodules and link the headers:

```bash
git submodule add https://github.com/p-ranav/argparse modules/argparse
git submodule add https://github.com/p-ranav/tabulate modules/tabulate
git submodule add https://github.com/p-ranav/indicators modules/indicators
git submodule add https://github.com/p-ranav/csv2 modules/csv2

ln -s ../modules/argparse/include/argparse include/argparse
ln -s ../modules/tabulate/include/tabulate include/tabulate
ln -s ../modules/indicators/include/indicators include/indicators
ln -s ../modules/csv2/include/csv2 include/csv2
```

Otherwise, you can just update the modules if they're already there and out-of-date:

```bash
git submodule update --init --recursive
```

And then build the binaries (check the "build" directory):

```bash
# Start by cloning the repo, then...
git clone https://github.com/davidallendj/gdpm
cd gdpm
export project_root=${pwd}

# ...if using Meson with Clang on Linux...
meson build
meson configure build # only needed if reconfiguring build
meson compile -C build -j$(nproc)
CXX=clang++ meson compile -C build -j$(npoc)

# ...if using CMake on Linux (needs work!)...
cd build
cmake .. 
make -j$(nproc)

# Easy build using predefined `compile` script 
${project_root}/bin/compile.sh --all


# ...build using MinGW on Windows ???
# ...build using MSVC on Windows ??? 
# ...build using Clang on MacOS ???
```

Alternatively, run the `compile.sh` script to build with CMake:

```bash
${project_root}/bin/compile.sh \
	--all \			# compiles all executables and libraries
	--exe \			# compiles executables with shared library
	--libs \		# compiles shared and static libraries
	--docs \		# generate the API documentation
	--link \		# create symlinks from executables to the `bin` directory
	--strip \		# strip symbols from binaries (same as running `strip` command)
```

Some macros can be set to enable/disable features or configure defaults by setting the C++ arguments in Meson. These values are used at compile time and a description of each value is provided below.

```cmake
if(MSVC)
	add_compile_defintions(
		# Add flags specific to MSVC here...
	)
else()
	add_compile_definitions(
		-DGDPM_LOG_LEVEL=2
		-DGDPM_REQUEST_DELAY=200ms
		-DGDPM_ENABLE_COLOR=1
		-DGDPM_ENABLE_TIMESTAMP=1
	)
endif()
```

Similarly, this can be done in Meson:

```bash
cpp_args = [
	...
	'-DGDPM_LOG_LEVEL=2',
	'-DGDPM_REQUEST_DELAY=200ms',
	'-DGDPM_ENABLE_COLORS=1',
	'-DGDPM_ENABLE_TIMESTAMPS=1'
	...
]
```

### Macro Definitions

| Macro                   | Values                                                       | Description                                                                                                                                     |
| ----------------------- | ------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| GDPM\_LOG\_LEVEL        | 0-5                                                          | Sets the logging level to show when the program runs. Setting GDPM\_LOG\_LEVEL=0 will completely disable logging information being displayed.   |
| GDPM\_REQUEST\_DELAY    | 200 ms                                                       | Sets the delay time (in ms) between each API request. The delay is meant to reduce the load on the API.                                         |
| GDPM\_ENABLE\_COLOR     | 0 or 1 (true/false)                                          | Enable/disable color output to stdout for errors and debugging. Disable if your system or console does not support ANSI colors. (default: true) |
| GDPM\_ENABLE\_TIMESTAMP | 0 or 1 (true/false)                                          | Enable/disable timestamp output to stdout. (default: true)                                                                                      |
| GDPM\_TIMESTAMP\_FORMAT | strftime formated string (default: ":%I:%M:%S %p; %Y-%m-%d") | Set the time format to use in logging via strftime(). Uses ISO 8601 date and time standard as default.                                          |

### API Documentation

There is support for generating API documentation with `doxygen` for the project. Simply run the command in the root directory of the project or with the `compile.sh` script and documentation will be generated in the `docs/doxygen/` directory. View it by opening the `html/index.html` file in a browser.

NOTE: The API documentation is still a work-in-progress. Over time, the code base will be updated to generate the important parts of the source code.

## How to use the CLI

The `gdpm` command takes a single subcommand argument such as `install`, `remove`, `search`, `remote`, `update` `list`, or `help` followed by other arguments. See the `gdpm help` command for usage and full list of subcommands.

### Installing, Removing, Updating, and Listing

Packages can be installed using the `install` command with a list of package names or by providing a one-package-name-per-line file using the `-f/--file` option. The `-f/--file` option is compatible with the package list using the `export` command.

Installation behavior can be adjusted using other flags like `--sync=disable`, `--cache=disable`, `-y/--skip-prompt`, or `--clean`. Use the `-j/--jobs` flag to download multiple packages in parallel.

```bash
$ gdpm install "Flappy Godot" GodotNetworking -y
$ gdpm install -f packages.txt --config config.json --sync=disable --skip-prompt --verbose
$ gdpm install XDateTime "Line Renderer" "Dev Blocks" Vpainter --jobs 4

$ gdpm export path/to/packages.txt
$ gdpm install -f path/to/packages.txt --sync=disable --skip-prompt
```

If you get a "no packages found to install" error message, try doing a `gdpm fetch` to update the local cache database, then try again.

```bash
$ gdpm install "Godot Jolt"
[ERROR 07:18:41 PM; 2024-03-03] package::install(): no packages found to install.
$ gdpm fetch                                                                               ✔ 
[INFO 07:19:19 PM; 2024-03-03] Sychronizing database...Done.
$ gdpm install "Godot Jolt"                                                                ✔ 
 Title       Author  Category  Version  Godot  Last Modified        Installed? 
 Godot Jolt  mihe    Misc         16     4.2   2024-01-08 11:10:46      ❌    
Do you want to install these packages? (Y/n)
```

If you leave out the `--skip-prompt` flag, hit enter to install by default.

```bash
$ gdpm install XDateTime "Line Renderer" "Dev Blocks" Vpainter --jobs 4
Title          Author    Category  Version  Godot  Last Modified        Installed? 
 XDateTime      XD        Tools        1      4.0   2021-05-31 16:02:25    ✔️   
 Line Renderer  LemiSt24  3D Tools     1      4.0   2022-06-09 16:40:44    ✔️   
 Dev Blocks     Manonox   Misc         1      4.0   2022-08-15 00:27:08    ✔️   
 Vpainter       NX7R      3D Tools     1      4.0   2022-06-25 19:41:56    ✔️   
Do you want to install these packages? (Y/n) n
```

Packages can be removed similiarly to installing.

```bash
$ gdpm remove "Flappy Godot" GodotNetworking -y
$ gdpm remove -f packages.txt --config config.json --sync=disable --skip-prompt
```

Update packages by either specific packages or update everything installed at once. The local asset data with automatically be updated as well. 

```bash
$ gdpm update GodotNetworking
$ gdpm update -f packages.txt
$ gdpm update						# Updates everything installed
$ gdpm fetch						# Updates local asset data
```

Print a list installed packages using the `list` command. This also provides some other extra information like the Godot version and license. You can also list the remote sources using the 'remote' option. Alternatively, a list of remotes can be printed using the `remote` command alone.

```bash
$ gdpm list --style=table
$ gdpm list packages				# Equivalent to `gdpm list`
$ gdpm list remote				# Equivalent to `gdpm remote`
$ gdpm remote list --style=table	# Equivalent to `gdpm list`
```

### Searching

Search for available packages using the `search` command. The results can be tweaked using a variety of options like `--sort` or `--support` as supported parameters by the Asset Library API. See the '--help' command for more details.

Use the `search` command to find a list of available packages. (Note: This currently will only search remotely for package information! This will be changed later to only search the local metadata instead.)
```bash
$ gdpm --verbose search "GodotNetworking" \ 
	--sort updated \
	--type project \
	--max-results 50 \
	--godot-version 4.0 \
	--author godot \
	--support official
```

### Linking and Cloning

Link an installed package using the `link` command or make copy of it using `clone`.

```bash
$ gdpm link GodotNetworking tests/test-godot-project
$ gdpm clone GodotNetworking tests/tests-godot-project/addons

# Future work
# gdpm link --all    # links all installed packages to current directory
# gdpm clone --all   # similar to the command above
```

### Cleaning Temporary Files

Temporary files downloaded from remote repositories can be cleaned by using the `gdpm clean` command or the `--clean` flag after installing or removing packages. Cleaning is recommended if space is limited, but keeping the files stored reduces the number of remote requests needed to synchronize the local asset database with the Asset library.

```bash
$ gdpm install -f packages.txt --clean -y		# install and discard temps
$ gdpm remove -f packages.txt --clean -y		# remove and delete temps
$ gdpm clean "GodotNetworking"				# clean for one package
$ gdpm clean									# clean all packages
```

### Managing Remote Sources

Remote repositories can be added and removed similar to using `git`. Be aware that this API is still being changed and may not always work as expected.

```bash
$ gdpm remote add official https://custom-assetlib/asset-library/api
$ gdpm remote remove official
```


To try `gdpm` without installing it to your system, create a symlink to the built executable and add the `bin` directory to your PATH variable.

```bash
$ ln -s path/to/build/gdpm path/to/bin/gdpm
$ export PATH=$PATH:path/to/bin/gdpm
```

### Managing Configuration Properties

Configuration properties can be viewed and set using the `gdpm config` command.

```bash
$ gdpm config get						# shows all config properties
$ gdpm config get --style=table		# shows all config properties in table
$ gdpm config get skip-prompt jobs	# shows config properties listed
$ gdpm config set timeout 30			# set config property value
$ gdpm config set username towk
```

## Planned Features

- [x] Compatible with Godot 4

- [x] Parallel downloading.

- [X] PKGBUILD for ArchLinux/Manjaro.

- [X] Dockerfile to run in sandbox.

- [ ] Proper updating.

- [ ] Editor plugin (...maybe?...).

- [ ] Complete Asset API including `auth`.

- [ ] Support for paid assets.

- [ ] Adapted to new Asset API.

- [ ] Unit tests.

- [ ] User interface.

- [ ] Experimental dependency management. There is no way of handling dependencies using the Godot Asset API. This is a [hot topic](https://github.com/godotengine/godot-proposals/issues/142) and might change in the near future.
## Known Issues

*   Logging doesn't write to file yet.

*   Download progress bars are not showing when downloading archives. This is being reworked to display multiple bars dynamically to better show download status.

## License

See the `LICENSE.md` file.
