from setuptools import setup

version = {}
with open("datasette_sqlite_path/version.py") as fp:
    exec(fp.read(), version)

VERSION = version['__version__']

setup(
    name="datasette-sqlite-path",
    description="",
    long_description="",
    long_description_content_type="text/markdown",
    author="Alex Garcia",
    url="https://github.com/asg017/sqlite-path",
    project_urls={
        "Issues": "https://github.com/asg017/sqlite-path/issues",
        "CI": "https://github.com/asg017/sqlite-path/actions",
        "Changelog": "https://github.com/asg017/sqlite-path/releases",
    },
    license="MIT License, Apache License, Version 2.0",
    version=VERSION,
    packages=["datasette_sqlite_path"],
    entry_points={"datasette": ["sqlite_path = datasette_sqlite_path"]},
    install_requires=["datasette", "sqlite-path"],
    extras_require={"test": ["pytest"]},
    python_requires=">=3.7",
)