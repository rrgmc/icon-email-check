# Icon Email Check

A tray app for macOS, Windows and Linux that counts unread email, in Qt 6 (C++17, Qt Widgets, CMake). [README.md](README.md) describes the features, the Gmail setup and the build.

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
- Merge into `master` only when the user asks. Then, from the main checkout, fast-forward it and push:

  ```sh
  git fetch origin
  git merge --ff-only origin/master
  git merge --ff-only <branch>
  git push origin master
  ```

  If `master` moved since the branch started, merge `master` into the branch in the worktree first, so the fast-forward works.
- After the merge, remove the worktree and delete the branch: `git worktree remove .claude/worktrees/<branch>` and `git branch -d <branch>`.

## Building

- Each worktree has its own build folder, `build/<preset>`. Configure it once: `cmake --preset dev -DCMAKE_PREFIX_PATH=<Qt>`. On this machine Qt is `~/Qt/6.8.3/macos`.
- Build with `cmake --build --preset dev -j 4` and test with `ctest --preset dev`. Keep the job count at 4: other sessions may build at the same time.
- Keep platform-specific code in `src/platform/`. Development and verification happen on macOS. Say so when a change touches Windows or Linux code, since it was not built here.
