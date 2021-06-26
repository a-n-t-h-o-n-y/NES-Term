# NES Term [+..••]

Terminal frontend for [SimpleNES](https://github.com/amhndu/SimpleNES) Emulator.

<p align="center">
  <img src="docs/mario.png">
</p>

<p align="center">
  <img src="docs/zelda.png">
</p>

## Build

Relies on cmake to generate build files, `nes-term` target is created. two git
submodules are used, make sure to run `git submodule update --init` after
cloning.

## Running

Can be run by specifying the rom file as the first argument when launching from
the terminal.

Root privileges are required for proper keyboard input in the terminal, key
release events cannot be accessed without opening the console as a file.

From the build directory:

`sudo ./src/nes-term ~/location/of/ROM/file`

The release build seems to run fine at 60fps; debug will be slow 🐌

## Controls

- Arrows keys
- A Button: z
- B Button: x
- Start:    Enter
- Select:   Backspace
