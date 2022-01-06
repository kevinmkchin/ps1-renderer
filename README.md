# untitled project

### Windows
```
git clone https://github.com/kevinmkchin/ps1-renderer.git
cd ps1-renderer
git submodule init
git submodule update
mkdir build
cmake -S . -B build
cmake --build build
```

### Mac
```
brew install SDL2
brew install assimp

git clone https://github.com/kevinmkchin/ps1-renderer.git
cd ps1-renderer
git submodule init
git submodule update
mkdir build
./build_osx.sh
```