# LoopGit

LoopGit is a lightweight, native Git GUI client built with C++, Qt6, and libgit2.

## Why LoopGit?

Git is an essential tool, but many graphical Git clients are either built with web technologies (Electron), making them resource-heavy and slow, or they lack a clean, modern interface.

LoopGit was built to solve this problem. It is a fully native application. It uses libgit2 directly for Git operations, ensuring maximum performance, and Qt6 for a responsive, distraction-free user interface.

## Features

- **Blazing Fast**: Built with C++ and libgit2 for native performance.
- **Modern UI**: A sleek, dark-themed interface built with Qt6.
- **Visual Commit Graph**: Interactive lane-based git commit graph representation.
- **Interactive Conflict Resolver**: Side-by-side visual merge conflict resolution tool.
- **Advanced Diff & Compare**: Compare any two commits by holding `Ctrl` and selecting them in the log.
- **Ahead/Behind Indicators**: View branch sync status (`[↑X ↓Y]`) directly in the side panel.
- **Settings & Remotes UI**: Manage your git configuration and remote URLs without touching the CLI.
- **Reflog & Blame Viewer**: Look up local repository reflog pointer history or blame line annotations.
- **Core Operations & Shortcuts**: Easy push, pull, fetch, commit (amend), stash, cherry-pick, revert with global shortcuts.

## Build Instructions

LoopGit uses CMake as its build system. The build process will automatically fetch and compile `libgit2` if it's not found on your system.

### Prerequisites

- A C++17 compatible compiler (GCC, Clang, MSVC)
- CMake (3.21 or newer)
- Qt 6 (Core, Gui, Widgets, Network)

### Building

```bash
git clone https://github.com/hakanyz/loopgit.git
cd loopgit

mkdir build
cd build

cmake ..
cmake --build .
```

### Running

- On Linux/macOS: `./LoopGit`
- On Windows: `.\LoopGit.exe`

## Contributing

Pull requests and issues are welcome. If you find a bug or want to propose a new feature, feel free to open an issue.

## License

This project is licensed under the MIT License.
