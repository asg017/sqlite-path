.load target/debug/libsqlite_path sqlite3_path_init

select path_version();
select path_at("a/b/c", 0);
select path_at("/a/b/c", 0);
