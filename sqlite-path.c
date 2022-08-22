#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

#include <ctype.h>
#include <cwalk.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma region sqlite - path meta scalar functions

/** path_version()
 * Returns the semver version string of the current version of sqlite-path.
 */
static void pathVersionFunc(sqlite3_context *context, int argc,
                            sqlite3_value **argv) {
  sqlite3_result_text(context, SQLITE_PATH_VERSION, -1, SQLITE_STATIC);
}

/** path_debug()
 * Returns a debug string of various info about sqlite-path, including
 * the version string, build date, commit hash, and cwalk version.
 */
static void pathDebugFunc(sqlite3_context *context, int argc,
                          sqlite3_value **arg) {
  const char *debug =
      sqlite3_mprintf("Version: %s\nDate: %s\nSource: %s\ncwalk version: %s",
                      SQLITE_PATH_VERSION, SQLITE_PATH_DATE, SQLITE_PATH_SOURCE,
                      SQLITE_PATH_CWALK_VERSION);
  if (debug == NULL) {
    sqlite3_result_error_nomem(context);
    return;
  }
  sqlite3_result_text(context, debug, -1, SQLITE_TRANSIENT);
  sqlite3_free((void *)debug);
}

#pragma endregion

#pragma region sqlite - path scalar functions

/** path_absolute(path)
 * Returns 1 if the given path is absolute, 0 otherwise.
 *
 */
static void pathAbsoluteFunc(sqlite3_context *context, int argc,
                             sqlite3_value **argv) {
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_int(context, 0);
    return;
  }
  sqlite3_result_int(
      context, cwk_path_is_absolute((const char *)sqlite3_value_text(argv[0])));
}
/** path_basename(path)
 * Returns the basename of the given path as text,
 * or NULL if it cannot be calculated.
 */
static void pathBasenameFunc(sqlite3_context *context, int argc,
                             sqlite3_value **argv) {
  const char *basename;
  size_t length;
  const char *path;
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }
  path = (const char *)sqlite3_value_text(argv[0]);
  cwk_path_get_basename(path, &basename, &length);
  sqlite3_result_text(context, basename, length, SQLITE_TRANSIENT);
}

/** path_dirname(path)
 * Returns the dirname of the given path as text,
 * or NULL if it cannot be calculated.
 */
static void pathDirnameFunc(sqlite3_context *context, int argc,
                            sqlite3_value **argv) {
  size_t length = 0;
  const char *path;

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }
  path = (const char *)sqlite3_value_text(argv[0]);
  cwk_path_get_dirname(path, &length);
  if (length == 0) {
    sqlite3_result_null(context);
    return;
  }
  sqlite3_result_text(context, path, length, SQLITE_TRANSIENT);
}

/** path_extension(path)
 * Returns the extension of the given path as text,
 * or NULL if it cannot be calculated.
 */
static void pathExtensionFunc(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {
  const char *extension;
  size_t length = 0;
  const char *path;
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }
  path = (const char *)sqlite3_value_text(argv[0]);
  cwk_path_get_extension(path, &extension, &length);
  if (length == 0) {
    sqlite3_result_null(context);
    return;
  }
  sqlite3_result_text(context, extension, length, SQLITE_TRANSIENT);
}

/** path_intersection(path)
 * Returns the common portions between two paths, or null if it cannot be
 * computed.
 *
 */
static void pathIntersectionFunc(sqlite3_context *context, int argc,
                                 sqlite3_value **argv) {
  char result[FILENAME_MAX];

  const char *base;
  const char *other;
  size_t length;
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL ||
      sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }

  base = (const char *)sqlite3_value_text(argv[0]);
  other = (const char *)sqlite3_value_text(argv[1]);
  length = cwk_path_get_intersection(base, other);

  if (length == 0) {
    sqlite3_result_null(context);
    return;
  }

  sqlite3_result_text(context, base, length, SQLITE_TRANSIENT);
}

/** path_join(path1, path2, [...pathN])
 * Join two or more paths together, or null if it cannot be computed.
 */
static void pathJoinFunc(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {
  char buffer[FILENAME_MAX];
  char *ptr;
  size_t size;

  if (argc < 2) {
    sqlite3_result_error(context, "at least 2 paths are required for path_join",
                         -1);
    return;
  }
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }

  size = 0;
  ptr = (char *)sqlite3_value_text(argv[0]);
  for (int i = 1; i < argc; i++) {
    size_t n;
    const char *b = (const char *)sqlite3_value_text(argv[i]);
    n = cwk_path_join(ptr, b, buffer, sizeof(buffer));
    size = n;
    ptr = buffer;
    buffer[size] = 0;
  }
  sqlite3_result_text(context, buffer, size, SQLITE_TRANSIENT);
}

