<div align="center">
  <h1>🌿 GitZen</h1>
  <p><b>A ridiculously fast Git GUI that doesn't eat your RAM.</b></p>
</div>

Hey there! 👋 Welcome to **GitZen**.

I built this because I was honestly tired of heavy, Electron-based Git clients that take ages to open and use half my memory just to show a commit log. I wanted something native, fast, and simple. So, I grabbed **C++**, **Qt6**, and **libgit2** and put this together. 

It's designed to give you that clean, distraction-free "Zen" feeling while coding. No cluttered "Excel-like" grids—just a smooth, unified interface that gets out of your way.

### 🚀 Why GitZen?

- **It's Native & Fast:** Pure C++ and Qt. Zero web-views, zero lag.
- **Dark Mode by Default:** A custom, deep blue theme inspired by modern IDEs. Easy on the eyes, especially late at night.
- **GitHub Integration:** Full support for pushing, pulling, and cloning. Just drop in your Personal Access Token (PAT) once, and it remembers it.
- **Line-by-Line Staging:** The diff viewer actually lets you stage exactly the hunks you want.
- **Visual Commit Graph:** I spent a lot of time making the branch routing and commit badges look pretty. No more squiggly bezier curves, just clean, sharp lines!

### 📸 Sneak Peek

*(I'll add some cool screenshots here soon! For now, you just have to trust me, it looks great 😄)*

### 🛠️ How to Build It

If you want to compile it yourself, it's pretty straightforward. It uses CMake and automatically fetches `libgit2` for you.

You'll need:
- A modern C++17 compiler (GCC, Clang, or MSVC)
- CMake (3.16+)
- Qt6 (Make sure the Widgets module is installed)

```bash
# Grab the code
git clone https://github.com/hakanyz/gitzen.git
cd gitzen

# Make a build folder
mkdir build
cd build

# Let CMake do its magic
cmake ..
cmake --build .
```
Then just run `./GitZen.exe` (or `./GitZen` on Linux/macOS) and you're good to go!

### 🤝 Want to help out?
This is a personal project, and it's far from perfect. If you spot a bug or have a cool idea, feel free to open an issue or throw a Pull Request my way. I'd love to see what you add to it!

### 📜 License
MIT License. Do whatever you want with the code!

---
<div align="center">
  <sub>Built with a lot of coffee ☕ and ❤️ by Hakan</sub>
</div>
