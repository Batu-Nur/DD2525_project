#!/bin/bash

set -eu

build_all () 
{
  docker run --rm -v "$(pwd):/src:z" \
    emscripten/emsdk:3.1.52 \
    emmake make
}

build_wasm ()
{
  docker run --rm -v "$(pwd):/src:z" \
    emscripten/emsdk:3.1.52 \
    emmake make wasm.js
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
    --name "wasm-ssti-server" \
    --memory="4096M" \
    -v .:/app/src/:z \
    wasm-vulns-server
}

case "${1:-""}" in
  "-b")
      build_all
    ;;
  "-c")
      clean
    ;;
  "-d")
      clean
      build_all
    ;;
  *)
      build_wasm
      run_server 
    ;;
esac
