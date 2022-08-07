# Godot Package Manager (GDPM)

GDPM is an attempt to make a front-end, command-line, package manager designed to handle assets from the Godot Game Engine's official asset library. It is written in C++ to be lightwight and fast with a few common dependencies found in most Linux distributions and can be used completely independent of Godot. It is designed to add more functionality not included with the official AssetLib with the ability to automate downloads for different systems. So far, the package manager is capable of searching, downloading, installing, and removing packages and makes managing Godot assets across multiple projects much easier. GDPM has not been tested to work on Windows or Mac.

## Why not just use the Asset Library?

The AssetLib actually works well enough for most use cases. However, it's common to download the same asset multiple times for different projects. If there isn't a need to modify the asset itself, it makes more sense to download the asset only once. That way, there will only be one copy of the asset and it can be linked to all new projects. The AssetLib does not have a way to manage assets like this, and therefore requires redownload assets for each new project. Additionally, the AssetLib is integrated into the engine itself with no command-line interface. When downloading multiple assets, it is much more convenient to be able to automate the downloads using a list from a text file. If the user knows, which assets to download, they can make a list and point to it.

Some assets are more common such as the "First Person Controller" are likely to be reused with little to no modification. Therefore, it made sense to provide some method to download and install the assets once and then provide a symbolic link back to the stored packages. GDPM attempts to fix this by storing the assets in a single location and creating symlinks/shortcuts to each project. By having a separate, simple binary executable, the package manager can be used in shell scripts to download assets more quickly.

Currently, there is no dependency management as it is not needed yet. There are future plans to implement a system to handle dependencies, but the implementation is still being fleshed out.

## How It Works