/** path_normalize(path)
 * Create a normalized version of the given path (resolving back segments),
 * or null if it cannot be computed.
 */
static void pathNormalizeFunc(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {
  char result[FILENAME_MAX];
  const char *path;
  int length;
  size_t size;

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }
  path = (const char *)sqlite3_value_text(argv[0]);
  size = cwk_path_normalize(path, result, sizeof(result));

  sqlite3_result_text(context, result, size, SQLITE_TRANSIENT);
}

// TODO path_name(path), "a.txt" -> "a", "d.tar.gz" -> "d" etc.

/** path_relative(path)
 * Returns 1 if the given path is relative, 0 if not, or null if path is null.

 */
static void pathRelativeFunc(sqlite3_context *context, int argc,
                             sqlite3_value **argv) {
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }

  sqlite3_result_int(
      context, cwk_path_is_relative((const char *)sqlite3_value_text(argv[0])));
}

/** path_root(path)
 * Returns the root portion of the given path, or null if it cannot be computed.
 *
 */
static void pathRootFunc(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {
  const char *path = (const char *)sqlite3_value_text(argv[0]);
  size_t length;
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }
  cwk_path_get_root(path, &length);
  sqlite3_result_text(context, path, length, SQLITE_TRANSIENT);
}

/** path_segment_at(path, at)
 * Returns the path segment in the given path at the specified index.
 * If 'at' is positive, then '0' is the first segment and counts to the end.
 * If 'at' is negative, then '-1' is the last segment and continue to the
 * beginning. If 'at' "overflows" in either direction, then returns NULL.
 *
 */
static void pathSegmentAtFunc(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {
  const char *path = (const char *)sqlite3_value_text(argv[0]);
  int at = sqlite3_value_int(argv[1]);
  struct cwk_segment segment;

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }

  // TODO both: if first/last returns false
  if (at >= 0) {
    int overflow = 0;
    cwk_path_get_first_segment(path, &segment);
    for (int i = 0; i < at; i++) {
      if (!cwk_path_get_next_segment(&segment)) {
        overflow = 1;
        break;
      }
    }
    if (overflow) {
      sqlite3_result_null(context);
    } else {
      sqlite3_result_text(context, segment.begin, segment.size,
                          SQLITE_TRANSIENT);
    }
    return;
  }
  int overflow = 0;
  cwk_path_get_last_segment(path, &segment);
  for (int i = -1; i > at; i--) {
    if (!cwk_path_get_previous_segment(&segment)) {
      overflow = 1;
      break;
    }
  }
  if (overflow) {
    sqlite3_result_null(context);
  } else {
    sqlite3_result_text(context, segment.begin, segment.size, SQLITE_TRANSIENT);
  }
}
#pragma endregion

#pragma region sqlite - path table functions
/** select * from path_segments(path)
 * Table function that returns each segment for the given path.
 * Return a table with the following schema:
 * ```sql
 * create table path_segments(
 *  type text,        --
 *  segment text,     --x
 *  path text hidden  -- input path
 * )
 * ```
 *
 */

#define PATH_SEGMENTS_COLUMN_ROWID -1
#define PATH_SEGMENTS_COLUMN_PATH 0
#define PATH_SEGMENTS_COLUMN_TYPE 1
#define PATH_SEGMENTS_COLUMN_SEGMENT 2

typedef struct path_segments_cursor path_segments_cursor;
struct path_segments_cursor {
  // Base class - must be first
  sqlite3_vtab_cursor base;
  sqlite3_int64 iRowid;
  // pointer to current segment
  struct cwk_segment *segment;
  // whether or not there's a next segment to yield
  bool next;
};

/*
** The pathSegmentsReadConnect() method is invoked to create a new
** pathSegments_vtab that describes the pathSegments_read virtual table.
**
** Think of this routine as the constructor for pathSegments_vtab objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the pathSegments_vtab object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against pathSegments_read will look like.
*/
static int pathSegmentsConnect(sqlite3 *db, void *pUnused, int argcUnused,
                               const char *const *argvUnused,
                               sqlite3_vtab **ppVtab, char **pzErrUnused) {
  sqlite3_vtab *pNew;
  int rc;
  (void)pUnused;
  (void)argcUnused;
  (void)argvUnused;
  (void)pzErrUnused;
  rc = sqlite3_declare_vtab(db, "CREATE TABLE x(path hidden, type, segment)");
  if (rc == SQLITE_OK) {
    pNew = *ppVtab = sqlite3_malloc(sizeof(*pNew));
    if (pNew == 0)
      return SQLITE_NOMEM;
    memset(pNew, 0, sizeof(*pNew));
    sqlite3_vtab_config(db, SQLITE_VTAB_INNOCUOUS);
  }
  return rc;
}

