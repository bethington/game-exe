# OpenD2

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![CMake](https://img.shields.io/badge/CMake-3.15+-green.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://github.com/yourusername/OpenD2)

An open-source reimplementation of Diablo 2, licensed under the GNU General Public License v3.

## 🎯 Project Description

OpenD2 is a comprehensive reverse-engineering project that aims to recreate the classic action RPG Diablo 2 from the ground up. This project provides a modern, open-source alternative to the original game engine while maintaining full compatibility with original game assets and save files.

**Key Features:**

- 🔄 Drop-in replacement for the original `game.exe`
- 📦 Compatible with original MPQ files and data formats  
- 🌐 TCP/IP and Open Battle.net multiplayer support
- 🏗️ Modular architecture for easy customization
- 🎮 Faithful recreation of original gameplay mechanics
- 🔧 Modern C++ codebase with CMake build system

## 🎮 Project Goals

This project aims to be a **"drag and drop" replacement** for the original game executable and libraries. The reimplementation:

- ✅ Operates with original MPQ files and file formats
- ✅ Provides a solid foundation for modding and enhancement
- ✅ Maintains compatibility with the original game over TCP/IP
- ✅ Supports Open Battle.net connectivity
- ❌ Does **not** support Closed Battle.net (to minimize legal risk)

**Philosophy:** Rather than fixing original game bugs (like the infamous lying character sheet), this project focuses on providing a stable, moddable base. Bug fixes and new features are expected to come from community branches and forks.

## 🛠️ Building the Project

### Prerequisites

- **Operating System:** Windows 10/11
- **Compiler:** Visual Studio 2019 or later (MSVC 19.x)
- **Build System:** CMake 3.15 or higher
- **Git:** For cloning and version control

### Quick Start

1. **Clone the repository:**

   ```bash
   git clone https://github.com/yourusername/OpenD2.git
   cd OpenD2
   ```

2. **Generate build files:**

   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. **Build the project:**

   ```bash
   cmake --build . --config Release
   ```

### Build Targets

The project currently includes the following build targets:

- **`game`** - Main executable (game.exe replacement)
- **`ALL_BUILD`** - Builds all configured targets
- **`ZERO_CHECK`** - CMake utility target

### Build Configuration

The CMake configuration supports the following options:

- `BUILD_GAME` (ON by default) - Build the main game executable
- `BUILD_D2CLIENT` (Planned) - Build D2Client library
- `BUILD_D2GAME` (Planned) - Build D2Game library  
- `BUILD_D2COMMON` (Planned) - Build D2Common library

## 🏗️ Architecture

Just as in the original game, there are several interlocking components driving the game. The difference is that all but the core can be swapped out by a mod.

### Core (game.exe)

The core game engine communicates with all of the other components and drives everything. It's also responsible for loading and running mods.

### Graphics (D2GFX.dll)

The graphics engine draws all of the sprites, handles lighting, and provides some basic postprocessing.

### Sound (D2Sound.dll)

The sound engine is responsible for playing sound effects, music, speech, etc.

### Networking (D2Net.dll)

The networking component is responsible for packet transmission, serialization, etc. over TCP/IP. It's not responsible for anything related to Battle.net.

### Game Logic: Shared (D2Common.dll)

The shared logic component provides a simulation state for the client and server to both run off of. D2Common routines involve pathfinding, dungeon building (DRLG), items, etc.

### Game Logic: Server (D2Game.dll)

The server logic component handles all of the game logic that happens on the server. This includes treasure classes (loot), artificial intelligence, etc.

### Game Logic: Client (D2Client.dll)

The client logic component handles all of the game logic that happens on the client. This includes drawing the user interface.

## 📁 Project Structure

```text
OpenD2/
├── Game/                   # Core game executable source
│   ├── diablo2.cpp        # Main game loop and initialization
│   ├── Diablo2.hpp        # Core game headers
│   └── Platform_Windows.cpp # Windows-specific implementations
├── Client/                 # D2Client library source (planned)
│   └── D2Client.hpp       # Client-side game logic headers
├── Server/                 # D2Game library source (planned)
│   └── D2Game.hpp         # Server-side game logic headers
├── Shared/                 # Common code shared between components
│   ├── D2Shared.cpp       # Shared utility functions
│   ├── D2Shared.hpp       # Shared headers and definitions
│   ├── D2Client.h         # Client interface definitions
│   ├── D2Gfx.h           # Graphics interface
│   ├── D2Sound.h         # Sound interface
│   ├── D2Win.h           # Windows interface
│   ├── Fog.h             # Fog rendering utilities
│   └── Storm.h           # Storm library interface
├── build/                  # CMake build output directory
├── CMakeLists.txt         # CMake build configuration
└── README.md              # This file
```

## 🚀 Getting Started

### Installation

1. Ensure you have a legal copy of Diablo 2 installed
2. Build the OpenD2 project following the compilation steps above
3. Copy the generated `game.exe` to your Diablo 2 installation directory
4. Launch the game normally

### Requirements

- Original Diablo 2 game files (MPQ archives)
- Windows 10/11 operating system
- DirectX compatible graphics card
- Sound card or integrated audio

## 🤝 Contributing

We welcome contributions from the community! Here's how you can help:

### Development

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Make your changes** and ensure they follow the coding standards
4. **Test thoroughly** on different configurations
5. **Commit your changes**: `git commit -m 'Add amazing feature'`
6. **Push to the branch**: `git push origin feature/amazing-feature`
7. **Open a Pull Request**

### Coding Standards

- Follow modern C++ best practices (C++17 or later)
- Use consistent formatting (consider using clang-format)
- Write meaningful commit messages
- Add comments for complex algorithms
- Ensure cross-platform compatibility where possible

### Areas Needing Help

- 🎨 Graphics engine implementation (D2GFX)
- 🔊 Sound system development (D2Sound)
- 🌐 Networking layer (D2Net)
- 🎮 Game logic components (D2Common, D2Game, D2Client)
- 📋 Testing and bug reporting
- 📚 Documentation improvements

## 🐛 Known Issues

- [ ] Project is in early development stage
- [ ] Only core executable is currently implemented
- [ ] Graphics and sound systems are not yet functional
- [ ] Networking layer needs implementation
- [ ] Save file compatibility untested

## 📜 Legal Information

### License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

### Disclaimer

This project is a clean-room reverse engineering effort for educational and preservation purposes. It requires legally obtained copies of the original game files to function. We do not provide, distribute, or encourage piracy of copyrighted game content.

**Important:** You must own a legal copy of Diablo 2 to use this software.

## 🙏 Acknowledgments

- Original Diablo 2 development team at Blizzard Entertainment
- The modding community for keeping the game alive
- Contributors to similar reverse engineering projects
- Open source community for tools and libraries used

## 📞 Support & Community

- **Issues:** Report bugs and feature requests via [GitHub Issues](https://github.com/yourusername/OpenD2/issues)
- **Discussions:** Join community discussions in the [GitHub Discussions](https://github.com/yourusername/OpenD2/discussions)
- **Wiki:** Check the [project wiki](https://github.com/yourusername/OpenD2/wiki) for additional documentation

---

Made with ❤️ by the OpenD2 community

