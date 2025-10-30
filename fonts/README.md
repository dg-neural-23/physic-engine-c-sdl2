# Fonts Directory

## 📁 Purpose
This directory can contain TrueType fonts (`.ttf`) for the engine to use.

## 🎯 Recommended Font
Download **Hack** font from: https://github.com/source-foundry/Hack/releases

1. Download `Hack-vX.XXX-ttf.zip`
2. Extract `Hack-Regular.ttf` to this folder

## 🔄 Fallback Behavior
The engine tries to load fonts in this order:
1. `./fonts/Hack-Regular.ttf` (local, works on all platforms)
2. `/usr/share/fonts/TTF/Hack-Regular.ttf` (Arch/Manjaro Linux)
3. `/usr/share/fonts/truetype/hack/Hack-Regular.ttf` (Ubuntu/Debian)
4. `C:/Windows/Fonts/arial.ttf` (Windows)
5. `C:/Windows/Fonts/consola.ttf` (Windows Consolas)

## 💡 Note
If you place `Hack-Regular.ttf` in this folder, the engine will be **fully portable** across Windows, Linux, and macOS!