/*
** This method is the destructor for path_segments_cursor objects.
*/
static int pathSegmentsDisconnect(sqlite3_vtab *pVtab) {
  sqlite3_free(pVtab);
  return SQLITE_OK;
}

/*
** Constructor for a new path_segments_cursor object.
*/
static int pathSegmentsOpen(sqlite3_vtab *pUnused,
                            sqlite3_vtab_cursor **ppCursor) {
  path_segments_cursor *pCur;
  (void)pUnused;
  pCur = sqlite3_malloc(sizeof(*pCur));
  if (pCur == 0)
    return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  *ppCursor = &pCur->base;
  return SQLITE_OK;
}

/*
** Destructor for a path_segments_cursor.
*/
static int pathSegmentsClose(sqlite3_vtab_cursor *cur) {
  sqlite3_free(cur);
  return SQLITE_OK;
}

/*
** Advance a path_segments_cursor to its next row of output.
*/
static int pathSegmentsNext(sqlite3_vtab_cursor *cur) {
  path_segments_cursor *pCur = (path_segments_cursor *)cur;
  pCur->next = cwk_path_get_next_segment(pCur->segment);
  pCur->iRowid++;
  return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int pathSegmentsEof(sqlite3_vtab_cursor *cur) {
  path_segments_cursor *pCur = (path_segments_cursor *)cur;
  return !pCur->next;
}

/*
** Return values of columns for the row at which the path_segments_cursor
** is currently pointing.
*/
static int pathSegmentsColumn(
    sqlite3_vtab_cursor *cur, /* The cursor */
    sqlite3_context *ctx,     /* First argument to sqlite3_result_...() */
    int i                     /* Which column to return */
) {
  path_segments_cursor *pCur = (path_segments_cursor *)cur;
  sqlite3_int64 x = 0;
  switch (i) {
  case PATH_SEGMENTS_COLUMN_PATH: {
    sqlite3_result_null(ctx);
    break;
  }
  case PATH_SEGMENTS_COLUMN_TYPE: {
    switch (cwk_path_get_segment_type(pCur->segment)) {
    case CWK_NORMAL:
      sqlite3_result_text(ctx, "normal", -1, SQLITE_STATIC);
      break;
    case CWK_CURRENT:
      sqlite3_result_text(ctx, "current", -1, SQLITE_STATIC);
      break;
    case CWK_BACK:
      sqlite3_result_text(ctx, "back", -1, SQLITE_STATIC);
      break;
    }
    break;
  }
  case PATH_SEGMENTS_COLUMN_SEGMENT: {
    sqlite3_result_text(ctx, pCur->segment->begin, pCur->segment->size,
                        SQLITE_TRANSIENT);
    break;
  }
  }
  return SQLITE_OK;
}

/*
** Return the rowid for the current row. In this implementation, the
** first row returned is assigned rowid value 1, and each subsequent
** row a value 1 more than that of the previous.
*/
static int pathSegmentsRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {
  path_segments_cursor *pCur = (path_segments_cursor *)cur;
  *pRowid = pCur->iRowid;
  return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the pathSegments_read virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
*/
/*
  Every query plan for pathSegments() or pathSegments_read() will use idxNum and
  idxStr.

  idxNum options:
    LINES_IDXNUM_FULL: "do a full scan", ie read all pathSegments from
  file/document LINES_IDXNUM_ROWID_EQ: Only read a single line, defined by a
  "rowid = :x" constraint

  idxStr is a 3-character string that denotes which argv option cooresponds
  to which column constraint. The i-th character in the string cooresponds
  to the i-th argv option in the xFilter functions.

  idxStr character options:
    LINES_IDXSTR_PATH: argv[i] is text to the path of the file or the document
  itself LINES_IDXSTR_DELIMITER: argv[i] will be text of delimiter to use
    LINES_IDXSTR_ROWID: argv[i] is integer of rowid to filter to, with
  LINES_IDXNUM_ROWID_EQ

*/

static int pathSegmentsBestIndex(sqlite3_vtab *pVTab,
                                 sqlite3_index_info *pIdxInfo) {
  int hasPath = 0;

  for (int i = 0; i < pIdxInfo->nConstraint; i++) {
    const struct sqlite3_index_constraint *pCons = &pIdxInfo->aConstraint[i];
    switch (pCons->iColumn) {
    case PATH_SEGMENTS_COLUMN_PATH: {
      if (!hasPath && !pCons->usable || pCons->op != SQLITE_INDEX_CONSTRAINT_EQ)
        return SQLITE_CONSTRAINT;
      hasPath = 1;
      pIdxInfo->aConstraintUsage[i].argvIndex = 1;
      pIdxInfo->aConstraintUsage[i].omit = 1;
      break;
    }
    }
  }
  if (!hasPath) {
    pVTab->zErrMsg = sqlite3_mprintf("path argument is required");
    return SQLITE_ERROR;
  }
  pIdxInfo->idxNum = 1;
  pIdxInfo->estimatedCost = (double)100000;
  pIdxInfo->estimatedRows = 100000;

  return SQLITE_OK;
}

/*
** This method is called to "rewind" the path_segments_cursor object back
** to the first row of output.  This method is always called at least
** once prior to any call to xColumn() or xRowid() or xEof().
**
** This routine should initialize the cursor and position it so that it
** is pointing at the first row, or pointing off the end of the table
** (so that xEof() will return true) if the table is empty.
*/
static int pathSegmentsFilter(sqlite3_vtab_cursor *pVtabCursor, int idxNum,
                              const char *idxStr, int argc,
                              sqlite3_value **argv) {
  path_segments_cursor *pCur = (path_segments_cursor *)pVtabCursor;
  struct cwk_segment *segment;
  pCur->segment = sqlite3_malloc(sizeof(*segment));
  if (pCur->segment == NULL)
    return SQLITE_NOMEM;
  memset(pCur->segment, 0, sizeof(*pCur->segment));

  pCur->next = cwk_path_get_first_segment(
      (const char *)sqlite3_value_text(argv[0]), pCur->segment);
  return SQLITE_OK;
}

static sqlite3_module pathSegmentsModule = {
    0,                      /* iVersion */
    0,                      /* xCreate */
    pathSegmentsConnect,    /* xConnect */
    pathSegmentsBestIndex,  /* xBestIndex */
    pathSegmentsDisconnect, /* xDisconnect */
    0,                      /* xDestroy */
    pathSegmentsOpen,       /* xOpen - open a cursor */
    pathSegmentsClose,      /* xClose - close a cursor */
    pathSegmentsFilter,     /* xFilter - configure scan constraints */
    pathSegmentsNext,       /* xNext - advance a cursor */
    pathSegmentsEof,        /* xEof - check for end of scan */
    pathSegmentsColumn,     /* xColumn - read data */
    pathSegmentsRowid,      /* xRowid - read data */
    0,                      /* xUpdate */
    0,                      /* xBegin */
    0,                      /* xSync */
    0,                      /* xCommit */
    0,                      /* xRollback */
    0,                      /* xFindMethod */
    0,                      /* xRename */
    0,                      /* xSavepoint */
    0,                      /* xRelease */
    0,                      /* xRollbackTo */
    0                       /* xShadowName */
};

#pragma endregion

#pragma region sqlite - path entrypoints

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_path_init(sqlite3 *db, char **pzErrMsg,
                          const sqlite3_api_routines *pApi) {
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);

  // Just unix for now - maybe should be configurable?
  cwk_path_set_style(CWK_STYLE_UNIX);

  (void)pzErrMsg; /* Unused parameter */
  if (rc == SQLITE_OK)
    rc = sqlite3_create_function(db, "path_version", 0,
                                 SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                     SQLITE_DETERMINISTIC,
                                 0, pathVersionFunc, 0, 0);
  if (rc == SQLITE_OK)
    rc = sqlite3_create_function(db, "path_debug", 0,
                                 SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                     SQLITE_DETERMINISTIC,
                                 0, pathDebugFunc, 0, 0);
  if (rc == SQLITE_OK)
    rc = sqlite3_create_function(db, "path_join", -1,
                                 SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                     SQLITE_DETERMINISTIC,
                                 0, pathJoinFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_dirname", 1,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathDirnameFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_basename", 1,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathBasenameFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_extension", 1,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathExtensionFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_segment_at", 2,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathSegmentAtFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_absolute", 1,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathAbsoluteFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_relative", 1,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathRelativeFunc, 0, 0);
  rc = sqlite3_create_function(
      db, "path_root", 1, SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC,
      0, pathRootFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_normalize", 1,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathNormalizeFunc, 0, 0);
  rc = sqlite3_create_function(db, "path_intersection", 2,
                               SQLITE_UTF8 | SQLITE_INNOCUOUS |
                                   SQLITE_DETERMINISTIC,
                               0, pathIntersectionFunc, 0, 0);

  if (rc == SQLITE_OK)
    rc = sqlite3_create_module(db, "path_segments", &pathSegmentsModule, 0);
  return rc;
}

#pragma endregion