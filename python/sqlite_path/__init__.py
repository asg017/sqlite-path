import os
import sqlite3
import platform

class SqlitePathUnsupportedPlatformError(Exception):
  pass

def load(conn: sqlite3.Connection)  -> None:
  conn.load_extension(loadable_path())

def loadable_path() -> str:
    system = platform.system()
    machine = platform.machine()
    if system == 'Darwin':
      suffix = 'dylib'
      if machine == 'x86_64':
        target = 'x86_64-macos'
    
    if target is None or suffix is None:
      raise SqlitePathUnsupportedPlatformError()
    loadable_path = os.path.join(os.path.dirname(__file__), "cross", target, f"path0.{suffix}")
    return os.path.normpath(loadable_path)