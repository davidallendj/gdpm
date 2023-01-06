#!/bin/bash
command=build/gdpm

# Install packages using install command and specifying each package name or file
${command} install "ResolutionManagerPlugin" "godot-hmac"
${command} install -f packages.txt --clean --godot-version 3.4

# Remove packages using remove command and specifying each package name or file
${command} remove "ResolutionManagerPlugin" "godot-hmac" -y
${command} remove -f packages.txt -y

# Manually synchronize the local database with remote servers
${command} sync

# Search for packages containing "Godot" in package name
${command} search "Godot" --config config.json
${command} search "Godot" --no-sync --godot-version 3.4 --max-results 20 --sort updated

# List all currently installed packages
${command} list
${command} list packages
${command} list remote-sources

${command} add-remote https://godotengine.org/asset-library/api
${command} delete-remote https://godotengine.org/asset-library/api

# Create a symlink of packages to specified path
${command} link "ResolutionManagerPlugin" "godot-hmac" --path tests/tests-godot-project
${command} link -f packages.txt --path tests/test-godot-project

# Similarly, make a copy of packages to a project
${command} clone "ResolutionManagerPlugin" "godot-hmac" --path tests/tests/godot-project
${command} clone -f packages.txt --path tests/tests-godot-project/addons

# Clean temporary downloaded files if they're taking up too much space. If no package names are provided, all temporary files will be removed.
${command} clean "ResolutionManagerPlugin"
${command} clean