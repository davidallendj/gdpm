# Godot Package Manager (GDPM)

A front-end, package manager designed to handle assets from the Godot Game Engine's official asset library. It is written in C++ to be lightwight and fast with a few common dependencies found in most Linux distributions and can be used completely independent of Godot. It is designed to add more functionality not included with the AssetLib with the ability to automate builds on different systems. So far, the package manager is capable of searching, downloading, installing and removing packages and makes managing Godot assets across multiple projects much easier. GDPM has not been tested on Windows yet and there are no currently plans to support macOS.

## Why not just use the Asset Library?

The AssetLib is a bit limited for my use case. Personally, I wanted a way to quickly download and install all the assets I'd need for a project once and only once. Then, when I'd need that same asset for another project, I could just reuse the ones installed for the previous project. This is where the issues begin. The AssetLib doesn't have a way to reuse asset across multiple projects without having to either redownload the asset again from the AssetLib or copy the assets from somewhere locally. Likewise, this would force game developers to have to copy their assets manually from one project to another and keep track of their assets themselves. 

Some assets are more common such as the "First Person Controller" are likely to be reused with little to no modification. Therefore, it made sense to provide some method to download and install the assets once and then provide a symbolic link back to the stored packages. GDPM attempts to fix this by storing the assets in a single location and creating symlinks/shortcuts to each project. By have a separate, simple binary executable, the package manager can be used in shell scripts to automate tasks and download assets quickly.

Currently, there is no dependency management as it is not needed. There are future plans to implement a system to handle dependencies, but it is not a priority.

## Features

- Download and install packages from Godot's official AssetLib or from custom repositories. (Note: Multithread download support planned.)

- Stores downloads in single location to use across multiple Godot projects.

- Copy packages into a Godot project to make modifications when needed.

- Create package groups to copy a set of packages into a Godot project.

- Login and register for access to repositories. (Note: Planned. This feature is not officially supported by Godot's official AssetLib.)

- Handle dependencies between multiple assets. (Note: Planned. This feature is not supported by Godot's official AssetLib.)

- Support for paid assets. (Note: Planned. This feature is not supported by Godot's official AssetLib repository.)

## Building from source

The project uses the CMake or Meson build system and has been tested with GCC and Clang on Arch/Manjaro Linux. Meson is preferred, but a CMakeLists.txt is provided as well. Building on Windows has not been tested yet so it's not guaranteed to work. Compiling with CMake will build an executable, shared, and archive library. Compiling with Meson only builds an executable and shared library.

### Prerequisites

- Meson or CMake (version 3.12 or later)

- C++20 compiler (GCC/Clang/MSVC??)

- libcurl (or curlpp)

- libzip

- RapidJson

- fmt (may be remove later in favor of C++20 std::format)

- Git (optional for cloning repository)

- Catch2 (optional for tests, but still WIP)

- cxxopts

- SQLite 3

After installing all necessary dependencies, open a terminal and paste the following commands.

```bash
git clone $(repo_name)
cd $(repo_name)

# ...if using Meson on Linux
meson build
meson configure build
meson compile -C build -j$(nproc)

# ... build using Clang++
CXX=clang++ meson compile -C build -j$(npoc)

# ... if using CMake on Linux
cd build
cmake .. 
make -j$(nproc)

# ... build using MinGW on Windows ???
# ... build using MSVC on Windows ??? 
```

## Usage Examples

GDPM takes a single command such as install, remove, search, or list followed by a list of package names as the following argument. The command specified is ran then the program exits reporting an errors that may occur. Each command can be modified by specifying additional optional parameters such as '--file' to specify a file to use or '--max-results' to set the number of max results to return from a search.

```bash
gdpm [COMMAND] [OPTIONS...]
```

To see more help information:

```bash
gdpm help
```

Packages can be installed using the "install" command and providing a list of comma-delimited package names with no spaces or by providing a one-package-per-line file using the '--file' option. Using the '-y' option will bypass the user prompt. The '--no-sync' option with bypass syncing the local package database with the AssetLib API and use locally stored information only. See the '--search' command to find a list of available packages.

```bash
gdpm install "Flappy Godot" "GodotNetworking" -y
gdpm install -f packages.txt --config config.json --no-sync -y
```

Packages can be removed similiarly to installing.

```bash
gdpm remove "Flappy Godot" "GodotNetworking" -y
gdpm remove -f packages.txt
```

Packages can be updated like installing or removing packages. However, if no argument is passed, GDPM will prompt the user to update ALL packages to latest instead.

```bash
gdpm update "GodotNetworking"
gdpm update -f packages.txt
gdpm update    # Updates all packages
```

To list installed packages, use the '--list' option. This also provides some other extra information like the Godot version and license.

```bash
gdpm list
```

Packages can be linked or copied into a project using the 'link' and 'clone' commands.

```bash
gdpm link "GodotNetworking" --path tests/test-godot-project
gdpm link -f packages.txt --path tests/test-godot-project
gdpm clone -f packages.txt --path tests/tests-godot-project/addons
```

Temporary files downloaded from remote repositories can be cleaned by using the clean command or the '--clean' flag after installing  or removing packages. This is recommended if space is limited, but also reduces the number of remote requests needed to rebuild the local package database. 

```bash
gdpm install -f packages.txt --clean -y
gdpm remove -f packages.txt --clean -y
gdpm clean "GodotNetworking"
gdpm clean
```

Planned: Add a custom remote AssetLib repository using [this](https://github.com/godotengine/godot-asset-library) API. You can set the priority for each remote repo with the '--set-priority' option or through the 'config.json' file.

```bash
gdpm add-remote https://custom-assetlib/asset-library/api --set-priority 0
```

Search for available packages from all added repositories. The results can be tweaked using a variety of options like '--sort' or '--support'. See the '--help' command for more details.

```bash
gdpm search "GodotNetworking" \ 
    --sort updated \
    --type project \
    --max-results 100 \
    --godot-version 3.4 \
    --verbose \
    --user towk \
    --support official
```

To see more logging information, set the '--verbose' flag using an integer value between 0-5.

```bash
gdpm list --verbose
```

## License

See the LICENSE.md file.
