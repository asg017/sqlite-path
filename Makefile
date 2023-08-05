COMMIT=$(shell git rev-parse HEAD)
VERSION=$(shell cat VERSION)
DATE=$(shell date +'%FT%TZ%z')

CWALK_VERSION=$(shell jq '.version' cwalk/clib.json)

LOADABLE_CFLAGS=-fPIC -shared

ifeq ($(shell uname -s),Darwin)
CONFIG_DARWIN=y
else ifeq ($(OS),Windows_NT)
CONFIG_WINDOWS=y
else
CONFIG_LINUX=y
endif

ifdef CONFIG_DARWIN
LOADABLE_EXTENSION=dylib
endif

ifdef CONFIG_LINUX
LOADABLE_EXTENSION=so
endif

ifdef CONFIG_WINDOWS
LOADABLE_EXTENSION=dll
endif

ifdef python
PYTHON=$(python)
else
PYTHON=python3
endif

ifdef IS_MACOS_ARM
RENAME_WHEELS_ARGS=--is-macos-arm
else
RENAME_WHEELS_ARGS=
endif

DEFINE_SQLITE_PATH_DATE=-DSQLITE_PATH_DATE="\"$(DATE)\""
DEFINE_SQLITE_PATH_VERSION=-DSQLITE_PATH_VERSION="\"v$(VERSION)\""
DEFINE_SQLITE_PATH_SOURCE=-DSQLITE_PATH_SOURCE="\"$(COMMIT)\""
DEFINE_SQLITE_PATH_CWALK_VERSION=-DSQLITE_PATH_CWALK_VERSION="\"$(CWALK_VERSION)\""
DEFINE_SQLITE_PATH=$(DEFINE_SQLITE_PATH_DATE) $(DEFINE_SQLITE_PATH_VERSION) $(DEFINE_SQLITE_PATH_SOURCE) $(DEFINE_SQLITE_PATH_CWALK_VERSION)

prefix=dist

TARGET_LOADABLE=$(prefix)/path0.$(LOADABLE_EXTENSION)
TARGET_WHEELS=$(prefix)/wheels
TARGET_SQLITE3_EXTRA_C=$(prefix)/sqlite3-extra.c
TARGET_SQLITE3=$(prefix)/sqlite3
TARGET_SQLJS_JS=$(prefix)/sqljs.js
TARGET_SQLJS_WASM=$(prefix)/sqljs.wasm
TARGET_SQLJS=$(TARGET_SQLJS_JS) $(TARGET_SQLJS_WASM)

INTERMEDIATE_PYPACKAGE_EXTENSION=python/sqlite_path/sqlite_path/path0.$(LOADABLE_EXTENSION)

$(prefix):
	mkdir -p $(prefix)

$(TARGET_WHEELS): $(prefix)
	mkdir -p $(TARGET_WHEELS)

