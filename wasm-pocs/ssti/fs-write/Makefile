EMCC       ?= emcc

SOURCES    := wasm.c
TARGET_JS  := wasm.js

EMCC_FLAGS := \
  -s MODULARIZE=1 \
  -s EXPORTED_FUNCTIONS=_get_script,_generate_nonce,_malloc,_free \
  -s EXPORTED_RUNTIME_METHODS=cwrap,ccall,getValue,UTF8ToString,stringToNewUTF8

all: $(TARGET_JS)

$(TARGET_JS): $(SOURCES) $(PCRE_LIB)
	@echo "⟳ Compiling WebAssembly module…"
	$(EMCC) $(SOURCES) $(EMCC_FLAGS) -o $(TARGET_JS)

.PHONY: all clean

clean:
	rm -rf $(CACHE_DIR) $(TARGET_JS) wasm.wasm
