# Terraria 1.4.5+ Inventory & Character Studio

<p align="center">
  <strong>Modern Terraria inventory editor, character editor and save utility for Terraria Desktop 1.4.5.6.</strong>
</p>

<p align="center">
  Edit inventory, equipment, storage, buffs, Journey research, player appearance and quick presets in one desktop app.
</p>

<p align="center">
  <a href="../../releases/latest">
    <img alt="Download Windows Build" src="https://img.shields.io/badge/Download-Windows%20Build-7c3aed?style=for-the-badge&logo=github&logoColor=white">
  </a>
  <a href=".">
    <img alt="Source Code" src="https://img.shields.io/badge/View-Source%20Code-111827?style=for-the-badge&logo=github&logoColor=white">
  </a>
</p>

---

## Terraria save editor for inventory, character and Journey mode

`Terraria 1.4.5+ Inventory & Character Studio` is a C++ desktop application built for players who want a fast and clean Terraria inventory editor with character customization tools, storage management and QoL actions.

This project is optimized for people searching for:

- Terraria inventory editor
- Terraria character editor
- Terraria 1.4.5 save editor
- Terraria player editor
- Terraria Journey research editor
- Terraria item editor for Windows
- Terraria Desktop 1.4.5.6 inventory manager

It is suitable for GitHub users, showcase posts, release pages and developer portfolios that need a polished open source presentation with source code and a downloadable build.

## Key features

- Compatible with `Terraria Desktop 1.4.5.6`
- Inventory editing with drag-and-drop workflow
- Equipment, armor, dyes and ammo editing
- Storage editing for piggy bank, safe, forge and void vault
- Character creator with colors, style and appearance controls
- Buffs, transformations and Journey research management
- Quick actions like fill money, max HP/mana, sort inventory and god mode preset
- Terraria process and player folder detection
- JSON save/preset workflow for fast local editing
- Mod-friendly item workflow for packs like `Calamity`, `Thorium`, `Fargo's Souls`, `Spirit Mod` and `The Stars Above`

## Why this repository stands out

- Clean desktop UI based on `ImGui`
- Native `C++20` project
- `CMake` build system
- Automatic Windows build pipeline for GitHub Actions
- Ready for GitHub releases with downloadable build artifacts

## Download

If you want the ready-to-use app, click the button above or open the latest release:

- `Latest release:` `../../releases/latest`

If you want the source code, clone the repository or download the ZIP from GitHub.

## Build from source

### Requirements

- `CMake 3.20+`
- `Visual Studio 2022` or another compiler with `C++20` support
- Internet connection on first configure step for dependency fetch

### Windows

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binary output:

```text
build/bin/TerrariaEditor2026.exe
```

### Dependencies

This project automatically fetches:

- `GLFW`
- `Dear ImGui`
- `nlohmann/json`

## Project highlights

### Inventory and storage management

The editor supports fast item movement, sorting and structured management across main inventory, armor, dyes, ammo and multiple storage containers.

### Character customization

You can adjust player name, difficulty, hairstyle, colors, HP, mana and cosmetic settings through a desktop-friendly layout.

### Terraria integration

The application can detect local Terraria player folders and running `Terraria.exe`, making it easier to work with your player data workflow.

### Presets and showcase workflow

The built-in preset and JSON flow is useful for:

- demo accounts
- showcase builds
- QA testing
- mod item loadouts
- creator presets

## SEO summary

If you need a searchable GitHub project page for `Yandex`, `Google`, `Bing` and other search engines, this repository targets high-intent phrases naturally in headings and description, including `Terraria inventory editor`, `Terraria character editor`, `Terraria save editor`, `Terraria 1.4.5 tool`, `Terraria Journey editor` and `Windows Terraria editor`.

## Repository structure

```text
src/        Application source code
build/      Local build output
dist/       Distribution-ready files
.github/    GitHub automation and repo configuration
```

## Notes

- This project is a fan-made utility.
- It is not affiliated with `Re-Logic`.
- Always keep backup copies of your player saves before editing.

## GitHub release tip

Create a tag like `v1.0.0` and push it to GitHub to generate a release build through the included workflow.
