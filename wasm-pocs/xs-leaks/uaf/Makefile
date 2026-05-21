EMCC       ?= emcc
CACHE_DIR  := cache
PCRE_REPO  := https://github.com/PCRE2Project/pcre2.git
PCRE_DIR   := $(CACHE_DIR)/pcre2
PREFIX     := $(abspath $(CACHE_DIR))
PCRE_BUILD := $(PCRE_DIR)/build
PCRE_LIB   := $(PCRE_BUILD)/libpcre2-8.a

SOURCES    := wasm.c
TARGET_JS  := wasm.js

EMCC_FLAGS := \
  -s MODULARIZE=1 \
  -s EXPORTED_FUNCTIONS=_add_secret,_get_secret,_add_user,_malloc,_free \
  -s EXPORTED_RUNTIME_METHODS=cwrap,ccall,getValue,UTF8ToString,stringToNewUTF8 \
  -I$(PCRE_BUILD) \
  -L$(PCRE_BUILD) -lpcre2-8

all: $(PCRE_LIB) $(TARGET_JS)

$(PCRE_DIR):
	@mkdir -p $(CACHE_DIR)
	git clone --depth 1 --branch pcre2-10.45 \
	  $(PCRE_REPO) $@

$(PCRE_LIB): $(PCRE_DIR)
	@echo "Building PCRE2 library..."
	@mkdir -p $(PCRE_BUILD)
	cd $(PCRE_DIR) && \
		emmake cmake -G "Unix Makefiles" -B build -DCMAKE_BUILD_TYPE=Release
	cd $(PCRE_DIR) && \
		emmake cmake --build build/

$(TARGET_JS): $(SOURCES) $(PCRE_LIB)
	@echo "Compiling WASM module..."
	$(EMCC) $(SOURCES) $(EMCC_FLAGS) -o $(TARGET_JS)

.PHONY: all clean

clean:
	rm -rf $(CACHE_DIR) $(TARGET_JS) wasm.wasm
