# LoopGit

A fast, lightweight, and native Git GUI client built with C++, Qt6, and libgit2.

LoopGit is a native desktop application designed for speed and low resource usage. Unlike Electron-based alternatives, it starts instantly, consumes minimal memory (typically under 50MB RAM), and provides a clean, distraction-free environment.

## Features

* **Performance**: Built natively in C++17 with direct libgit2 integration.
* **Tabbed Interface**: Manage multiple repositories simultaneously in a single window.
* **Visual Git Graph**: Interactive lane-based commit history representation.
* **Conflict Resolution**: Interactive split-view editor to resolve merge conflicts (Ours/Theirs/Both).
* **Commit Comparison**: Select any two commits in the history to view their changes and inline diffs.
* **Sync Indicators**: At-a-glance branch synchronization status (ahead/behind counts).
* **Repository Settings**: Manage local git config and remotes directly from the interface.
* **Reflog & Blame**: Dedicated tools for inspecting line-by-line blame annotations and local pointer history.
* **Keyboard Navigation**: Optimized workflows with global keyboard shortcuts.

## Keyboard Shortcuts

| Shortcut | Action |
| --- | --- |
| `Ctrl + O` | Open Local Repository |
| `Ctrl + W` | Close Current Tab |
| `Ctrl + F` | Fetch from Remote |
| `Ctrl + P` | Push to Remote |
| `Ctrl + Shift + P` | Pull from Remote |
| `Ctrl + Shift + C` | Focus Commit Message Box |
| `Ctrl + Return` | Commit Staged Changes (when message box is focused) |

## Build Instructions

### Prerequisites
* C++17 compatible compiler (GCC, Clang, or MSVC)
* CMake 3.16 or newer
* Qt 6 SDK (Core, Gui, Widgets)

### Compiling
```bash
git clone https://github.com/hakanyz/loopgit.git
cd loopgit
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Running
* **Windows**: `.\build\Release\LoopGit.exe`
* **macOS / Linux**: `./build/LoopGit`

## License

Distributed under the MIT License. See `LICENSE` for more information.
