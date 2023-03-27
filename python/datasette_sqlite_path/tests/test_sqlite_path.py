from datasette.app import Datasette
import pytest


@pytest.mark.asyncio
async def test_plugin_is_installed():
    datasette = Datasette(memory=True)
    response = await datasette.client.get("/-/plugins.json")
    assert response.status_code == 200
    installed_plugins = {p["name"] for p in response.json()}
    assert "datasette-sqlite-path" in installed_plugins

@pytest.mark.asyncio
async def test_sqlite_path_functions():
    datasette = Datasette(memory=True)
    response = await datasette.client.get("/_memory.json?sql=select+path_version(),path()")
    assert response.status_code == 200
    path_version, path = response.json()["rows"][0]
    assert path_version[0] == "v"
    assert len(path) == 26