from setuptools import setup
import os

VERSION = "0.1"


def get_long_description():
    with open(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "README.md"),
        encoding="utf8",
    ) as fp:
        return fp.read()


setup(
    name="sqlite-path",
    description="Manage sqlite-path loadable extension.",
    long_description=get_long_description(),
    long_description_content_type="text/markdown",
    author="Alex Garcia",
    url="https://github.com/asg017/sqlite-path",
    project_urls={
        "Issues": "https://github.com/asg017/sqlite-path/issues",
        "CI": "https://github.com/asg017/sqlite-path/actions",
        "Changelog": "https://github.com/asg017/sqlite-path/releases",
    },
    license="Apache License, Version 2.0",
    version=VERSION,
    packages=["sqlite_path"],
    package_data={"sqlite_path": ["cross/**/*"]},
    install_requires=[],
    extras_require={"test": ["pytest"]},
    python_requires=">=3.7",
)
