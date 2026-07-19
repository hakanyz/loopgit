# LoopGit - Feature Specification & Project Summary

LoopGit is a lightweight, fast, and feature-rich Git GUI client built with C++, Qt6, and libgit2. Our goal is to make it a daily driver capable of replacing heavy Electron-based Git clients (like GitKraken or SmartGit) by offering blazing fast performance and essential professional Git features.

Below is the complete feature list implemented so far. Please review it to see if there are any critical missing features for a daily-driver Git GUI before its Beta release.

## 1. Core Repository Management
- **Open / Clone**: Open local Git repositories or clone remote ones over HTTP/HTTPS/SSH.
- **Tabs/Multi-Repo**: Manage multiple open repositories simultaneously with a tabbed interface.
- **Context-Aware UI**: Left sidebar for Branches/Stashes/Tags, Center for Commit History graph, Right sidebar for file details, diffs, and local changes.

## 2. Commit History & Graph (Log View)
- **Visual Git Graph**: Renders a visually continuous and color-coded lane graph representing branches, merges, and commit relationships.
- **Rich Commit List**: Displays Commit Hash, Message, Author, and Date.
- **Commit Details**: Selecting a commit shows detailed metadata and lists the changed files in that commit.
- **Diff Viewer**: Selecting a file from a commit shows a side-by-side or unified inline diff (additions in green, deletions in red).
- **Search & Filter**: Live-search bar to instantly filter the commit history by Hash, Author, or Message.

## 3. Local Changes & Staging Area
- **File Trees**: Categorizes local working directory into "Staged", "Unstaged", and "Conflicted" files.
- **Stage/Unstage**: Double click or right-click to stage/unstage specific files or everything at once.
- **Discard/Revert**: Easily discard local unstaged changes or revert uncommitted files to HEAD.
- **.gitignore Integration**: Right-click on untracked files to automatically add them or their extensions/folders to `.gitignore`.

## 4. Committing & Stashing
- **Commit Action**: Write commit messages and commit staged files locally.
- **Amend Commit**: Support for `--amend` to modify the last commit.
- **Stash Management**: Save local changes to a new stash, apply stashes, or drop stashes directly from the UI.

## 5. Branch Management & Navigation
- **Branch Tree**: Lists Local Branches and Remote Branches.
- **Checkout**: Double-click any branch to instantly switch to it.
- **Create / Delete**: Create new branches from current HEAD, or delete existing local branches.
- **Git Flow Integration**: Built-in buttons to quickly start `feature/`, `bugfix/`, `release/`, or `hotfix/` branches.

## 6. Remote Operations (Sync)
- **Fetch, Pull, Push**: Dedicated toolbar buttons to interact with remotes.
- **Force Push**: Toolbar dropdown includes a "Force Push (with lease)" option to safely overwrite remote history.
- **Remote Management**: A UI dialog to Add, Edit, and Remove remote URLs (e.g., `origin`).
- **Credentials**: Supports PAT (Personal Access Token) credentials for HTTPS auth.

## 7. Advanced Professional Features ("Pro" Tools)
- **Interactive Conflict Resolver**: When a merge/pull results in a conflict, double-clicking the conflicted file opens a dedicated UI. It parses Git markers (`<<<<<<<`, `=======`, `>>>>>>>`) and allows you to resolve code blocks visually choosing "Ours", "Theirs", or "Both".
- **Blame Viewer**: Right-click any tracked file to view line-by-line Blame annotations (Author, Date, Commit Hash).
- **Reflog Viewer**: A dedicated "Show Reflog..." window that lists the local `git reflog` (`HEAD@{0}`, etc.) to help recover lost commits or track repository pointer movements.
- **Cherry-Pick**: Right-click a commit in the graph to cherry-pick it to the current branch.
- **Revert Commit**: Right-click a commit to safely generate a reverting commit.
- **Squash & Reword**: Select multiple contiguous commits in the graph, right-click, and dynamically squash them together or reword messages.

## 8. UX / QoL Improvements
- **History File Filter**: A text box in the commit details panel to quickly filter large file lists (e.g., hiding `.o` or build files to easily find `.cpp` changes).
- **Settings Dialog**: UI to quickly configure Git `user.name` and `user.email`.
- **System Tray**: App can minimize to the system tray for background availability.
- **Modern Dark Theme**: Customized `#1E1E1E` style dark mode, matching VSCode/modern IDE aesthetics.
