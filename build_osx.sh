# build script for macosx

rm -rf build/ps1.app
cmake -S . -B build
cmake --build build
mkdir build/ps1.app/Contents/Resources
cp -r data/* build/ps1.app/Contents/Resources
