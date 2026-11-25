# Icon Email Check

A tray app for macOS, Windows and Linux that counts unread email, in Qt 6 (C++17, Qt Widgets, CMake). [README.md](README.md) describes the app for its users. [DEVELOPMENT.md](DEVELOPMENT.md) describes the build, the Google OAuth client, CI and releases.

## Git workflow

The `personal-rules` plugin supplies the general rules: every change in a worktree under `.claude/worktrees/`, commits, changelog fragments, releases, and merging by stage (`.github/personal-rules.yml`: `established`). In this repository:

- `master` is protected by a GitHub ruleset: changes reach it only through a pull request whose check **CI result** passes. Push the branch, open the pull request with `gh pr create`, and merge it with `gh pr merge <number> --merge` once CI result passes (`/personal-rules:mmp` does all of it). Never squash or rebase, and do not pass `--delete-branch` from a worktree.
- If `master` moved and the pull request conflicts, merge `master` into the branch in the worktree, resolve the conflicts there and push again.
- Tags are not protected: a release starts from `git push origin vX.Y.Z` on `master`, after `/personal-rules:release` merged the version change.

## Building

- Each worktree has its own build folder, `build/<preset>`. Configure it once: `cmake --preset dev -DCMAKE_PREFIX_PATH=<Qt>`. Where Qt is on this machine goes in `CLAUDE.local.md`.
- Build with `cmake --build --preset dev -j 4` and test with `ctest --preset dev`. Keep the job count at 4: other sessions may build at the same time.
- `.github/workflows/build.yml` builds, tests and packages on Windows, macOS and Linux; `ci.yml` runs it for changes that need a build (Linux only on pull requests, except the `full-paths` it lists), and `release.yml` for a tag `vX.Y.Z`, then publishes the release. When a change adds a folder or a script, update the path rules of `changes` in `ci.yml`. Packaging is in `cmake/Install.cmake`, and `tools/check-package.py` lists the files each package must hold. Each change a user would notice adds a fragment to `changelog.d/` (see its README); never edit `CHANGELOG.md`. The release raises `IEC_VERSION` in `CMakeLists.txt`.
- Keep platform-specific code in `src/platform/`. Development and verification happen on macOS. Say so when a change touches Windows or Linux code, since it was not built here.
