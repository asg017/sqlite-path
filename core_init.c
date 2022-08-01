/*
  This file is appended to the end of a sqlite3.c amalgammation
  file to include sqlite3_lines functions/tables statically in
  a build. This is used for the demo CLI and WASM implementations.
*/
#include "sqlite-path.h"
int core_init(const char *dummy) {
  return sqlite3_auto_extension((void *)sqlite3_path_init);
}