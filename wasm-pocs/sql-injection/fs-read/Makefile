EMCC ?= emcc

CACHE_DIR = cache

SQLITE_VERSION = 3490200
SQLITE_YEAR = 2025
SQLITE_ZIP_FILENAME = sqlite-amalgamation-$(SQLITE_VERSION).zip
SQLITE_ZIP = $(CACHE_DIR)/$(SQLITE_ZIP_FILENAME)
SQLITE_URL = https://sqlite.org/$(SQLITE_YEAR)/$(SQLITE_ZIP_FILENAME)
SQLITE_DIR = $(CACHE_DIR)/sqlite-amalgamation-$(SQLITE_VERSION)

SOURCES = wasm.c $(SQLITE_DIR)/sqlite3.c

TARGET_JS = wasm.js
TARGET_WASM = wasm.wasm

EMCC_FLAGS = \
    -s MODULARIZE=1 \
    -s EXPORTED_FUNCTIONS=_free,_is_token_valid,_get_user_secret,_open_database,_close_db,_sqlite3_open,_sqlite3_errmsg,_malloc,_sqlite3_exec \
    -s EXPORTED_RUNTIME_METHODS=NODEFS,FS,cwrap,ccall,getValue,UTF8ToString,stringToNewUTF8 \
		-s FORCE_FILESYSTEM=1 \
		-lnodefs.js \
    -I$(SQLITE_DIR)

all: $(SQLITE_DIR) $(TARGET_JS) $(TARGET_WASM)

$(CACHE_DIR):
	@mkdir -p $(CACHE_DIR)

$(SQLITE_ZIP): $(CACHE_DIR)
	@echo "Downloading SQLite amalgamation to $(CACHE_DIR)..."
	curl -o $@ $(SQLITE_URL) || wget -O $@ $(SQLITE_URL)

$(SQLITE_DIR): $(SQLITE_ZIP)
	@echo "Unzipping SQLite amalgamation to $(CACHE_DIR)..."
			unzip -q -o $< -d $(CACHE_DIR)

$(TARGET_JS): $(SOURCES)
	@echo "Compiling $(TARGET_JS)..."
	$(EMCC) $(SOURCES) $(EMCC_FLAGS) -o $(TARGET_JS)

.PHONY: all clean download_sqlite unzip_sqlite

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET_JS) $(TARGET_WASM) *.o *.wasm.map *.js.mem users.db compile_commands.json
	rm -rf $(CACHE_DIR)
