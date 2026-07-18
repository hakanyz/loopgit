<div align="center">
  <h1>🌿 GitZen</h1>
  <p><b>A Fast, Native, and Zen-like Git GUI Client</b></p>

  <p>
    <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17">
    <img src="https://img.shields.io/badge/Qt-6.0%2B-41CD52.svg" alt="Qt6">
    <img src="https://img.shields.io/badge/libgit2-v1.8.x-orange.svg" alt="libgit2">
    <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg" alt="Platforms">
  </p>
</div>

<br>

GitZen is a modern, lightweight, and incredibly fast Git GUI client designed to take the stress out of version control. Built from the ground up using **C++**, **Qt6**, and the powerful **libgit2** library, GitZen provides a seamless, "Zen-like" experience without compromising on professional features.

## ✨ Features

- **🚀 Native Performance:** No Electron, no web-views. Pure C++ and Qt for maximum speed and minimal memory usage.
- **🎨 Zen-like UI/UX:** A distraction-free, modern interface with a custom dark-blue theme (inspired by top-tier IDEs) that replaces the standard "Excel-like" grid with a beautiful, unified canvas.
- **☁️ Full Remote Support:** Complete GitHub integration. Clone, Fetch, Pull, and Push directly from the UI using Personal Access Tokens (PAT).
- **🌳 Beautiful Commit Graph:** An advanced, anti-aliased commit history graph with intelligent branch lane routing and visual commit badges.
- **🔍 Advanced Diff Viewer:** A dual-pane visual diff editor with line-by-line staging and hunk extraction capabilities.
- **🗂️ Interactive Staging:** Effortlessly stage individual files, unstaged changes, or even specific hunks of code.
- **🛡️ Comprehensive Tooling:** Support for Cherry-Picking, Stashing, Reverting, Branch Management, and `.gitignore` editing.

## 📸 Screenshots

*(Replace these placeholders with actual screenshots of your application)*

| Local Files Perspective | History & Graph Perspective |
| :---: | :---: |
| <img src="https://via.placeholder.com/600x400.png?text=Local+Files+View" width="400"/> | <img src="https://via.placeholder.com/600x400.png?text=Commit+History+Graph" width="400"/> |

## 🛠️ Build Instructions

GitZen uses CMake as its build system. `libgit2` is automatically fetched and built if not found on your system.

### Requirements
- **C++17** compatible compiler (GCC, Clang, or MSVC)
- **CMake** 3.16 or higher
- **Qt6** (Widgets module)

### Building on Windows (MinGW / MSVC)
```bash
# Clone the repository
git clone https://github.com/hakanyz/gitzen.git
cd gitzen

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .
```

### Running the application
After a successful build, the executable will be located in the `build` directory:
```bash
./GitZen.exe
```

## 🤝 Contributing
Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📜 License
Distributed under the MIT License. See `LICENSE` for more information.

---
<div align="center">
  <sub>Built with ❤️ by Hakan</sub>
</div>
