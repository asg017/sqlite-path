const fs = require("fs").promises;

module.exports = async ({ github, context }) => {
  const {
    repo: { owner, repo },
    sha,
  } = context;
  console.log(process.env.GITHUB_REF);
  const release = await github.rest.repos.getReleaseByTag({
    owner,
    repo,
    tag: process.env.GITHUB_REF.replace("refs/tags/", ""),
  });

  const release_id = release.data.id;
  async function uploadReleaseAsset(name, path) {
    console.log("Uploading", name, "at", path);

    return github.rest.repos.uploadReleaseAsset({
      owner,
      repo,
      release_id,
      name,
      data: await fs.readFile(path),
    });
  }
  await Promise.all([
    uploadReleaseAsset("sqljs-path0.wasm", "path0-sqljs/sqljs.wasm"),
    uploadReleaseAsset("sqljs-path0.js", "path0-sqljs/sqljs.js"),
    uploadReleaseAsset(
      "linux-x86_64-gnu-path0.so",
      "path0-cross/x86_64-linux-gnu/path0.so"
    ),
    uploadReleaseAsset(
      "linux-x86_64-path0.so",
      "path0-cross/x86_64-linux/path0.so"
    ),
    uploadReleaseAsset(
      "linux-i386-path0.so",
      "path0-cross/i386-linux/path0.so"
    ),
    uploadReleaseAsset(
      "linux-aarch64-path0.so",
      "path0-cross/aarch64-linux/path0.so"
    ),
    uploadReleaseAsset(
      "macos-x86_64-path0.dylib",
      "path0-cross/x86_64-macos/path0.dylib"
    ),
    uploadReleaseAsset(
      "macos-aarch64-path0.dylib",
      "path0-cross/aarch64-macos/path0.dylib"
    ),
    uploadReleaseAsset(
      "windows-i3860-path0.dll",
      "path0-cross/i386-windows/path0.dll"
    ),
    uploadReleaseAsset(
      "windows-x86_64-path0.dll",
      "path0-cross/x86_64-windows/path0.dll"
    ),
  ]);

  return;
};
