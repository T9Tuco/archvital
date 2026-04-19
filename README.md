# archvital

A desktop system monitor for Linux that tries to stay useful, calm, and readable.

`archvital` gives you a clean overview of what your machine is doing without turning the whole screen into a cockpit. It is built with `Qt6`, written in `C++20`, and focused on the stuff you usually care about first: CPU, memory, GPU, disks, network activity, processes, diagnostics, and a customizable theme.

## Why This Exists

Most system monitors drift into one of two extremes:

- they look old and feel cramped
- they show everything at once and become visual noise

`archvital` aims for the middle ground: enough detail to be genuinely useful, but still pleasant to look at for more than ten seconds.

## Features

- compact `Overview` page for the important numbers
- dedicated pages for `CPU`, `Memory`, `GPU`, `Disk`, `Network`, and `Tasks`
- `Diagnostics` page for a few common health checks
- `Settings` page for theme colors and section visibility
- saved UI preferences through `QSettings`
- custom cards, sparklines, bars, and sidebar styling

## Screenshots

### Main Window
![Main Window](how%20it%20looks/2026-04-19-224810_1914x1068_scrot.png)

### CPU Section
![CPU Section](how%20it%20looks/2026-04-19-224818_1914x1074_scrot.png)

### Memory Section
![Memory Section](how%20it%20looks/2026-04-19-224829_1912x1079_scrot.png)

### Task Section
![Disk Section](how%20it%20looks/2026-04-19-224907_1912x1037_scrot.png)

### Settings Section
![Network Section](how%20it%20looks/2026-04-19-224916_1898x1027_scrot.png)

## Quick Start

From the project root:

```bash
cmake -S . -B build
cmake --build build
./build/archvital
```

If you already have a `build/` directory, this is usually enough:

```bash
cmake --build build
./build/archvital
```

## Requirements

You will mainly need:

- Linux
- `Qt6`
- `cmake`
- a C++ compiler with `C++20` support

Package names depend on your distro. On Arch Linux, you will usually want something along the lines of:

- `qt6-base`
- `cmake`
- `gcc` or `clang`

## What Is In Here

- [`src/core`](./src/core) handles system data collection and shared data structs
- [`src/sections`](./src/sections) contains the main app pages
- [`src/ui`](./src/ui) contains reusable UI components like the sidebar, cards, sparklines, and bars
- [`src/mainwindow.cpp`](./src/mainwindow.cpp) wires the app together

## Settings And Persistence

`archvital` stores a few things so it feels like a normal desktop app instead of a goldfish:

- window geometry
- collapsed sidebar state
- last opened section
- selected theme colors
- which sections should be shown or hidden

This is handled with `QSettings`.

## Current Status

The project is already usable and feels pretty solid, but it is still actively evolving. If you spot rough edges, odd readings, or UI stuff that feels off, that is valuable feedback, not nitpicking.

## Contributing

If you want to improve something:

1. clone the repo
2. create a branch
3. build and test your changes
4. open a pull request

Small, focused improvements are very welcome.
