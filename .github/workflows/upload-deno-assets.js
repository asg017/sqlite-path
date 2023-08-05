const fs = require("fs").promises;

const compiled_extensions = [
  {
    path: "sqlite-path-macos/path0.dylib",
    name: "deno-darwin-x86_64.path0.dylib",
  },
  {
    path: "sqlite-path-macos-aarch64/path0.dylib",
    name: "deno-darwin-aarch64.path0.dylib",
  },
  {
    path: "sqlite-path-linux_x86/path0.so",
    name: "deno-linux-x86_64.path0.so",
  },
  {
    path: "sqlite-path-windows/path0.dll",
    name: "deno-windows-x86_64.path0.dll",
  },
];

module.exports = async ({ github, context }) => {
  const { owner, repo } = context.repo;
  const release = await github.rest.repos.getReleaseByTag({
    owner,
    repo,
    tag: process.env.GITHUB_REF.replace("refs/tags/", ""),
  });
  const release_id = release.data.id;

  await Promise.all(
    compiled_extensions.map(async ({ name, path }) => {
      return github.rest.repos.uploadReleaseAsset({
        owner,
        repo,
        release_id,
        name,
        data: await fs.readFile(path),
      });
    })
  );
};
