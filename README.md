# DD2525_project
This repository contains code with vulnerabilities in WebAssembly build to test wasmati, a static analysis tool for WebAssembly. 
The folder wasm-pocs comes from: https://anonymous.4open.science/r/wasm-pocs/README.md

## Running the code
The code contains a makefile for each vulnerability using emscripten to compile C code to WebAssembly. To run the code, simply run `make` in the respective folder. This will generate a .wasm file that can be analyzed with wasmati.
However, it may not generate the .js and .html files for your browser. However, you can generate it using emscriten by running the following command in the terminal:
```
emcc -o output.html poc.c
```