# archvital

[![GitHub Release](https://img.shields.io/github/v/release/T9Tuco/archvital?label=latest%20release&color=4f8ef7)](https://github.com/T9Tuco/archvital/releases/latest)
[![AUR Version](https://img.shields.io/aur/version/archvital?label=AUR%20stable&color=4f8ef7)](https://aur.archlinux.org/packages/archvital)
[![AUR Version](https://img.shields.io/aur/version/archvital-git?label=AUR%20git&color=6ea8fe)](https://aur.archlinux.org/packages/archvital-git)

hi, i'm tuco. this is archvital, a desktop system monitor for linux that i built because i wanted something between "ancient terminal table" and "visual cockpit with forty gauges".

if you're here, you're probably one of these:

* an arch user deciding if this replaces btop/htop or just sits next to them
* someone who found it on aur and wants to see the source before installing
* a recruiter trying to figure out if i'm serious (im not)
* me, checking if the latest release actually builds

## what is this place?

it's a qt6/c++20 app that watches your machine and tries to stay calm about it. no cockpit full of gauges, no ancient gray table either, just the numbers you actually look at:

* a compact `overview` page for the important stuff at a glance
* dedicated pages for `cpu`, `memory`, `gpu`, `disk`, `network`, and `tasks`
* per process network and gpu usage in the task manager (network needs `nethogs`, gpu needs an amdgpu/i915/nvidia driver)
* a `diagnostics` page that runs a few health checks (failed systemd units, journal errors, temps, swap, zombie processes) and spits out a copyable report
* a `settings` page for theme colors and which sections even show up
* sparklines, bars, cards, a sidebar that remembers if it was collapsed, all of it saved through `qsettings` so it doesn't forget who you are between launches

## screenshots

### main window
<img src="how it looks/2026-04-19-224810_1914x1068_scrot.png" alt="Main Window" width="600">

### cpu section
<img src="how it looks/2026-04-19-224818_1914x1074_scrot.png" alt="CPU Section" width="600">

### memory section
<img src="how it looks/2026-04-19-224829_1912x1079_scrot.png" alt="Memory Section" width="600">

### task section
<img src="how it looks/2026-04-19-224907_1912x1037_scrot.png" alt="Disk Section" width="600">

### settings section
<img src="how it looks/2026-04-19-224916_1898x1027_scrot.png" alt="Network Section" width="600">

## why it looks like this

most system monitors drift into one of two extremes, they either look like they escaped the 90s and feel cramped, or they throw every metric on screen at once and turn into noise. archvital tries to sit in the middle: enough detail to actually be useful, still pleasant to look at for more than ten seconds.

## tech stack

* `c++20` for the actual work
* `qt6` for the ui and the worker thread that polls everything
* plain `/proc`, `/sys`, and a couple of optional helper binaries (`nvidia-smi`, `nethogs`) instead of some heavy monitoring framework
* `cmake` to build it

## how to run it

the fastest way, if you just want to see it running:

```bash
cmake -B build
cmake --build build
./build/archvital
```

if you already have a `build/` folder, this is usually enough:

```bash
cmake --build build
./build/archvital
```

### actually installing it

one liner, downloads the latest build, installs the binary, registers the desktop entry and icon:

```bash
curl -fsSL https://raw.githubusercontent.com/T9Tuco/archvital/main/install.sh | bash
```

the same command works for updates. if a newer version is out, the app shows `↑` next to the version number in the bottom left of the sidebar, click it and the command is right there.

on arch, there's aur packages instead:

```bash
yay -S archvital      # stable release
yay -S archvital-git  # latest commit
```

both install the binary, desktop entry, and icon. stable tracks tagged releases, git builds whatever is currently on `main`.

or build and install it yourself:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
```

### uninstalling

if you went the one liner or manual route:

```bash
sudo rm /usr/local/bin/archvital
sudo rm /usr/share/applications/archvital.desktop
sudo rm /usr/share/pixmaps/archvital.svg
```

if you went through aur:

```bash
yay -R archvital
# or
yay -R archvital-git
```

## requirements

you'll mainly need:

* linux
* `qt6`
* `cmake`
* a c++ compiler with `c++20` support

on arch that's roughly:

* `qt6-base`
* `cmake`
* `gcc` or `clang`

optional, for extra task manager columns:

* `nethogs`, for per process network usage (needs packet capture rights, usually fine without root since the package ships the right capabilities)

## what's in here

* [`src/core`](./src/core) handles system data collection and the shared data structs
* [`src/sections`](./src/sections) has the actual app pages
* [`src/ui`](./src/ui) has reusable bits like the sidebar, cards, sparklines, and bars
* [`src/mainwindow.cpp`](./src/mainwindow.cpp) wires it all together

## settings and persistence

archvital remembers a few things so it feels like a normal desktop app instead of a goldfish:

* window geometry
* collapsed sidebar state
* last opened section
* selected theme colors
* which sections are shown or hidden

all through `qsettings`.

## current status

it's usable and feels pretty solid day to day, but still actively evolving. if you spot rough edges, weird readings, or ui stuff that feels off, that's useful feedback, not nitpicking.

## contributing

sure, if something's broken or missing:

1. clone the repo
2. make a branch
3. build it, test your change
4. open a pull request

small, focused changes are the easiest to review and the easiest to merge.

## license

mit. do what you want.