clean:
	rm dist/*

FORMAT_FILES=sqlite-path.h sqlite-path.c core_init.c
format: $(FORMAT_FILES)
	clang-format -i $(FORMAT_FILES)

loadable: $(TARGET_LOADABLE) $(TARGET_LOADABLE_NOFS)
sqlite3: $(TARGET_SQLITE3)
sqljs: $(TARGET_SQLJS)

$(TARGET_LOADABLE): sqlite-path.c $(prefix)
	gcc -Isqlite -Icwalk/include \
	$(LOADABLE_CFLAGS) $(CFLAGS) \
	$(DEFINE_SQLITE_PATH) \
	$< -o $@ cwalk/src/cwalk.c

python: $(TARGET_WHEELS) $(TARGET_LOADABLE) $(TARGET_WHEELS) scripts/rename-wheels.py $(shell find python/sqlite_path -type f -name '*.py')
	cp $(TARGET_LOADABLE) $(INTERMEDIATE_PYPACKAGE_EXTENSION)
	rm $(TARGET_WHEELS)/sqlite_path* || true
	pip3 wheel python/sqlite_path/ -w $(TARGET_WHEELS)
	python3 scripts/rename-wheels.py $(TARGET_WHEELS) $(RENAME_WHEELS_ARGS)
	echo "✅ generated python wheel"

python-versions: python/version.py.tmpl
	VERSION=$(VERSION) envsubst < python/version.py.tmpl > python/sqlite_path/sqlite_path/version.py
	echo "✅ generated python/sqlite_path/sqlite_path/version.py"

	VERSION=$(VERSION) envsubst < python/version.py.tmpl > python/datasette_sqlite_path/datasette_sqlite_path/version.py
	echo "✅ generated python/datasette_sqlite_path/datasette_sqlite_path/version.py"


datasette: $(TARGET_WHEELS) $(shell find python/datasette_sqlite_path -type f -name '*.py')
	rm $(TARGET_WHEELS)/datasette* || true
	pip3 wheel python/datasette_sqlite_path/ --no-deps -w $(TARGET_WHEELS)

bindings/sqlite-utils/pyproject.toml: bindings/sqlite-utils/pyproject.toml.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@
	echo "✅ generated $@"

bindings/sqlite-utils/sqlite_utils_sqlite_path/version.py: bindings/sqlite-utils/sqlite_utils_sqlite_path/version.py.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@
	echo "✅ generated $@"

sqlite-utils: $(TARGET_WHEELS) bindings/sqlite-utils/pyproject.toml bindings/sqlite-utils/sqlite_utils_sqlite_path/version.py
	python3 -m build bindings/sqlite-utils -w -o $(TARGET_WHEELS)

npm: VERSION npm/platform-package.README.md.tmpl npm/platform-package.package.json.tmpl npm/sqlite-path/package.json.tmpl scripts/npm_generate_platform_packages.sh
	scripts/npm_generate_platform_packages.sh

deno: VERSION deno/deno.json.tmpl
	scripts/deno_generate_package.sh

bindings/ruby/lib/version.rb: bindings/ruby/lib/version.rb.tmpl VERSION
	VERSION=$(VERSION) envsubst < $< > $@

ruby: bindings/ruby/lib/version.rb

version:
	make python-versions
	make python
	make bindings/sqlite-utils/pyproject.toml bindings/sqlite-utils/sqlite_utils_sqlite_path/version.py
	make npm
	make deno
	make ruby

$(TARGET_SQLITE3): $(prefix) $(TARGET_SQLITE3_EXTRA_C) sqlite/shell.c sqlite-path.c
	gcc \
	$(DEFINE_SQLITE_PATH) \
	-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION=1 \
	-DSQLITE_EXTRA_INIT=core_init \
	-I./ -I./sqlite -Icwalk/include \
	$(TARGET_SQLITE3_EXTRA_C) sqlite/shell.c sqlite-path.c cwalk/src/cwalk.c \
	-o $@

$(TARGET_SQLITE3_EXTRA_C): sqlite/sqlite3.c core_init.c
	cat sqlite/sqlite3.c core_init.c > $@

test:
	make test-format
	make test-loadable
	make test-python
	make test-npm
	make test-deno
	make test-sqlite3

test-format: SHELL:=/bin/bash
test-format:
	diff -u <(cat $(FORMAT_FILES)) <(clang-format $(FORMAT_FILES))

test-loadable:
	$(PYTHON) tests/test-loadable.py

test-python:
	$(PYTHON) tests/test-python.py

test-npm:
	node npm/sqlite-path/test.js

test-deno:
	deno task --config deno/deno.json test

test-loadable-watch: $(TARGET_LOADABLE)
	watchexec -w sqlite-path.c -w $(TARGET_LOADABLE) -w tests/test-loadable.py --clear -- make test-loadable

test-sqlite3: $(TARGET_SQLITE3)
	python3 tests/test-sqlite3.py

test-sqlite3-watch: $(TARAGET_SQLITE3)
	watchexec -w $(TARAGET_SQLITE3) -w tests/test-sqlite3.py --clear -- make test-sqlite3

test-sqljs: $(TARGET_SQLJS)
	python3 -m http.server & open http://localhost:8000/tests/test-sqljs.html

publish-release:
	./scripts/publish_release.sh

.PHONY: all clean format publish-release \
	version python python-versions datasette sqlite-utils npm deno ruby \
	test test-watch test-format \
	loadable test-loadable test-loadable-watch

# The below is mostly borrowed from https://github.com/sql-js/sql.js/blob/master/Makefile

# WASM has no (easy) filesystem for the demo, so disable lines_read
SQLJS_CFLAGS = \
	-O2 \
	-DSQLITE_OMIT_LOAD_EXTENSION \
	-DSQLITE_DISABLE_LFS \
	-DSQLITE_ENABLE_JSON1 \
	-DSQLITE_THREADSAFE=0 \
	-DSQLITE_ENABLE_NORMALIZE \
	$(DEFINE_SQLITE_PATH) -DSQLITE_LINES_DISABLE_FILESYSTEM \
	-DSQLITE_EXTRA_INIT=core_init

SQLJS_EMFLAGS = \
	--memory-init-file 0 \
	-s RESERVED_FUNCTION_POINTERS=64 \
	-s ALLOW_TABLE_GROWTH=1 \
	-s EXPORTED_FUNCTIONS=@wasm/exported_functions.json \
	-s EXPORTED_RUNTIME_METHODS=@wasm/exported_runtime_methods.json \
	-s SINGLE_FILE=0 \
	-s NODEJS_CATCH_EXIT=0 \
	-s NODEJS_CATCH_REJECTION=0 \
	-s LLD_REPORT_UNDEFINED

SQLJS_EMFLAGS_WASM = \
	-s WASM=1 \
	-s ALLOW_MEMORY_GROWTH=1

SQLJS_EMFLAGS_OPTIMIZED= \
	-s INLINING_LIMIT=50 \
	-O3 \
	-flto \
	--closure 1

SQLJS_EMFLAGS_DEBUG = \
	-s INLINING_LIMIT=10 \
	-s ASSERTIONS=1 \
	-O1

$(TARGET_SQLJS): $(prefix) $(shell find wasm/ -type f) sqlite-path.c $(TARGET_SQLITE3_EXTRA_C)
	emcc $(SQLJS_CFLAGS) $(SQLJS_EMFLAGS) $(SQLJS_EMFLAGS_DEBUG) $(SQLJS_EMFLAGS_WASM) \
		-I./sqlite -I./ -I./cwalk/include sqlite-path.c cwalk/src/cwalk.c $(TARGET_SQLITE3_EXTRA_C) \
		--pre-js wasm/api.js \
		-o $(TARGET_SQLJS_JS)
	mv $(TARGET_SQLJS_JS) tmp.js
	cat wasm/shell-pre.js tmp.js wasm/shell-post.js > $(TARGET_SQLJS_JS)
	rm tmp.js