GDPM is built to work with an instance of [Godot's Asset API](https://github.com/godotengine/godot-asset-library). When installing packages, GDPM will first make HTTP requests to retrieve required asset data to download the asset and sync it in with a local database. Then, it will make another API request to retrieve the rest of an asset's data, update it in the database, then download it from the remote repository. Unfortunately, the package manager requires at least 2 request for a single asset. Therefore, there's an intentional 200 ms delay by default between each request to not bombard the API server, but is configurable at compile time (see "[Building from Source](#building-from-source)" section below).

When removing a package, the unzip files are remove but the temporary files are keep in the specified `.tmp` directory unless using the `--clean` option. This allows the user to remove the uncompressed files, but easily reinstall the package without having to download it remotely again.

The local database stores all the information sent by the Asset API with additional information like "install_path" and "remote_source" for the API used to gather information. 

## Features

- Download and install packages from Godot's official AssetLib or from custom repositories. (Note: Multithread download support planned.)

- Stores downloads in single location to use across multiple Godot projects.

- Copy packages into a Godot project to make modifications when needed.

- Create package groups to copy a set of packages into a Godot project.

## Building from Source

The project uses the CMake or Meson build system and has been tested with GCC and Clang on Arch/Manjaro Linux. Meson is preferred, but a CMakeLists.txt is provided due to CMake's popularity and should work with some tweaking. Building on Windows or Mac has not been tested yet so it's not guaranteed to work. Compiling with CMake will build an executable, shared, and archive library, while compiling with Meson only builds an executable that links with a shared library.

### Prerequisites

- Meson or CMake (version 3.12 or later)

- C++20 compiler (GCC/Clang/MSVC??)

- libcurl (or curlpp)

- libzip

- RapidJson

- fmt (may be removed later in favor of C++20 std::format)

- Catch2 (optional for tests, but still WIP)

- cxxopts (header only)

- SQLite 3

After installing all necessary dependencies, run the following commands:

```bash
# Start by cloning the repo, then...
git clone https://github.com/davidallendj/gdpm
cd https://github.com/davidallendj/gdpm

# ...if using Meson with Clang on Linux...
meson build
meson configure build # only needed if reconfiguring build
meson compile -C build -j$(nproc)
CXX=clang++ meson compile -C build -j$(npoc)

# ...if using CMake on Linux (needs work!)...
cd build
cmake .. 
make -j$(nproc)

# ...build using MinGW on Windows ???
# ...build using MSVC on Windows ??? 
# ...build using Clang on MacOS ???
```

Some macros can be set to enable/disable features or configure defaults by setting the C++ arguments in Meson. These values are used at compile time and a description of each value is provided below.

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

This can be done in CMake in a similar fashion:

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

### Macro Definitions

| Macro                 | Values                                                       | Description                                                                                                                                     |
| --------------------- | ------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| GDPM_LOG_LEVEL        | 0-5                                                          | Sets the logging level to show when the program runs. Setting GDPM_LOG_LEVEL=0 will completely disable logging information being displayed.     |
| GDPM_REQUEST_DELAY    | 200 ms                                                       | Sets the delay time (in ms) between each API request. The delay is meant to reduce the load on the API.                                         |
| GDPM_ENABLE_COLOR     | 0 or 1 (true/false)                                          | Enable/disable color output to stdout for errors and debugging. Disable if your system or console does not support ANSI colors. (default: true) |
| GDPM_ENABLE_TIMESTAMP | 0 or 1 (true/false)                                          | Enable/disable timestamp output to stdout. (default: true)                                                                                      |
| GDPM_TIMESTAMP_FORMAT | strftime formated string (default: ":%I:%M:%S %p; %Y-%m-%d") | Set the time format to use in logging via strftime(). Uses ISO 8601 date and time standard as default.                                          |

## Usage Examples

GDPM takes a single command such as install, remove, search, or list followed by a list of package names as the following argument. The command specified is ran then the program exits reporting an errors that may occur. Each command can be modified by specifying additional optional parameters such as '--file' to specify a file to use or '--max-results' to set the number of max results to return from a search.

```bash
gdpm [COMMAND] [OPTIONS...]
```

To see more help information:

```bash
gdpm help
```

Packages can be installed using the `install` command and providing a list of comma-delimited package names with no spaces or by providing a one-package-per-line file using the `--file` option. Using the `-y` option will bypass the user prompt. The `--no-sync` option with bypass syncing the local package database with the AssetLib API and use locally stored information only. See the `search` command to find a list of available packages.

```bash
gdpm install "Flappy Godot" "GodotNetworking" -y
gdpm install -f packages.txt --config config.json --no-sync --no-prompt
```

Packages can be removed similiarly to installing.

```bash
gdpm remove "Flappy Godot" "GodotNetworking" -y
gdpm remove -f packages.txt --config config.json --no-sync --no-prompt
```

Packages can be updated like installing or removing packages. However, if no argument is passed, GDPM will prompt the user to update ALL packages to latest instead.

```bash
gdpm update "GodotNetworking"
gdpm update -f packages.txt
gdpm update    # Updates all packages
```

To list installed packages, use the 'list' command. This also provides some other extra information like the Godot version and license. You can also list the remote sources using the 'remote-sources' option.

```bash
gdpm list
gdpm list packages
gdpm list remote-sources
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
gdpm delete-remote https://custom-assetlib/asset-library/api
```

Search for available packages from all added repositories. The results can be tweaked using a variety of options like '--sort' or '--support'. See the '--help' command for more details.

```bash
gdpm search "GodotNetworking" \ 
    --sort updated \
    --type project \
    --max-results 50 \
    --godot-version 3.4 \
    --verbose \
    --user towk \
    --support official
```

To see more logging information, set the '--verbose' flag using an integer value between 0-5.

```bash
gdpm list packages --verbose
gdpm list remote-sources
```

If you want to try `gdpm` without installing it to your system, make a symlink to the built executable and add the `bin` directory to your PATH variable.

```bash
ln -s path/to/build/gdpm path/to/bin/gdpm
export PATH=$PATH:path/to/bin/gdpm
```

## Planned Features

- [ ] Dependency management. There is no way of handling dependencies using the Godot Asset API. This is a [hot topic](https://github.com/godotengine/godot-proposals/issues/142) and might change in the near future.

- [ ] Proper updating mechanism. 

- [ ] Plugin integration into the editor.

- [ ] Complete integration with the Asset API including moderation tools.

- [ ] Login and register for access to repositories.

- [ ] Support for paid assets. 

## Known Issues

- The `help` command does not should the command/options correctly. Commands do not use the double hypen, `--command` format. Commands should be use like `gdpm command --option` instead.

## License

See the LICENSE.md file.
