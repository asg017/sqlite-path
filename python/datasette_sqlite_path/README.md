# The `datasette-sqlite-path` Datasette Plugin

`datasette-sqlite-path` is a [Datasette plugin](https://docs.datasette.io/en/stable/plugins.html) that loads the [`sqlite-path`](https://github.com/asg017/sqlite-path) extension in Datasette instances, allowing you to generate and work with [TODO](https://github.com/path/spec) in SQL.

```
datasette install datasette-sqlite-path
```

See [`docs.md`](../../docs.md) for a full API reference for the path SQL functions.

Alternatively, when publishing Datasette instances, you can use the `--install` option to install the plugin.

```
datasette publish cloudrun data.db --service=my-service --install=datasette-sqlite-path

```
