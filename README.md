# n64
Nintendo 64 emulation research project

![Build](https://github.com/Dillonb/n64/workflows/Build/badge.svg)

# What is this?
This is a project I am working on to learn about the N64 and more advanced emulation techniques.

# What is this not?
- This is NOT intended to be your "daily driver" N64 emulator for gaming, at least it is currently not planned to be.

# Goals
## Personal development
- Learn about JIT dynamic recompilers
- Learn about 3D graphics
- Challenge myself

## Technical
- Nothing _exclusively_ using high level emulation (HLE.) If anything is HLE'd, it must be an optional setting.
- GUI debugger to view hardware registers/memory
- GDB stub for debugging
- Reasonable amount of automated testing
- Configurable input, keyboard and gamepad support

# Limitations
- Only Linux is currently supported, as I use Linux-specific features. This is planned to be fixed.
- Only little-endian host platforms are planned to be supported.
- Only gcc is supported for now, but this is for sure planned to be fixed. The binary built with gcc will export symbols for the RDP plugin (loaded as a shared library) to call back into, but the binary built with clang will not.
- Only gcc and clang will be supported. I unapologetically use extensions like case ranges and binary literals.
- RDRAM is stored internally as big-endian and translated on every read/write.

# Progress
Currently, the only known commercial game that this will run is Namco Museum, as everything else that I know of needs a more complete RSP.

## UI
Basically nonexistent. Runs a simple SDL window and configured with command-line parameters.

## Debugger
Not started. There is verbose logging with -v (repeatable) that provides some debug info, however.

## CPU
Reasonably complete. Not all instructions are there yet, but enough for now. I'm implementing instructions as I come across them in games, so this will slowly progress as time goes on.

## RSP
Very early. Very few vector instructions are supported, and no known RSP microcode currently works yet. Tests have been adapted from another emulation project, and only a few are passing.

## RDP
Not started. [My fork of Angrylion](https://github.com/Dillonb/angrylion-rdp-plus) can (and must!) be loaded to provide this functionality in the meantime, however.
