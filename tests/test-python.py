import unittest
import sqlite3
import sqlite_path

class TestSqlitepathPython(unittest.TestCase):
  def test_path(self):
    db = sqlite3.connect(':memory:')
    db.enable_load_extension(True)

    self.assertEqual(type(sqlite_path.loadable_path()), str)
    
    sqlite_path.load(db)
    version, = db.execute('select path_version()').fetchone()
    self.assertEqual(version[0], "v")

if __name__ == '__main__':
    unittest.main()