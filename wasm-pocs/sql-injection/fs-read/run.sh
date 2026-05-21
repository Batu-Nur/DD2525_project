#!/bin/bash

set -eu

build_wasm ()
{
  docker run --rm -v "$(pwd):/src:z" \
    emscripten/emsdk:3.1.52 \
    emmake make
}

clean ()
{
  docker run --rm -v "$(pwd):/src:z" \
    emscripten/emsdk:3.1.52 \
    emmake make clean
}

run_server() 
{
  # Build server image
  docker build ../../server-template/ -t "wasm-vulns-server"

  docker run --rm -p 3000:3000 \
    --name "wasm-sqli-server" \
    -v .:/app/src/:z \
    wasm-vulns-server
}

case "${1:-""}" in
  "-b")
      build_wasm 
    ;;
  "-c")
      clean
    ;;
  "-d")
      clean
      build_wasm
    ;;
  *)
      clean
      build_wasm
      run_server 
    ;;
esac
