<a href="https://repology.org/metapackage/teeworlds/versions">
    <img src="https://repology.org/badge/vertical-allrepos/teeworlds.svg" alt="Packaging status" align="right">
</a>

Teeworlds ![GitHub Actions](https://github.com/teeworlds/teeworlds/workflows/Build/badge.svg)
=========

A retro multiplayer shooter
---------------------------

Teeworlds is a free online multiplayer game, available for all major
operating systems. Battle with up to 16 players in a variety of game
modes, including Team Deathmatch and Capture The Flag. You can even
design your own maps!

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software. See license.txt for full license
text including copyright information.

Please visit https://www.teeworlds.com/ for up-to-date information about
the game, including new versions, custom maps and much more.

Originally written by Magnus Auvinen.

---

Teeworlds supports two build systems: CMake and bam.

Building on Linux or macOS (CMake)
==========================

Installing dependencies
-----------------------

    # Debian/Ubuntu
    sudo apt install build-essential cmake git libfreetype6-dev libsdl2-dev libpnglite-dev libwavpack-dev python3
    
    # Fedora
    sudo dnf install @development-tools cmake gcc-c++ git freetype-devel pnglite-devel python3 SDL2-devel wavpack-devel
    
    # Arch Linux (doesn't have pnglite in its repositories)
    sudo pacman -S --needed base-devel cmake freetype2 git python sdl2 wavpack
    
    # macOS
    brew install cmake freetype sdl2


Downloading repository
----------------------

    git clone https://github.com/teeworlds/teeworlds --recurse-submodules
    cd teeworlds
    
    # If you already cloned the repository before, use:
    # git submodule update --init


Building
--------

    mkdir -p build
    cd build
    cmake ..
    make

On subsequent builds, you only have to repeat the `make` step.

You can then run the client with `./teeworlds` and the server with
`./teeworlds_srv`.


Build options
-------------

The following options can be passed to the `cmake ..` command line (between the
`cmake` and `..`) in the "Building" step above.

`-GNinja`: Use the Ninja build system instead of Make. This automatically
parallelizes the build and is generally **faster**. (Needs `sudo apt install
ninja-build` on Debian, `sudo dnf install ninja-build` on Fedora, and `sudo
pacman -S --needed ninja` on Arch Linux.)

`-DDEV=ON`: Enable debug mode and disable some release mechanics. This leads to
**faster** builds.

`-DCLIENT=OFF`: Disable generation of the client target. Can be useful on
headless servers which don't have graphics libraries like SDL2 installed.

Building on Linux or macOS (bam)
==========================

Installing dependencies
-----------------------

    # Debian/Ubuntu 19.10+
    sudo apt install bam git libfreetype6-dev libsdl2-dev libpnglite-dev libwavpack-dev python3
    
    # Fedora
    sudo dnf install bam gcc-c++ git freetype-devel pnglite-devel python3 SDL2-devel wavpack-devel
    
    # Arch Linux (doesn't have pnglite in its repositories)
    sudo pacman -S --needed base-devel bam freetype2 git python sdl2 wavpack
    
    # macOS
    brew install bam freetype sdl2
    
    # other (add bam to your path)
    git clone https://github.com/teeworlds/bam
    cd bam
    ./make_unix.sh


Downloading repository
----------------------

    git clone https://github.com/teeworlds/teeworlds --recurse-submodules
    cd teeworlds
    
    # If you already cloned the repository before, use:
    # git submodule update --init


Building
--------

    bam

The compiled game is located in a sub-folder of `build`. You can run the client from there with `./teeworlds` and the server with `./teeworlds_srv`.


Build options
-------------

One of the following targets can be added to the `bam` command line: `game` (default), `server`, `client`, `content`, `masterserver`, `tools`.

The following options can also be added.

`conf=release` to build in release mode (defaults to `conf=debug`).

`arch=x86` or `arch=x86_64` to force select an architecture.

Building on Windows with Visual Studio & CMake
======================

Download and install some version of [Microsoft Visual
Studio](https://www.visualstudio.com/) (as of writing, MSVS Community 2019)
with the following components:

* Desktop development with C++ (on the main page)
* Python development (on the main page)
* Git for Windows (in Individual Components → Code tools)

Run Visual Studio. Open the Team Explorer (View → Team Explorer, Ctrl+^,
Ctrl+M). Click Clone (in the Team Explorer, Connect → Local Git Repositories).
Enter `https://github.com/teeworlds/teeworlds` into the first input box. Wait
for the download to complete (terminals might pop up).

Wait until the CMake configuration is done (watch the Output windows at the
bottom).

Select `teeworlds.exe` in the Select Startup Item… combobox next to the green
arrow. Wait for the compilation to finish.

For subsequent builds you only have to click the button with the green arrow
again.

Building on Windows with MSVC build tools & bam
======================

Download and install [Microsoft C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) and [Python](https://www.python.org/downloads/).

Download and unzip [Teeworlds stable sources](https://github.com/teeworlds/teeworlds/releases) or [Teeworlds latest sources](https://github.com/teeworlds/teeworlds/archive/master.zip).

Download and unzip [bam](https://github.com/matricks/bam/archive/v0.5.1.tar.gz) to `teeworlds-version\bam`.

Run the `x64 Native Tools Command Prompt` (or `x86` for 32-bit) from the start menu.

    # Navigate to the Teeworlds source directory
    cd ...\teeworlds-version
    
    # Build bam (use make_win32_msvc.bat for 32-bit)
    cd bam
    make_win64_msvc.bat
    copy bam ..
    cd ..
    
    # Build Teeworlds
    bam conf=release

Use `conf=debug` to build the debug version instead. You can also provide a target after the `bam` command : `game` (default), `server`, `client`, `content`, `masterserver`, `tools`.

Building on Windows with MinGW & CMake
======================

Download and install MinGW with at least the following components:

- mingw-developer-toolkit-bin
- mingw32-base-bin
- mingw32-gcc-g++-bin
- msys-base-bin

Also install [Git](https://git-scm.com/downloads) (for downloading the source
code), [Python](https://www.python.org/downloads/) and
[CMake](https://cmake.org/download/).

Open CMake ("CMake (cmake-gui)" in the start menu). Click "Browse Source"
(first line) and select the directory with the Teeworlds source code. Next,
click "Browse Build" and create a subdirectory for the build (e.g. called
"build"). Then click "Configure". Select "MinGW Makefiles" as the generator and
click "Finish". Wait a bit (until the progress bar is full). Then click
"Generate".

You can now build Teeworlds by executing `mingw32-make` in the build directory.
