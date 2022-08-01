import sqlite3
import unittest

EXT_PATH="./dist/path0"

def connect(ext):
  db = sqlite3.connect(":memory:")

  db.execute("create table base_functions as select name from pragma_function_list")
  db.execute("create table base_modules as select name from pragma_module_list")

  db.enable_load_extension(True)
  db.load_extension(ext)

  db.execute("create temp table loaded_functions as select name from pragma_function_list where name not in (select name from base_functions) order by name")
  db.execute("create temp table loaded_modules as select name from pragma_module_list where name not in (select name from base_modules) order by name")

  db.row_factory = sqlite3.Row
  return db


db = connect(EXT_PATH)

def explain_query_plan(sql):
  return db.execute("explain query plan " + sql).fetchone()["detail"]

def execute_all(sql, args=None):
  if args is None: args = []
  results = db.execute(sql, args).fetchall()
  return list(map(lambda x: dict(x), results))

def spread_args(args):
  return ",".join(['?'] * len(args))

FUNCTIONS = [
  "path_absolute",
  "path_basename",
  "path_debug",
  "path_dirname",
  "path_extension",
  "path_intersection",
  "path_join",
  "path_normalize",
  "path_relative",
  "path_root",
  "path_segment_at",
  "path_version",
]

MODULES = [
  "path_segments",
]
class TestPath(unittest.TestCase):
  def test_funcs(self):
    funcs = list(map(lambda a: a[0], db.execute("select name from loaded_functions").fetchall()))
    self.assertEqual(funcs, FUNCTIONS)

  def test_modules(self):
    modules = list(map(lambda a: a[0], db.execute("select name from loaded_modules").fetchall()))
    self.assertEqual(modules, MODULES)

    
  def test_path_version(self):
    with open("./VERSION") as f:
      version = f.read()
    
    self.assertEqual(db.execute("select path_version()").fetchone()[0], version)

  def test_path_debug(self):
    debug = db.execute("select path_debug()").fetchone()[0].split('\n')
    self.assertEqual(len(debug), 4)

    self.assertTrue(debug[0].startswith("Version: v"))
    self.assertTrue(debug[1].startswith("Date: "))
    self.assertTrue(debug[2].startswith("Source: "))
    self.assertTrue(debug[3].startswith("cwalk version:"))
  
  def test_path_absolute(self):
    path_absolute = lambda arg: db.execute("select path_absolute(?)", [arg]).fetchone()[0]
    self.assertEqual(path_absolute("/a"), 1)
    self.assertEqual(path_absolute("~/a"), 0)
    self.assertEqual(path_absolute("./a"), 0)
  
  def test_path_basename(self):
    path_basename = lambda arg: db.execute("select path_basename(?)", [arg]).fetchone()[0]
    self.assertEqual(path_basename("a/b.txt"), "b.txt")
    self.assertEqual(path_basename("a/b/c.txt"), "c.txt")
    self.assertEqual(path_basename("c.txt"), "c.txt")
    self.assertEqual(path_basename("c"), "c")
    self.assertEqual(path_basename(""), None)
  
  def test_path_dirname(self):
    path_dirname = lambda arg: db.execute("select path_dirname(?)", [arg]).fetchone()[0]
    self.assertEqual(path_dirname("a/b.txt"), "a/")
    self.assertEqual(path_dirname("a/"), None)
    self.assertEqual(path_dirname("a"), None)
    self.assertEqual(path_dirname(""), None)
  
  def test_path_extension(self):
    path_extension = lambda arg: db.execute("select path_extension(?)", [arg]).fetchone()[0]
    self.assertEqual(path_extension("b.txt"), ".txt")
    self.assertEqual(path_extension("b.tar.gz"), ".gz")
    self.assertEqual(path_extension("abc"), None)
  
  def test_path_intersection(self):
    path_intersection = lambda a, b: db.execute("select path_intersection(?, ?)", [a, b]).fetchone()[0]
    self.assertEqual(path_intersection('/this/is/a/test', '/this/is/a/ayoo/what'), "/this/is/a")
  
  def test_path_join(self):
    path_join = lambda *a: db.execute("select path_join({args})".format(args=spread_args(a)), a).fetchone()[0]
    self.assertEqual(path_join("a", "b"), "a/b")
    self.assertEqual(path_join("abc", "xyz"), "abc/xyz")
    # TODO joining is hard :(
    #self.assertEqual(path_join("aa", "bbb", "cccc"), "aa/bbb/cccc")
  
  def test_path_normalize(self):
    path_normalize = lambda arg: db.execute("select path_normalize(?)", [arg]).fetchone()[0]
    self.assertEqual(path_normalize("~/../a/b/./c/../ayoo"), "a/b/ayoo")
  
  def test_path_relative(self):
    path_relative = lambda arg: db.execute("select path_relative(?)", [arg]).fetchone()[0]
    self.assertEqual(path_relative("a/b.txt"), 1)
    self.assertEqual(path_relative("/a/b.txt"), 0)

    # TODO wtf
    self.assertEqual(path_relative(""), 1)
  
  def test_path_root(self):
    path_root = lambda arg: db.execute("select path_root(?)", [arg]).fetchone()[0]
    self.assertEqual(path_root("a/b.txt"), "")
    self.assertEqual(path_root("/a/b.txt"), "/")
    # TODO what does windows do
    self.assertEqual(path_root("C:/a/b.txt"), "")
  
  def test_path_segment_at(self):
    path_segment_at = lambda a, b: db.execute("select path_segment_at(?, ?)", [a, b]).fetchone()[0]
    PATH = "/home/oppenheimer/projects/manhattan/README.md"

    # 0 and positive indicies within bounds works
    self.assertEqual(path_segment_at(PATH, 0), "home")
    self.assertEqual(path_segment_at(PATH, 1), "oppenheimer")
    self.assertEqual(path_segment_at(PATH, 2), "projects")
    self.assertEqual(path_segment_at(PATH, 3), "manhattan")
    self.assertEqual(path_segment_at(PATH, 4), "README.md")

    # negative indicies within bounds wrap around
    self.assertEqual(path_segment_at(PATH, -1), "README.md")
    self.assertEqual(path_segment_at(PATH, -2), "manhattan")
    self.assertEqual(path_segment_at(PATH, -3), "projects")
    self.assertEqual(path_segment_at(PATH, -4), "oppenheimer")
    self.assertEqual(path_segment_at(PATH, -5), "home")

    # negative indicies out of bounds return null
    self.assertEqual(path_segment_at(PATH, -6), None)
    self.assertEqual(path_segment_at(PATH, -7), None)
    self.assertEqual(path_segment_at(PATH, -8), None)

    # postive indicies out of bounds return null
    self.assertEqual(path_segment_at(PATH, 5), None)
    self.assertEqual(path_segment_at(PATH, 6), None)
    self.assertEqual(path_segment_at(PATH, 7), None)
  
  def test_path_segments(self):
    self.assertEqual(execute_all("select rowid, * from path_segments('/home/root/.././.ssh/keys')"), [
      {"rowid": 0, "segment": "home", "type": "normal"},
      {"rowid": 1, "segment": "root", "type": "normal"},
      {"rowid": 2, "segment": "..", "type": "back"},
      {"rowid": 3, "segment": ".", "type": "current"},
      {"rowid": 4, "segment": ".ssh", "type": "normal"},
      {"rowid": 5, "segment": "keys", "type": "normal"},
    ])
class TestCoverage(unittest.TestCase):                                      
  def test_coverage(self):                                                      
    test_methods = [method for method in dir(TestPath) if method.startswith('test_path')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()