# sqlite-path

A loadable SQLite extension for parsing, generating, and querying paths. Based on [cwalk](https://github.com/likle/cwalk)

Try it out in your browser and learn more in [_Introducing sqlite-path: A SQLite extension for parsing and generating file paths_](https://observablehq.com/@asg017/introducing-sqlite-path) (August 2022)

## Usage

```sql
.load ./path0
select path_dirname('foo/bar.txt'); -- 'foo/'
select path_basename('foo/bar.txt'); -- 'bar.txt'
select path_extension('foo/bar.txt'); -- '.txt'

select path_part_at('foo/bar/baz.txt', 0); -- 'foo'
select path_part_at('foo/bar/baz.txt', 1); -- 'bar'
select path_part_at('foo/bar/baz.txt', -1); -- 'baz.txt'
```

Iterate through all parts in a path.

```sql

select *
from path_parts('/usr/bin/sqlite3');

/*
┌────────┬─────────┐
│  type  │  part   │
├────────┼─────────┤
│ normal │ usr     │
│ normal │ bin     │
│ normal │ sqlite3 │
└────────┴─────────┘
*/
```

Inside a ZIP archive of the [SQLite source code](https://github.com/sqlite/sqlite), find the top 5 deepest `.c` source code files under the `ext/` directory (using SQLite's [ZIP support](https://www.sqlite.org/zipfile.html)).

```sql
select
  name,
  path_length(name) as depth
from zipfile('sqlite.archive.master.zip')
where
  -- under the ext/ directory
  path_part_at(name, 1) == 'ext'
  -- ends in ".c"
  and path_extension(name) == '.c'
order by 2 desc
limit 5;

/*
┌────────────────────────────────────────────┬───────┐
│                    name                    │ depth │
├────────────────────────────────────────────┼───────┤
│ sqlite-master/ext/fts3/tool/fts3view.c     │ 5     │
│ sqlite-master/ext/lsm1/lsm-test/lsmtest1.c │ 5     │
│ sqlite-master/ext/lsm1/lsm-test/lsmtest2.c │ 5     │
│ sqlite-master/ext/lsm1/lsm-test/lsmtest3.c │ 5     │
│ sqlite-master/ext/lsm1/lsm-test/lsmtest4.c │ 5     │
└────────────────────────────────────────────┴───────┘
*/
```

Make a histogram of the count of file extensions in the current directory, using [`fsdir()`](https://www.sqlite.org/cli.html#file_i_o_functions).

```sql

select
  path_extension(name),
  count(*),
  printf('%.*c', count(*), '*') as bar
from fsdir('.')
where path_extension(name) is not null
group by 1
order by 2 desc
limit 6;

/*
┌──────────────────────┬──────────┬────────────────────────────────────┐
│ path_extension(name) │ count(*) │                bar                 │
├──────────────────────┼──────────┼────────────────────────────────────┤
│ .md                  │ 34       │ ********************************** │
│ .sample              │ 26       │ **************************         │
│ .c                   │ 21       │ *********************              │
│ .css                 │ 5        │ *****                              │
│ .yml                 │ 4        │ ****                               │
│ .h                   │ 4        │ ****                               │
└──────────────────────┴──────────┴────────────────────────────────────┘
*/
```

## Documentation

See [`docs.md`](./docs.md) for a full API reference.

## Installing

| Language       | Install                                                    |                                                                                                                                                                                           |
| -------------- | ---------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Python         | `pip install sqlite-path`                                   | [![PyPI](https://img.shields.io/pypi/v/sqlite-path.svg?color=blue&logo=python&logoColor=white)](https://pypi.org/project/sqlite-path/)                                                      |
| Datasette      | `datasette install datasette-sqlite-path`                   | [![Datasette](https://img.shields.io/pypi/v/datasette-sqlite-path.svg?color=B6B6D9&label=Datasette+plugin&logoColor=white&logo=python)](https://datasette.io/plugins/datasette-sqlite-path) |
| Node.js        | `npm install sqlite-path`                                   | [![npm](https://img.shields.io/npm/v/sqlite-path.svg?color=green&logo=nodedotjs&logoColor=white)](https://www.npmjs.com/package/sqlite-path)                                                |
| Deno           | [`deno.land/x/sqlite_path`](https://deno.land/x/sqlite_path) | [![deno.land/x release](https://img.shields.io/github/v/release/asg017/sqlite-path?color=fef8d2&include_prereleases&label=deno.land%2Fx&logo=deno)](https://deno.land/x/sqlite_path)        |
| Ruby           | `gem install sqlite-path`                                   | ![Gem](https://img.shields.io/gem/v/sqlite-path?color=red&logo=rubygems&logoColor=white)                                                                                                   |
| Github Release |                                                            | ![GitHub tag (latest SemVer pre-release)](https://img.shields.io/github/v/tag/asg017/sqlite-path?color=lightgrey&include_prereleases&label=Github+release&logo=github)                     |

<!--
| Elixir         | [`hex.pm/packages/sqlite_path`](https://hex.pm/packages/sqlite_path) | [![Hex.pm](https://img.shields.io/hexpm/v/sqlite_path?color=purple&logo=elixir)](https://hex.pm/packages/sqlite_path)                                                                       |
| Go             | `go get -u github.com/asg017/sqlite-path/bindings/go`               | [![Go Reference](https://pkg.go.dev/badge/github.com/asg017/sqlite-path/bindings/go.svg)](https://pkg.go.dev/github.com/asg017/sqlite-path/bindings/go)                                     |
| Rust           | `cargo add sqlite-path`                                             | [![Crates.io](https://img.shields.io/crates/v/sqlite-path?logo=rust)](https://crates.io/crates/sqlite-path)                                                                                 |
-->

The [Releases page](https://github.com/asg017/sqlite-path/releases) contains pre-built binaries for Linux amd64, MacOS amd64 (no arm), and Windows.

### As a loadable extension

If you want to use `sqlite-path` as a [Runtime-loadable extension](https://www.sqlite.org/loadext.html), Download the `path0.dylib` (for MacOS), `path0.so` (Linux), or `path0.dll` (Windows) file from a release and load it into your SQLite environment.

> **Note:**
> The `0` in the filename (`path0.dylib`/ `path0.so`/`path0.dll`) denotes the major version of `sqlite-path`. Currently `sqlite-path` is pre v1, so expect breaking changes in future versions.

For example, if you are using the [SQLite CLI](https://www.sqlite.org/cli.html), you can load the library like so:

```sql
.load ./path0
select path_version();
-- v0.0.1
```

Or in Python, using the builtin [sqlite3 module](https://docs.python.org/3/library/sqlite3.html):

```python
import sqlite3

con = sqlite3.connect(":memory:")

con.enable_load_extension(True)
con.load_extension("./path0")

print(con.execute("select path_version()").fetchone())
# ('v0.0.1',)
```

Or in Node.js using [better-sqlite3](https://github.com/WiseLibs/better-sqlite3):

```javascript
const Database = require("better-sqlite3");
const db = new Database(":memory:");

db.loadExtension("./path0");

console.log(db.prepare("select path_version()").get());
// { 'html_version()': 'v0.0.1' }
```

Or with [Datasette](https://datasette.io/):

```
datasette data.db --load-extension ./path0
```

## See also

- [sqlite-url](https://github.com/asg017/sqlite-url), for parsing and generating URLs (pairs well with this library)
- [sqlite-http](https://github.com/asg017/sqlite-http), for making HTTP requests in SQLite
- [sqlite-html](https://github.com/asg017/sqlite-html), for parsing HTML documents
- [sqlite-lines](https://github.com/asg017/sqlite-lines), for reading large files line-by-line
- [nalgeon/sqlean](https://github.com/nalgeon/sqlean), several pre-compiled handy SQLite functions, in C
