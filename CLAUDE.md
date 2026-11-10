# Icon Email Check

A tray app for macOS, Windows and Linux that counts unread email, in Qt 6 (C++17, Qt Widgets, CMake). [README.md](README.md) describes the app for its users. [DEVELOPMENT.md](DEVELOPMENT.md) describes the build, the Google OAuth client, CI and releases.

## Git workflow

Every change happens in a Git worktree with its own branch, so several sessions can work on the repository at the same time.

- Before the first edit, create a worktree in `.claude/worktrees/`, from `origin/master`. Run this from the main checkout, with an absolute path:

  ```sh
  git fetch origin
  git worktree add -b <branch> "$PWD/.claude/worktrees/<branch>" origin/master
  ```

  `EnterWorktree` in Claude Code does the same. Name the branch after the work, for example `outlook-support`.
- The main checkout stays on a clean `master`, so other sessions can branch from it. Never edit its files, and never run `git checkout` or `git switch` in it. A new branch in the main checkout is not enough.
- The hook in `.claude/hooks/require-worktree.py` enforces this: it refuses Edit and Write calls on files of the main checkout. Shell commands are not checked, so do not write files there with them either.
- Commit in the worktree as the work progresses, one focused commit per finished piece.
- `master` is protected by a GitHub ruleset: changes reach it only through a pull request whose check "CI result" passes. That check passes when the Windows, macOS and Linux builds pass, or when the pull request changes only Markdown files and nothing is built. Nobody can push to it, force-push it or delete it. Tags are not protected, so a release still starts from `git push origin vX.Y.Z`.
- When a piece is finished and its build and tests pass, push the branch and open a pull request with `gh pr create`. Pushing a branch and opening its pull request need the user's go-ahead.
- Only the user merges, on GitHub or by asking for it. Turn on auto-merge only when the user asks. When the user asks for the merge, use a merge commit: `gh pr merge <number> --merge`. Do not pass `--delete-branch`: from a worktree it fails, because it tries to check out `master`, which the main checkout has.
- If `master` moved since the branch started and the pull request conflicts, merge `master` into the branch in the worktree, resolve the conflicts there and push again.
- After the merge, update the main checkout and clean up, from the main checkout:

  ```sh
  git fetch origin --prune
  git merge --ff-only origin/master
  git worktree remove .claude/worktrees/<branch>
  git branch -d <branch>
  ```

## Building

- Each worktree has its own build folder, `build/<preset>`. Configure it once: `cmake --preset dev -DCMAKE_PREFIX_PATH=<Qt>`. On this machine Qt is `~/Qt/6.8.3/macos`.
- Build with `cmake --build --preset dev -j 4` and test with `ctest --preset dev`. Keep the job count at 4: other sessions may build at the same time.
- `.github/workflows/build.yml` builds, tests and packages on Windows, macOS and Linux; `ci.yml` runs it for every change, and `release.yml` for a tag `vX.Y.Z`, then publishes the release. Packaging is in `cmake/Install.cmake`, and `tools/check-package.py` lists the files each package must hold. Add a line to the `Unreleased` section of `CHANGELOG.md` for each change a user would notice; a release renames that section to its version.
- Keep platform-specific code in `src/platform/`. Development and verification happen on macOS. Say so when a change touches Windows or Linux code, since it was not built here.
