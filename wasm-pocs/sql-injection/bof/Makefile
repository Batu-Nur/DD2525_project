EMCC ?= emcc

CACHE_DIR = ../cache

SQLITE_VERSION = 3490200
SQLITE_YEAR = 2025
SQLITE_ZIP_FILENAME = sqlite-amalgamation-$(SQLITE_VERSION).zip
SQLITE_ZIP = $(CACHE_DIR)/$(SQLITE_ZIP_FILENAME)
SQLITE_URL = https://sqlite.org/$(SQLITE_YEAR)/$(SQLITE_ZIP_FILENAME)
SQLITE_DIR = $(CACHE_DIR)/sqlite-amalgamation-$(SQLITE_VERSION)

SOURCES = wasm.c 

TARGET_JS = wasm.js
TARGET_WASM = wasm.wasm

EMCC_FLAGS = \
    -s MODULARIZE=1 \
    -s EXPORTED_FUNCTIONS=_free,_get_user_secret,_open_database,_initialize_db,_close_db,_sqlite3_open,_sqlite3_errmsg,_malloc \
    -s EXPORTED_RUNTIME_METHODS=stringToNewUTF8,UTF8ToString,stackAlloc,getValue,cwrap \
	-g \
	-fno-inline \
	-sERROR_ON_UNDEFINED_SYMBOLS=0 \
	-sSTANDALONE_WASM=1 \
	-Wl,--no-entry \
	-mno-reference-types \
	-mno-bulk-memory \
	-mno-sign-ext \
	-mno-nontrapping-fptoint \
	

   
all: $(TARGET_JS) $(TARGET_WASM)


$(TARGET_JS): $(SOURCES)
	@echo "Compiling $(TARGET_JS)..."
	$(EMCC) $(SOURCES) $(EMCC_FLAGS) -o $(TARGET_JS)

.PHONY: all clean download_sqlite unzip_sqlite

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET_JS) $(TARGET_WASM) *.o *.wasm.map *.js.mem users.db
	rm -rf $(CACHE_DIR)
