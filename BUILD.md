### Build Dependencies

 - [**wut**](https://github.com/devkitPro/wut/)
 - **devkitPPC**
 - **ppc-zlib** - available from devkitPro pacman (`pacman -S ppc-zlib`)

We suggest using Docker to compile.

## Docker 
```sh
docker build . -t martini
docker run -it --rm -v ${PWD}:/app martini
```
For Docker on Windows Replace ${PWD} with %CD%

## CMake
It is recommended to use a Linux OS for compiling (WSL does work)

First Export
```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
```

Then CMake
```sh
mkdir build && cd build && cmake .. && make
```
When now recompiling just run `make` (in the build folder)


## Troubleshoot

Missing SubModules:
```sh
git submodule update --init
```