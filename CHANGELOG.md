# Changelog

All notable changes to the LoopGit project will be documented in this file.

## [1.1.22] - 2026-09-21

### Added
- **Standalone Linux .deb Packaging**: self-contained standalone `.deb` package for Debian 12+ and Ubuntu 22.04+ with all Qt6 runtime dependencies bundled.
- **Deterministic Lane Graph Rendering**: GitExtensions-style stable lane coloring where the primary trunk remains solid GitHub Blue and feature branches maintain clean dedicated lane colors.
- **C1 Continuous Splines**: smooth cubic Bézier curves connecting fork and merge commits without abrupt angular joints.
- **Stash Leaf Nodes**: stashes render cleanly as compact diamond markers without drawing persistent transit lines across unrelated history.
- **Project Restructuring**: organized all C++ source files into `src/` and resources into `resources/`.
- **Licensing**: updated open source license to GNU General Public License v3.0 (GPL-3.0).

## [1.0.0-beta] - 2026-07-19

This is the first feature-complete Beta release of LoopGit, offering a native, lightweight, and modern Git client experience.

### Added
- **Interactive Conflict Resolver**: visual split view to resolve git conflicts inline with "Ours", "Theirs" and "Both" options.
- **Commit & History Search**: instant commit filtering via UI search bar (by Hash, Author, Message).
- **Settings & Remotes Management**: UI to configure Git user name/email, and add/edit/remove remote configurations.
- **Reflog & Blame Viewer**: full support for local reflog listing and blame line-by-line annotations.
- **Commit Comparison**: hold `Ctrl` to select two commits and inspect their mutual differences in a split-screen dialog.
- **Ahead/Behind Indicators**: local branches display status markers like `[↑X ↓Y]`.
- **Keyboard Shortcuts & Tooltips**: globally mapped shortcuts for Push (`Ctrl+P`), Pull (`Ctrl+Shift+P`), Fetch (`Ctrl+F`), and Commit Focus (`Ctrl+Shift+C`).
- **Tray Icon Support**: minimize to system tray for quick access in the background.
- **Visual Commit Graph**: custom painting delegate for displaying git repository lanes and merge bubbles.
- **Git Flow Integration**: dedicated shortcuts to start features, bugfixes, releases, and hotfixes.
- **Tabs Support**: handle multiple active repositories simultaneously.
- **Modern Dark Theme**: clean dark color palette based on IDE standards.
 
