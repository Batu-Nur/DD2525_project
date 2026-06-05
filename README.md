# DD2525_project
This repository contains code with vulnerabilities in WebAssembly build to test wasmati, a static analysis tool for WebAssembly. 
The folder wasm-pocs comes from: https://anonymous.4open.science/r/wasm-pocs/README.md

## Running the code
The code contains a makefile for each vulnerability using emscripten to compile C code to WebAssembly. To run the code, simply run `make` in the respective folder. This will generate a .wasm file that can be analyzed with wasmati.
However, this does not generate the .js files for your browser. However, you can generate it using emscriten by running the following command in the terminal:
```
emcc <pocname.c> -o poc.js -s "EXPORTED_FUNCTIONS=['_write_to_web']" -s EXPORTED_RUNTIME_METHODS=ccall,cwrap
```
Then you can run it on browser by opening the html file after hosting the files on a local python server 

```
python3 -m http.server 
```

## Analysis with Wasmati
- First, clone and compile wasmati. Follow the instructions on the wasmati repository: [Repository](https://github.com/halo34/wasmati)
- Then, run the following command to analyze the generated .wasm file:
```
python3 DD2525_project/evaluation/evauluate.py \
  --directory "path/to/wasm/files" \
  --recursive \
  --max-size-kb 25 \
  --workers 3 \
  --output-jsonl my_results_max25kb.jsonl

```
To analyse the PoCs, you can run the following command:
```
python3 DD2525_project/evaluation/evauluate.py \
  --directory "path/to/wasm-pocs" \
  --recursive \
  --max-size-kb 25 \
  --workers 3 \
  --output-jsonl poc.jsonl
  
- To analyze the results, you can open the `analyse.ipynb` notebook. Please be aware to install first the required dependencies, e.g., using pip:
```
pip install pandas matplotlib seaborn
```

### Pregenerated results
The results of the analysis of the PoCs are available in `poc.jsonl`. The results of the analysis of WasmBench is available in `my_results_max25kb.jsonl` without language extensions and with language in `my_results_max25kb_with_language.jsonl`. 