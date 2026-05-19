# wasmati/tests/c/makefile
EMCC=emcc
WASM_FILES=$(patsubst %.c,%.wasm,$(wildcard *.c))

CFLAGS=-O0 \
  -g \
  -fno-inline \
  -sERROR_ON_UNDEFINED_SYMBOLS=0 \
  -sSTANDALONE_WASM=1 \
  -Wl,--no-entry \
  -mno-reference-types \
  -mno-bulk-memory \
  -mno-sign-ext \
  -mno-nontrapping-fptoint \
  -Wl,--export=write_to_web

.PHONY: all clean

all: $(WASM_FILES)

%.wasm: %.c
	$(EMCC) $< -o $@ $(CFLAGS)  

clean:
	rm -f $(WASM_FILES)
