# Godot Package Manager (GDPM)

![](https://ody.sh/B5vPxVhNTr)

GDPM is an attempt to make a simple, front-end, command-line, package manager designed to handle assets from the Godot Game Engine's official asset library. It is written in C++ to be lightwight and fast with a few common dependencies found in most Linux distributions and can be used completely independent of Godot. It is designed to add more functionality not included with the official AssetLib with the ability to automate downloads for different platforms. So far, the package manager is capable of searching, downloading, installing, and removing packages and makes managing Godot assets across multiple projects much easier. It stores "packages" in a global directories and is capable of linking or copying global packages to local projects.

\*GDPM has not been tested for Windows or Mac.

## Why not just use the Asset Library?

The AssetLib actually works well enough for most use cases. However, it's fairly common to download the same asset multiple times for different projects. If there isn't a need to modify the asset itself, it makes more sense to download the asset *once and only once*. That way, there will only be one global, read-only copy of the asset that is accessable and it can be linked to more than one project. The AssetLib does not have a built-in way to manage assets like this, and therefore requires redownloading assets for each new project. Additionally, the AssetLib is integrated into the engine itself with no command-line interface to manage assets. When downloading multiple assets, it is much more convenient to be able to automate the downloads using a list from a text file. If the user knows, which assets to download, they can make a list and point to it.

Some assets are more common such as the "First Person Controller" are likely to be reused with little to no modification. Therefore, it made sense to provide some method to download and install the assets once and then provide a symbolic link back to the stored packages. GDPM attempts to fix this by storing the assets in a single location and creating symlinks/shortcuts to each project. By having a separate, simple binary executable, the package manager can be used in shell scripts to download assets more quickly.

Currently, there is no dependency management as it is not needed yet. There are future plans to implement a system to handle dependencies, but the implementation is still being fleshed out.

## How It Works

GDPM is built to work with an instance of [Godot's Asset API](https://github.com/godotengine/godot-asset-library). When installing packages, GDPM will first make HTTP requests to retrieve required asset data to download the asset and sync it in with a local database. Then, it will make another API request to retrieve the rest of an asset's data, update it in the database, then download it from the remote repository. Unfortunately, the package manager requires at least 2 request for a single asset. Therefore, there's an intentional 200 ms delay by default between each request to not bombard the API server, but is configurable at compile time (see "[Building from Source](#building-from-source)" section below).

When removing a package, the unzip files are remove but the temporary files are keep in the specified `.tmp` directory unless using the `--clean` option. This allows the user to remove the uncompressed files, but easily reinstall the package without having to download it remotely again.

The local database stores all the information sent by the Asset API with additional information like "install\_path" and "remote\_source" for the API used to gather information.

## Features

*   Download and install packages from Godot's official AssetLib or from custom repositories. (Note: Multithread download support planned.)

*   Stores downloads in single global location to use across multiple Godot projects using symlinks.

*   Copy packages into a Godot project to make modifications when needed.

*   Automate installing packages in scripts.

## Building from Source

The project uses the CMake or Meson build system and has been tested with GCC and Clang on Arch/Manjaro Linux. CMake is preferred, but a `meson.build` script is provided and should work with some tweaking. Building on Windows or Mac has not been tested yet so it's not guaranteed to work. Compiling with CMake will build 2 executables, 3 shared libraries, and an archive library to link, while compiling with Meson only builds an executable that links with a shared library.

### Prerequisites

*   Meson or CMake (version 3.12 or later)

*   C++20 compiler (GCC/Clang/MSVC??)

*   libcurl (or curlpp)

*   libzip

*   RapidJson

*   fmt (may be removed later in favor of C++20 std::format)

*   doctest (optional; for tests, but still WIP)

*   clipp (header only)

*   SQLite 3

*   Doxygen (optional; to generate API docs)

After installing all necessary dependencies, build the project with the following commands:

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
${project_root}/bin/compile.sh

# ...build using MinGW on Windows ???
# ...build using MSVC on Windows ??? 
# ...build using Clang on MacOS ???
```

Alternatively, run the `compile.sh` script to build with CMake:

```bash
${project_root}/bin/compile.sh \
    --all \        # compiles all executables and libraries
    --docs \       # generate the API documentation
    --link \       # create symlinks from executables to the `bin` directory
    --strip \      # strip symbols from binaries (same as running `strip` command)
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

Similarly, this can be done in CMake:

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

| Macro                   | Values                                                       | Description                                                                                                                                     |
| ----------------------- | ------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| GDPM\_LOG\_LEVEL        | 0-5                                                          | Sets the logging level to show when the program runs. Setting GDPM\_LOG\_LEVEL=0 will completely disable logging information being displayed.   |
| GDPM\_REQUEST\_DELAY    | 200 ms                                                       | Sets the delay time (in ms) between each API request. The delay is meant to reduce the load on the API.                                         |
| GDPM\_ENABLE\_COLOR     | 0 or 1 (true/false)                                          | Enable/disable color output to stdout for errors and debugging. Disable if your system or console does not support ANSI colors. (default: true) |
| GDPM\_ENABLE\_TIMESTAMP | 0 or 1 (true/false)                                          | Enable/disable timestamp output to stdout. (default: true)                                                                                      |
| GDPM\_TIMESTAMP\_FORMAT | strftime formated string (default: ":%I:%M:%S %p; %Y-%m-%d") | Set the time format to use in logging via strftime(). Uses ISO 8601 date and time standard as default.                                          |

### API Documentation

There is now support for generating API documentation with `doxygen` for the project. Simply run the command in the root directory of the project or with the `compile.sh` script and documentation will be generated in the `docs/doxygen/` directory. View it by opening the `html/index.html` file in a browser.

NOTE: The API documentation is still a work-in-progress. Over time, the code base will be updated to generate the important parts of the source code.

## How to Use

GDPM takes a single command such as `install`, `remove`, `search`, `remote`, `update` `list`, or `help` followed by a list of arguments. The command specified is ran then the program exits reporting any errors that may occur. Each command can be modified by specifying additional optional parameters such as `--file` to read in a file for input or `--max-results` to limit the number of returned results from a search.&#x20;

```bash
gdpm [COMMAND] [OPTIONS...]
```

To see more help information:

```bash
gdpm help
```

Packages can be installed using the `install` command with a list of package names or by providing a one-package-name-per-line file using the `--file` option. Additionally, a package list can be exported to reinstall packages using the `export` command.&#x20;

Other installation behavior can be changed with additional flags. Adding the `-y` flag will skip the user prompt. Adding `--no-sync` option with bypass syncing the local package database with the AssetLib API and use locally stored metadata only. Adding the `--verbose` flag will some more output. Use the `search` command to find a list of available packages. (Note: This currently will only search remotely for package information! This will be changed later to only search the local metadata instead.)

```bash
gdpm install "Flappy Godot" "GodotNetworking" -y
gdpm install -f packages.txt --config config.json --no-sync --no-prompt --verbose

gdpm export path/to/packages.txt
gdpm install -f path/to/packages.txt --no-sync --no-prompt

# Future work for multithreading support...
# gdpm install -f path/to/packages.txt -j$(nproc) --no-sync --no-prompt --verbose
```

Packages can be removed similiarly to installing.

```bash
gdpm remove "Flappy Godot" "GodotNetworking" -y
gdpm remove -f packages.txt --config config.json --no-sync --no-prompt
```

Packages can be updated simliar installing or removing packages. However, if no argument is passed, GDPM will prompt the user to update ALL packages to latest instead. The local metadata database can be updated using the `sync` command. (Note: The `sync` command will be changed to the `fetch` command to reflect `git`'s API.)

```bash
gdpm update "GodotNetworking"
gdpm update -f packages.txt
gdpm update    # Updates all packages
gdpm sync      # Updates metadata
```

To print a list installed packages, use the 'list' command. This also provides some other extra information like the Godot version and license. You can also list the remote sources using the 'remote' option. Alternatively, a list of remotes can be printed using the `remote` command alone.

```bash
gdpm list
gdpm list packages      # Equivalent to `gdpm list`
gdpm list remote        # Equivalent to `gdpm remote`
gdpm remote            
```

Packages can be linked or copied into a project using the 'link' and 'clone' commands. Specify which packages to link or clone then a path to link/clone it to. (Note: Listing no path will be changed to link/clone packages to the current directory.)

```bash
gdpm link "GodotNetworking" --path tests/test-godot-project
gdpm link -f packages.txt --path tests/test-godot-project
gdpm clone -f packages.txt --path tests/tests-godot-project/addons

# Future work
# gdpm link --all    # links all installed packages to current directory
# gdpm clone --all   # similar to the command above
```

Temporary files downloaded from remote repositories can be cleaned by using the clean command or the `--clean` flag after installing  or removing packages. Cleaning is recommended if space is limited, but keeping the files stored reduces the number of remote requests needed to rebuild the local metadata database.

Specifying packages will only clean those from temporary storage. Otherwise, no arguments will clean all temporary storage.

```bash
gdpm install -f packages.txt --clean -y
gdpm remove -f packages.txt --clean -y
gdpm clean "GodotNetworking"
gdpm clean
```

Remote repositories can be added and removed similar to using `git`. Be aware that this API is still being changed and may not always work as expected.

```bash
gdpm remote add official https://custom-assetlib/asset-library/api
gdpm remote remove https://custom-assetlib/asset-library/api
```

Search for available packages using the `search` command. The results can be tweaked using a variety of options like '--sort' or '--support' as supported parameters by the Asset Library API. See the '--help' command for more details.

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

To try `gdpm` without installing it to your system, create a symlink to the built executable and add the `bin` directory to your PATH variable.

```bash
ln -s path/to/build/gdpm path/to/bin/gdpm
export PATH=$PATH:path/to/bin/gdpm
```

## Planned Features

*   [ ] Multithreaded support.

*   [ ] Dependency management. There is no way of handling dependencies using the Godot Asset API. This is a [hot topic](https://github.com/godotengine/godot-proposals/issues/142) and might change in the near future.

*   [ ] Proper updating mechanism.

*   [ ] Plugin integration into the editor.

*   [ ] Complete integration with the Asset API including moderation tools.

*   [ ] Login and register for access to repositories.

*   [ ] Support for paid assets.

## Known Issues

*   The `help` command does currently print the command/options correctly. Commands do not use the double hypen, `--command` format. Commands should be used like `gdpm command --option` instead.

## License

See the LICENSE.md file.
