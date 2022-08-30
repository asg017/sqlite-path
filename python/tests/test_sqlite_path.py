from sqlite_path import loadable_path
import sqlite3
def load(conn):
    pass
def test_example_function():
    conn = sqlite3.connect(":memory:")
    conn.enable_load_extension(True)
    conn.load_extension(loadable_path())
    assert loadable_path() == '/Users/alex/projects/sqlite-path/python/sqlite_path/cross/x86_64-macos/path0.dylib'
    assert conn.execute("select path_version()").fetchone()[0] == 'v0.1.0'
