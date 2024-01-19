# DinoTech

The DinoTech library is a c++ poker math library for evaluating player's equities based off of range of hands.
Currently it supports:

- Evaluating Texas Holdem and Omaha hand equities including both single and double boards
- Evaluating Texas Holdem equities using hand ranges as inputs
- Uses presolved preflop equities for instant solutions
- A Javascript glue layer allowing compilation to web assembly and utilization within a browser
- A robust hand declaration syntax

The majority of the library is written in c++, but a wrapper is also included that allows you to compile and use
the functionality of the library from javascript.

# Dependencies:

- Install Emscripten. Environment variables EMSDK, etc are required
- CMake
  NOTE: If compiling on windows with visual studio, just use the 64-bit visual studio developer command prompt which has CMake built in.

# Building

All cmake and ctest commands should be run from the cpp directory

To build a native static library and test suite:

cmake --preset x64-debug
cmake --build --preset x64-debug

To build a wasm libary that can be used from javascript:
cmake --preset wasm-release
cmake --build --preset wasm-release

# Tests

You can run the native tests using ctest
ctest --preset wasm-release

You can run the javascript tests to test the javascript library bindings and wasm library using npm.
Run from the js folder
npm test

# Create Package

After building the wasm-release configuration, you can create a distributable package:
npm run package

The package will be placed in out/dist

# License

Code is licensed under AGPL-3.0-only. Please inquire for alternative license.
