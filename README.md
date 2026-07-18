# GitZen

GitZen is a lightweight, native Git GUI client built with C++, Qt6, and libgit2.

## Motivation

Most modern Git clients are built on top of web technologies like Electron. While they look nice, they tend to be heavy, consume excessive RAM, and feel sluggish just to display a simple commit log or diff. 

GitZen was built to solve this problem. It is a fully native application. It uses libgit2 directly for Git operations, ensuring maximum performance, and Qt6 for a responsive, distraction-free user interface.

## Features

- Native performance with minimal memory footprint
- Custom commit history graph rendering
- Integrated diff viewer with hunk-level staging
- Remote support (Clone, Fetch, Pull, Push) via GitHub Personal Access Tokens (PAT)
- Branch management, cherry-picking, stashing, and revert capabilities
- Dark mode interface designed for readability

## Build Instructions

GitZen uses CMake as its build system. The build process will automatically fetch and compile `libgit2` if it's not found on your system.

### Prerequisites
- C++17 compatible compiler
- CMake 3.16+
- Qt6 (Widgets module)

### Windows (MinGW / MSVC)
```bash
git clone https://github.com/hakanyz/gitzen.git
cd gitzen

mkdir build
cd build

cmake ..
cmake --build .
```

After building, you can run the executable directly from the build directory.

## Contributing

Pull requests and issues are welcome. If you find a bug or want to propose a new feature, feel free to open an issue.

## License

This project is licensed under the MIT License.
