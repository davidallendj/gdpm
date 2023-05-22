#!/usr/bin/env bash

# Call this from project root
grep -v ^S include/*.hpp src/*.cpp | wc -l include/*.hpp src/*.cpp 
