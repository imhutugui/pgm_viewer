# PGM Viewer (Qt6 C++)

A simple PGM image viewer with pixel erase functionality, built with Qt6 and C++.

## Features

- **View PGM files** - Supports both ASCII (P2) and binary (P5) PGM formats
- **Pixel Erase** - Click or drag to erase pixels (set to black)
- **Adjustable Brush Size** - Toolbar slider to control brush size (1-50)
- **Zoom** - Mouse wheel to zoom in/out (0.1x - 10x)
- **Pan** - Middle mouse button or Ctrl+drag to pan the view
- **Save/Reset** - Save modified images or reset to original

## Build Requirements

- Qt6 (qt6-base-dev)
- C++17 compatible compiler
- qmake6

## Build Instructions

```bash
# Install dependencies (Debian/Ubuntu)
apt-get install qt6-base-dev build-essential

# Build
qmake6 pgm_viewer.pro
make

# Run
./pgm_viewer [optional: path/to/image.pgm]
```

## Controls

| Action | Control |
|--------|---------|
| Erase pixels | Left click/drag |
| Pan view | Middle click/drag or Ctrl+drag |
| Zoom in/out | Mouse wheel |
| Open file | File > Open or Ctrl+O |
| Save file | File > Save As or Ctrl+Shift+S |
| Reset image | Edit > Reset Image or F5 |

## Files

- `main.cpp` - Application entry point
- `mainwindow.h/cpp` - Main window with menu bar and toolbar
- `imageview.h/cpp` - Custom widget for displaying and editing PGM images
- `mainwindow.ui` - Qt Designer UI file
- `pgm_viewer.pro` - Qt project file

## License

Same as the parent project.
