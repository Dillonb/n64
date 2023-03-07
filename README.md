# dgb-n64
Experimental low-level N64 emulator written in C and a bit of C++.

Still under heavy development and not ready for prime time. Compatibility is not high and performance is not great (yet.)

The goals of this project are to create a low-level emulator with good compatibility, while learning a lot along the way.

![Build](https://github.com/Dillonb/n64/workflows/Build/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/n64/badge/?version=latest)](https://n64.readthedocs.io/?badge=latest)

![Mario Face](https://media.githubusercontent.com/media/Dillonb/n64/master/screenshots/mario.png)

# Download

As the emulator is still in heavy development, there are no stable releases. The files linked below are built automatically every time code is pushed to this repository.

There may be problems. Feel free to [open an issue](https://github.com/Dillonb/n64/issues/new) if you find one!

You'll need a fast CPU and a modern GPU that supports Vulkan 1.1.

- [Latest Linux version](https://nightly.link/Dillonb/n64/workflows/build/master/n64-linux.zip)
- [Latest Windows version](https://nightly.link/Dillonb/n64/workflows/build/master/n64-windows.zip)

# Links
- [TODO List](https://github.com/Dillonb/n64/projects/1)
- [Compatibility List](https://github.com/users/Dillonb/projects/1/views/1)
- [My Test Suite](https://github.com/dillonb/n64-tests)
- [My Documentation](https://n64.readthedocs.io/)

# Goals
- Reasonably accurate low-level emulation
- Decent performance. Because this is a low-level emulator, it will never be as fast as high-level emulators.
- Reasonable amount of automated testing
  
# Features
- Keyboard and gamepad support
- GDB stub for debugging

# Limitations & TODOs
- Only Linux and Windows are currently supported. MacOS support is not currently possible, since parallel-rdp does not support it.
- The dynamic recompiler currently only supports x86_64. aarch64 support is planned in the (distant) future.
- Only little-endian host platforms are planned to be supported.
- Only gcc and clang are supported. I currently use extensions that MSVC does not support, but this is planned to be fixed.

# Building
For Linux:

1. Install dependencies: SDL2, Vulkan, dbus, and optionally Capstone
2. Run the following commands:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```
For Windows:

1. Install dependencies: Visual Studio 2019 with the clang workload, vcpkg, CMake.
2. Run the following commands, replacing the vcpkg path with where you installed it:
```bash
vcpkg install sdl2[vulkan]:x64-windows
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -T clangcl -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ..
cmake --build . --config Release
```

# Running
Run with no parameters to use the default settings and load your ROM using the GUI, or run with options:

```
./n64 [OPTION]... [FILE]
n64, a dgb n64 emulator

  -v, --verbose             enables verbose output, repeat up to 4 times for more verbosity
  -h, --help                Display this help message
  -i, --interpreter         Force the use of the interpreter
  -m, --movie               Load movie (Mupen64Plus .m64 format)
  -p, --pif                 Load PIF ROM

https://github.com/Dillonb/n64
```

# Progress

## CPU
An interpreter and a basic dynamic recompiler are available, able to be switched at launch time with a command line flag.

The dynamic recompiler currently only supports x86_64.

## RSP
Reasonably complete, enough for most games to run.

Hardware-verified over the EverDrive 64's USB port using [rsp-recorder](https://github.com/dillonb/rsp-recorder).

Still greatly in need of optimization.

## RDP
Very early stage. [parallel-rdp by Themaister](https://github.com/Themaister/parallel-rdp) is integrated to provide RDP functionality in the meantime.

# Credits
- [Stephen Lane-Walsh](https://github.com/whobrokethebuild) for [cflags](https://github.com/whobrokethebuild/cflags), [gdbstub](https://github.com/WhoBrokeTheBuild/gdbstub), and moral support
- [@wheremyfoodat](https://github.com/wheremyfoodat) for his help with x86 assembly and moral support
- [Giovanni Bajo](https://github.com/rasky/) for his automated RSP tests
- [Peter Lemon](https://github.com/peterlemon/) for his tests, and permission to use his ASM code and font as a template for my own tests

# Libraries Used
- [DynASM](https://luajit.org/dynasm.html) as the emitter for the dynamic recompiler
- [SDL2](https://www.libsdl.org/) for graphics, audio, and input
- [Capstone](http://www.capstone-engine.org/) as a MIPS disassembler for debugging
- [parallel-rdp](https://github.com/Themaister/parallel-rdp) as the RDP until I write my own
- [Dear Imgui](https://github.com/ocornut/imgui) for the GUI
