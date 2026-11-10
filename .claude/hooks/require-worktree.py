#!/usr/bin/env python3
"""Claude Code PreToolUse hook: refuses file edits in the main checkout.

Every change to this repository happens in a worktree under
.claude/worktrees/ (see CLAUDE.md). This hook reads the tool call from
stdin, finds the Git working tree that holds the file, and blocks the call
when that is the main working tree of this repository. Files in a linked
worktree, and files outside the repository, are allowed.
"""
import json
import os
import subprocess
import sys


def git(directory, *args):
    result = subprocess.run(["git", "-C", directory, *args], capture_output=True, text=True)
    return result.stdout.strip().splitlines() if result.returncode == 0 else None


def nearest_existing_dir(path):
    directory = path if os.path.isdir(path) else os.path.dirname(path)
    while directory and not os.path.isdir(directory):
        parent = os.path.dirname(directory)
        if parent == directory:
            return None
        directory = parent
    return directory


def main():
    try:
        call = json.load(sys.stdin)
    except ValueError:
        return 0
    tool_input = call.get("tool_input") or {}
    path = tool_input.get("file_path") or tool_input.get("notebook_path")
    if not path:
        return 0
    path = os.path.abspath(os.path.join(call.get("cwd") or os.getcwd(), path))
    directory = nearest_existing_dir(path)
    if not directory:
        return 0

    info = git(directory, "rev-parse", "--path-format=absolute", "--git-dir", "--git-common-dir", "--show-toplevel")
    if not info or len(info) < 3:
        return 0    # not in a Git working tree
    git_dir, common_dir, top = info[0], info[1], info[2]

    # Only this repository's checkouts are guarded. This script lives in
    # <checkout>/.claude/hooks, in the main checkout or in a worktree.
    here = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ours = git(here, "rev-parse", "--path-format=absolute", "--git-common-dir")
    if not ours or os.path.realpath(ours[0]) != os.path.realpath(common_dir):
        return 0

    # In the main working tree the Git folder is the common one; a linked
    # worktree has its own under .git/worktrees/.
    if os.path.realpath(git_dir) != os.path.realpath(common_dir):
        return 0

    print(
        f"Blocked: {path} is in the main checkout ({top}). In this repository every change "
        "happens in a worktree. Create one first, from the main checkout:\n"
        "  git fetch origin\n"
        f'  git worktree add -b <branch> "{top}/.claude/worktrees/<branch>" origin/master\n'
        "or use EnterWorktree, then edit the file under .claude/worktrees/<branch>/. See CLAUDE.md.",
        file=sys.stderr,
    )
    return 2


if __name__ == "__main__":
    sys.exit(main())
