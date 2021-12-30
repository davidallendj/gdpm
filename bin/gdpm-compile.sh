# Run this script at project root
meson configure build
CXX=clang++ meson compile -C build -j$(proc)