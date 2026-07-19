# LoopGit

LoopGit is a fast, lightweight, and native Git GUI client built for developers who value performance and simplicity. Unlike many modern Git clients built on top of resource-heavy web technologies (Electron), LoopGit is written in C++ using Qt6 and communicates directly with your repositories via `libgit2`.

It delivers instant startup times, low memory consumption (typically under 50MB RAM), and a distraction-free dark interface.

---

## Key Features

### ⚡ Performance & Core
* **Native Speed**: Zero Electron overhead. Built with C++17 and `libgit2`.
* **Multi-Repository Tabs**: Work on multiple repositories simultaneously in a clean tabbed view.
* **Status Summary**: At-a-glance branch synchronization status (`[↑X ↓Y]` ahead/behind indicators).

### 🛠️ Working Directory & Diff
* **Stage & Unstage**: Fast file tree categorized into Staged, Unstaged, and Conflicted files.
* **Interactive Conflict Resolver**: Visual split-view tool to resolve merge conflicts using *Ours*, *Theirs*, or *Both* strategies.
* **Advanced Diff & Compare**: Compare changes between any two commits by holding `Ctrl` and selecting them in the history graph.
* **File Filtering**: Instantly filter modified file trees in large commits to locate specific file types.
* **Automatic Draft Saving**: Your commit message draft is safely saved and restored if the application is closed.

### 📈 History & Repository Views
* **Continuous Git Graph**: Renders a visually clean, color-coded branch lane graph showing merges, tags, and commits.
* **Instant History Search**: Real-time filtering by Commit Hash, Author name, or Message.
* **Blame & Reflog**: Right-click to inspect line-by-line file history (Blame) or view pointer history (Reflog) to recover lost commits.
* **Git Flow Integration**: Direct actions to manage feature, bugfix, release, and hotfix branches.

---

## Keyboard Shortcuts

LoopGit is designed to keep your hands on the keyboard. Below are the default shortcuts (also accessible via `Help -> Keyboard Shortcuts...`):

| Shortcut | Action |
| --- | --- |
| `Ctrl + O` | Open Local Repository |
| `Ctrl + W` | Close Current Tab |
| `Ctrl + F` | Fetch from Remote |
| `Ctrl + P` | Push to Remote (includes dropdown for Force Push) |
| `Ctrl + Shift + P` | Pull from Remote |
| `Ctrl + Shift + C` | Focus Commit Message Box |
| `Ctrl + Return` | Commit Staged Changes (when message box is focused) |

---

## Build Instructions

LoopGit uses CMake. The build process automatically downloads and compiles `libgit2` as a static dependency if it's not found on your system.

### Prerequisites
* A C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
* CMake 3.16 or newer
* Qt 6 (Core, Gui, Widgets)

### Compiling
```bash
git clone https://github.com/hakanyz/loopgit.git
cd loopgit
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Running the Binary
* **Windows**: `.\build\Release\LoopGit.exe`
* **macOS / Linux**: `./build/LoopGit`

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
