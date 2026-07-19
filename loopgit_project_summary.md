# LoopGit - Project Summary & History

**LoopGit** is a custom-built, modern, and highly responsive desktop Git client designed with a premium, dark-themed UI (inspired by VS Code). It aims to streamline Git workflows while remaining lightweight.

## Tech Stack
- **Language:** C++17
- **UI Framework:** Qt6 (Widgets)
- **Git Backend:** libgit2
- **Build System:** CMake

## Core Architecture
- **`MainWindow`:** Manages the overall application layout, the top Toolbar, and a `QTabWidget` that allows working with multiple repositories simultaneously. Includes a welcome screen for opening/cloning repos.
- **`RepoWidget`:** The core widget for a single repository. It toggles between two main perspectives: **Local Files** and **History**.
- **`GitManager`:** A robust C++ wrapper around the `libgit2` C library. Handles complex Git operations like staging, committing (with amend), pushing, pulling, fetching, diff generation, and commit graph walking.
- **`DiffViewWidget`:** A custom widget using `QTextEdit` to display Git diffs with syntax highlighting (green for additions, red for deletions).
- **`CommitGraphModel` & `CommitGraphDelegate`:** Custom Qt Model/View classes designed specifically to render a visual, colored Git commit graph (nodes and connecting lines) alongside commit history.

## Implemented Features

### 1. UI/UX & Aesthetics
- High-quality SVG vector icons generated for the entire application.
- Seamless dark theme (`#1E1E1E` background) with custom styling for scrollbars, toolbars, and buttons.
- A smart, separated toolbar layout that clearly groups **Views** (Local/History), **Sync Ops** (Fetch/Pull/Push/Refresh), and **Branch Tools**.
- Status bar for temporary success/error messages and a dedicated status label for current branch tracking.

### 2. Local Files Perspective
- **File Changes Tree:** Displays unstaged and staged files.
- **Staging Controls:** Buttons to Stage/Unstage selected files, or Stage All/Unstage All.
- **Diff Viewer:** Clicking a file shows its real-time diff in the bottom pane.
- **Compact Commit Panel (Right-aligned):** 
  - Text area for the commit message.
  - Checkbox for **Amend last commit**.
  - Checkbox for **Push after commit**.
  - A prominent Commit button.

### 3. History Perspective
- **Interactive Commit Graph:** A beautifully rendered commit history tree showing branches and merges.
- **Commit Details:** Selecting a commit shows the commit message, author, date, and a tree of the files changed in that commit.
- **History Diff Viewer:** Clicking a changed file in a historical commit shows what changed in that specific commit.
- **Branch Management:** A sidebar tree showing Local and Remote branches. Context menu allows for Checkout, Merge, and Delete operations.

### 4. GitFlow & Branch Automation
- **GitFlow Toolbar Buttons:** Dedicated icons for `Feature`, `Bugfix`, `Release`, and `Hotfix`.
- **Auto-Naming:** Clicking a GitFlow button opens a dialog that automatically prefixes the branch name (e.g., `feature/my-new-idea`).
- **One-Click Finish:** A magical **"Finish"** button that detects the current GitFlow branch, checks out `main` (or `master`), merges the branch, and cleanly deletes the local branch, all after a single user confirmation prompt.

---

## What's Next? (Prompting other LLMs)

*You can share this document with Claude or another LLM and ask:*

> "This is the current state of my custom C++/Qt6 Git client, LoopGit. We have a solid foundation with a great dark theme, GitFlow automation, and a custom commit graph. Based on this architecture and feature set, what advanced features, workflow optimizations, or UI/UX enhancements should we add next to make this a world-class Git client? Please suggest both low-hanging fruit and complex 'killer features'."
