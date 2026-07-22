# LoopGit - Beta 1.0 Feature Set

LoopGit is a modern, fast, and user-friendly Git GUI client built with Qt (C++) and libgit2. It is designed to be a daily driver, replacing tools like SmartGit or GitKraken.

Below is the comprehensive list of features currently implemented in Beta 1.0.

## 1. Repository Management
* **Multi-Repo Dashboard:** A welcoming start screen that lists up to 10 recently opened repositories for quick 1-click access.
* **Clone & Open:** Easily clone remote repositories or open existing local ones.
* **System Tray Integration:** App minimizes to the system tray on close (with a "Don't ask again" option) to stay running in the background for quick access.

## 2. Local Files & Staging
* **Live File Tree:** Visualizes unstaged and staged files in a clear tree structure.
* **Granular Staging:** Stage or unstage individual files, or use "Stage All" / "Unstage All".
* **Diff Viewer:** Live, syntax-highlighted side-by-side diff viewer for unstaged and staged files. Gracefully handles binary files.
* **Stash Management:** "Stash Changes" and "Pop Stash" buttons to quickly save and restore work-in-progress code.
* **.gitignore Editor:** Right-click any unstaged file to automatically add it to `.gitignore` (options include: exact file, by extension, or entire folder).

## 3. Commit Operations
* **Commit Creation:** Write commit messages and commit with a single click.
* **Crash-Safety (Drafts):** Commit messages are auto-saved in real-time. If the app crashes or the user switches repos, the draft is restored automatically.
* **Amend Commit:** A checkbox to amend the last commit easily.
* **Push After Commit:** Optional checkbox to automatically push to the remote after a successful commit.

## 4. History & Commit Graph
* **Visual Graph:** A beautiful, multi-lane commit graph showing branch topologies, merges, and diverging histories.
* **Commit Details:** Shows Author, Date, Hash, and the full commit message.
* **File Filter:** A search bar to instantly filter the list of changed files within a large commit.
* **Historical Diffs:** Clicking a file in a past commit shows its exact diff for that specific point in time.

## 5. Branching & GitFlow
* **Branch Management:** View all local and remote branches. Right-click to Checkout, Create new branch here, Merge into current, or Delete.
* **GitFlow Toolbar:** Quick-action buttons to start or finish GitFlow branches (`Feature`, `Bugfix`, `Release`, `Hotfix`).

## 6. Advanced Git Tools (Context Menus)
* **Cherry-pick:** Right-click any commit in history to cherry-pick it into the current branch.
* **Squash Commits:** Select multiple contiguous commits, right-click, and squash them into a single commit with a new message.
* **Reword Commit:** Right-click the HEAD commit to quickly change its message.
* **Tag Management:** Right-click any commit to create an annotated or lightweight Tag (e.g., `v1.0.0`). Tags are rendered visually on the graph.
* **Blame View:** Right-click any file in the Local Files tree to open a Blame Dialog, showing line-by-line author and commit history.
* **Undo/Revert:** Right-click an unstaged file to discard local changes.

## 7. UI & Aesthetics
* **Dark Mode Native:** A sleek, modern dark theme heavily inspired by VS Code and premium Git clients.
* **SVG Icons:** High-resolution, DPI-aware SVG icons for all toolbar actions and file states.
* **Responsive Splitters:** Resizable panels for file trees, commit messages, and diff viewers that remember their sizes.

---
**Question for Claude:** Based on the above feature set for a modern Git GUI, are there any critical or "must-have" features missing that a professional developer would absolutely need before adopting this as their daily driver instead of SmartGit or GitKraken?
 
