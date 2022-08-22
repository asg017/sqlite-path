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

TARGET_LOADABLE=dist/path0.$(LOADABLE_EXTENSION)

DEFINE_SQLITE_PATH_DATE=-DSQLITE_PATH_DATE="\"$(DATE)\""
DEFINE_SQLITE_PATH_VERSION=-DSQLITE_PATH_VERSION="\"$(VERSION)\""
DEFINE_SQLITE_PATH_SOURCE=-DSQLITE_PATH_SOURCE="\"$(COMMIT)\""
DEFINE_SQLITE_PATH_CWALK_VERSION=-DSQLITE_PATH_CWALK_VERSION="\"$(CWALK_VERSION)\""
DEFINE_SQLITE_PATH=$(DEFINE_SQLITE_PATH_DATE) $(DEFINE_SQLITE_PATH_VERSION) $(DEFINE_SQLITE_PATH_SOURCE) $(DEFINE_SQLITE_PATH_CWALK_VERSION)

prefix=dist
TARGET_SQLITE3_EXTRA_C=$(prefix)/sqlite3-extra.c
TARGET_SQLITE3=$(prefix)/sqlite3

$(prefix):
	mkdir -p $(prefix)

clean:
	rm dist/*

FORMAT_FILES=sqlite-path.h sqlite-path.c core_init.c
format: $(FORMAT_FILES)
	clang-format -i $(FORMAT_FILES)

loadable: $(TARGET_LOADABLE) $(TARGET_LOADABLE_NOFS)
sqlite3: $(TARGET_SQLITE3)

$(TARGET_LOADABLE): sqlite-path.c
	gcc -Isqlite -Icwalk/include \
	$(LOADABLE_CFLAGS) \
	$(DEFINE_SQLITE_PATH) \
	$< -o $@ cwalk/src/cwalk.c

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
	make test-sqlite3

test-format: SHELL:=/bin/bash
test-format:
	diff -u <(cat $(FORMAT_FILES)) <(clang-format $(FORMAT_FILES))

test-loadable: $(TARGET_LOADABLE)
	python3 tests/test-loadable.py

test-loadable-watch: $(TARGET_LOADABLE)
	watchexec -w sqlite-path.c -w $(TARGET_LOADABLE) -w tests/test-loadable.py --clear -- make test-loadable

test-sqlite3: $(TARGET_SQLITE3)
	python3 tests/test-sqlite3.py

test-sqlite3-watch: $(TARAGET_SQLITE3)
	watchexec -w $(TARAGET_SQLITE3) -w tests/test-sqlite3.py --clear -- make test-sqlite3

.PHONY: all clean format \
	test test-watch test-format \
	loadable test-loadable test-loadable-watch
