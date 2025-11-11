# OpenD2 Reimplementation Plan

## Executive Summary

This document outlines a comprehensive plan for continuing the Diablo 2 reimplementation using Ghidra analysis tools and existing binary analysis documents. The project already has:

- **Comprehensive Analysis**: 22 detailed binary analysis documents covering all major DLLs
- **Partial Implementation**: Core game.exe initialization and command-line parsing
- **Built DLLs**: 29 DLLs available in `build/Release/`
- **Architecture**: Modular design separating Game, Client, Server, and Shared components

## Current Status Assessment

### What's Already Done

1. **Binary Analysis** (docs/analysis/)
   - ✅ GAME_EXE_BINARY_ANALYSIS.md (248 KB, 125 documented functions)
   - ✅ All major DLL analyses (D2Client, D2Common, D2Game, D2Gfx, D2Win, etc.)
   - ✅ Function signatures, parameters, and call graphs documented
   - ✅ Data structures and global variables identified

2. **Source Code Implementation** (partial)
   - ✅ Game/diablo2.cpp: Command-line parsing, configuration management
   - ✅ Game/Diablo2.hpp: Core structures (D2GameConfigStrc, OpenD2ConfigStrc)
   - ✅ Shared/: Interface headers for all DLL subsystems
   - ⚠️ Function stubs in D2Stubs.cpp (need implementation)

3. **Build System**
   - ✅ CMake configuration
   - ✅ 29 DLLs built in build/Release/
   - ✅ Visual Studio 2019+ support

### What Needs Implementation

1. **Core Game Engine** (Game/)
   - Main game loop (currently stubbed in InitGame())
   - Module frame pumping (PumpModuleFrame needs full implementation)
   - Window creation and event handling
   - Resource cleanup and shutdown

2. **DLL Interface Layer** (Shared/)
   - Implement FOG functions (10019, 10021, 10089, 10090, 10101, 10143, 10218)
   - Implement D2WIN functions (10000, 10001, 10002, 10036, 10037, 10171, etc.)
   - Implement D2GFX functions (10001, 10011, 10015, 10018, 10020)
   - Implement D2SOUND functions (10000, 10001)

3. **Platform Layer** (Game/Platform_Windows.cpp)
   - Windows-specific initialization
   - Registry access
   - Service mode support (Windows NT service capability)

4. **Module System**
   - Implement module interface (D2InterfaceModules)
   - Launch module (D2I_LAUNCH)
   - Client/Server modules
   - Menu/Game/Character modules

## Ghidra MCP Tools Setup

### Status: NOT CURRENTLY AVAILABLE

The Ghidra MCP server is not currently configured. To enable Ghidra integration:

### Option 1: Install Ghidra MCP Server

1. **Install Ghidra** (if not already installed)
   ```bash
   # Download from https://ghidra-sre.org/
   # Extract to a permanent location
   # Set GHIDRA_INSTALL_DIR environment variable
   ```

2. **Install Ghidra MCP Server**
   ```bash
   # Install the MCP server package for Ghidra
   # Configure Claude Code to use the Ghidra MCP server
   # Add server configuration to Claude Code settings
   ```

3. **Load game.exe in Ghidra**
   ```bash
   # Create new Ghidra project
   # Import game.exe (32-bit Windows executable)
   # Run auto-analysis
   # Open Ghidra MCP server connection
   ```

### Option 2: Use Existing Analysis Documents (RECOMMENDED)

**Current Approach**: The existing binary analysis documents are comprehensive and cover 100% of game.exe functions. This approach is:
- ✅ **Faster**: No Ghidra setup required
- ✅ **Complete**: All functions already documented
- ✅ **Detailed**: Includes call graphs, data structures, and usage notes
- ✅ **Tested**: Analysis has been verified and refined

**Workflow**:
1. Read relevant sections of binary analysis documents
2. Implement functions based on documented signatures and behavior
3. Use existing DLLs in build/Release/ for testing
4. Cross-reference with original game behavior

## Implementation Roadmap

### Phase 1: Core Infrastructure (Week 1-2)

**Priority**: Critical foundation for all other work

#### 1.1 FOG Library Implementation
- **File**: `Shared/Fog.cpp` (new file)
- **Functions**: 7 core FOG functions
- **Difficulty**: Medium
- **Dependencies**: None (low-level utility library)

Functions to implement:
```cpp
// Fog.cpp
int FOG_10019(char* gameName, int param2, char* version, int majorVersion);  // Init system
int FOG_10021(char* logPath);                                                 // Init log manager
int FOG_10089(int asyncMode, int param2);                                     // Init async data
int FOG_10090();                                                               // Destroy async data
int FOG_10101(bool bDirect, int param2);                                      // Set working directory
int FOG_10143(void* memPtr);                                                  // Kill fog memory
int FOG_10218();                                                               // Init mask table
```

**Analysis Reference**: docs/analysis/FOG_BINARY_ANALYSIS.md

#### 1.2 Module System Interface
- **File**: `Game/Diablo2.hpp` (extend existing)
- **Structures**: Module interface definitions
- **Difficulty**: Low

Add module definitions:
```cpp
// Module interface function pointer type
typedef D2InterfaceModules (*ModuleFrameFunc)(D2GameConfigStrc* pConfig);

// Module registry
extern ModuleFrameFunc gpfModules[D2I_MAX];

// Module initialization
void Sys_InitModules();
```

#### 1.3 Platform-Specific Initialization
- **File**: `Game/Platform_Windows.cpp` (extend existing)
- **Functions**: Registry access, OS detection
- **Difficulty**: Medium
- **Dependencies**: Windows API

Implement:
```cpp
// Platform detection (already in CRTStartup, needs extraction)
bool Platform_DetectOS(DWORD* platformId, DWORD* majorVer, DWORD* minorVer);

// Registry functions
bool Platform_ReadRegistry(const char* keyPath, const char* valueName, void* buffer);
bool Platform_WriteRegistry(const char* keyPath, const char* valueName, void* data);

// Window creation helpers
HWND Platform_CreateGameWindow(HINSTANCE hInst, bool windowed, int width, int height);
```

### Phase 2: Graphics and Window Subsystem (Week 3-4)

**Priority**: Required for visual output

#### 2.1 D2WIN Implementation
- **File**: `Shared/D2Win.cpp` (new file)
- **Functions**: 15+ D2WIN functions
- **Difficulty**: High
- **Dependencies**: Windows GDI, MPQ archive access

Functions to implement:
```cpp
// Archive management
bool D2WIN_10037();                                                            // Load archives
bool D2WIN_10171(void* func1, void* func2, int param3, D2GameConfigStrc* cfg); // Load expansion archives
void D2WIN_10036();                                                            // Unload archives

// Window and rendering
bool D2WIN_10000(HINSTANCE hInst, DWORD renderMode, bool windowed, bool compress);
bool D2WIN_10001(bool windowed, int spriteMode);
void D2WIN_10002();                                                            // Destroy sprite cache

// Callback functions
int D2WIN_10174();                                                             // Expansion check callback
int D2WIN_10205();                                                             // MPQ loading callback
```

**Analysis Reference**: docs/analysis/D2WIN_BINARY_ANALYSIS.md

#### 2.2 D2GFX Implementation
- **File**: `Shared/D2Gfx.cpp` (new file)
- **Functions**: 10+ D2GFX functions
- **Difficulty**: High
- **Dependencies**: DirectDraw, Direct3D, or GDI

Functions to implement:
```cpp
void D2GFX_10001();                                                            // Destroy window
void D2GFX_10011(bool perspective);                                            // Set perspective mode
void D2GFX_10015();                                                            // Enable low-end graphics
void D2GFX_10018(DWORD gamma);                                                // Set gamma
void D2GFX_10020();                                                            // Enable vsync
```

**Analysis Reference**: docs/analysis/D2GFX_BINARY_ANALYSIS.md

### Phase 3: Audio Subsystem (Week 5)

**Priority**: Medium (can be stubbed initially)

#### 3.1 D2SOUND Implementation
- **File**: `Shared/D2Sound.cpp` (new file)
- **Functions**: 5+ D2SOUND functions
- **Difficulty**: Medium
- **Dependencies**: DirectSound or modern audio API

Functions to implement:
```cpp
bool D2SOUND_10000(DWORD expansion);                                           // Initialize sound system
void D2SOUND_10001();                                                          // Destroy sound system
void D2SOUND_PlaySFX(int soundId, int volume, int pan);
void D2SOUND_PlayMusic(int musicId, bool loop);
void D2SOUND_SetVolume(int masterVol, int sfxVol, int musicVol);
```

**Analysis Reference**: docs/analysis/D2SOUND_BINARY_ANALYSIS.md

### Phase 4: Game Loop and Modules (Week 6-7)

**Priority**: Critical for game functionality

#### 4.1 Module System Implementation
- **File**: `Game/Modules.cpp` (new file)
- **Modules**: Launch, Menu, Character Select, Game
- **Difficulty**: High
- **Dependencies**: All previous phases

Implement module handlers:
```cpp
// Module frame functions
D2InterfaceModules LaunchModule(D2GameConfigStrc* pConfig);
D2InterfaceModules MenuModule(D2GameConfigStrc* pConfig);
D2InterfaceModules CharacterSelectModule(D2GameConfigStrc* pConfig);
D2InterfaceModules GameModule(D2GameConfigStrc* pConfig);

// Register modules
void Sys_InitModules() {
    gpfModules[D2I_LAUNCH] = LaunchModule;
    gpfModules[D2I_MENU] = MenuModule;
    gpfModules[D2I_CHARSELECT] = CharacterSelectModule;
    gpfModules[D2I_GAME] = GameModule;
}
```

#### 4.2 Main Game Loop
- **File**: `Game/diablo2.cpp` (extend existing InitGame)
- **Function**: Complete the game loop in InitGame()
- **Difficulty**: Medium

Complete implementation:
```cpp
// Starting with Launch, ping-pong between all of the different modules until one dies
dwCurrentModule = D2I_LAUNCH;
while (dwCurrentModule != D2I_NONE) {
    dwCurrentModule = PumpModuleFrame(dwCurrentModule, &config);

    // Process Windows messages
    if (!Platform_ProcessMessages()) {
        break;
    }

    // Frame rate limiting (25 FPS game logic)
    Platform_SleepMs(40);
}
```

### Phase 5: Networking Layer (Week 8-9)

**Priority**: Required for multiplayer

#### 5.1 D2NET Implementation
- **File**: `Shared/D2Net.cpp` (new file)
- **Functions**: 20+ D2NET functions
- **Difficulty**: High
- **Dependencies**: TCP/IP stack

**Analysis Reference**: docs/analysis/D2NET_BINARY_ANALYSIS.md

#### 5.2 D2MCPCLIENT Implementation
- **File**: `Shared/D2MCPClient.cpp` (new file)
- **Functions**: MCP (Master Control Program) protocol
- **Difficulty**: High

**Analysis Reference**: docs/analysis/D2MCPCLIENT_BINARY_ANALYSIS.md

### Phase 6: Game Logic Integration (Week 10+)

**Priority**: Required for actual gameplay

#### 6.1 D2Common Library
- **File**: `Shared/D2Common.cpp` (new file)
- **Functions**: 100+ functions (pathfinding, DRLG, items, etc.)
- **Difficulty**: Very High

**Analysis Reference**: docs/analysis/D2COMMON_BINARY_ANALYSIS.md

#### 6.2 D2Client Library
- **File**: `Client/D2Client.cpp` (extend existing)
- **Functions**: 50+ client-side functions
- **Difficulty**: High

**Analysis Reference**: docs/analysis/D2CLIENT_BINARY_ANALYSIS.md

#### 6.3 D2Game Library
- **File**: `Server/D2Game.cpp` (new file)
- **Functions**: 50+ server-side functions
- **Difficulty**: High

**Analysis Reference**: docs/analysis/D2GAME_BINARY_ANALYSIS.md (if available)

## Implementation Methodology

### For Each Function/Module

1. **Analysis Phase**
   - Read corresponding binary analysis document
   - Identify function signature, parameters, return type
   - Note dependencies and called functions
   - Document global variables accessed

2. **Planning Phase**
   - Determine if function can use existing DLLs or needs reimplementation
   - Identify required data structures
   - Plan testing approach

3. **Implementation Phase**
   - Write function signature in appropriate file
   - Implement function body based on analysis
   - Add error handling
   - Add logging for debugging

4. **Testing Phase**
   - Test with original game.exe DLLs (if possible)
   - Compare behavior with original game
   - Test edge cases and error conditions

5. **Integration Phase**
   - Ensure function integrates with existing code
   - Update build configuration if needed
   - Document any deviations from original behavior

### Using Existing DLLs

The `build/Release/` directory contains 29 pre-built DLLs. Strategy:

1. **Initial Development**: Link against original DLLs
   - Allows game.exe to run with partial implementation
   - Can test new code against known-good DLL behavior
   - Faster iteration during development

2. **Progressive Replacement**: Replace DLLs one at a time
   - Start with simplest DLLs (FOG, D2SOUND)
   - Work up to complex DLLs (D2COMMON, D2CLIENT)
   - Maintain backward compatibility

3. **Testing Strategy**:
   - **Hybrid Mode**: Run game.exe with mix of original/reimplemented DLLs
   - **Full Mode**: Run entirely with reimplemented DLLs
   - **Comparison Mode**: Log function calls and compare with original

### Using Ghidra (When Available)

If Ghidra MCP tools are set up:

1. **Interactive Analysis**
   - Navigate to function addresses from analysis documents
   - View decompiled code for complex functions
   - Examine cross-references and call graphs

2. **Data Structure Extraction**
   - Extract struct definitions automatically
   - Generate header files from Ghidra analysis
   - Verify offsets and sizes

3. **Symbol Importing**
   - Import PDB symbols if available (X:\trunk\Diablo2\Builder\PDB\Game.pdb)
   - Enhance analysis with official function names
   - Cross-reference with community reverse engineering efforts

### Without Ghidra (Current Approach)

**Primary Resource**: Binary analysis documents in `docs/analysis/`

**Workflow**:

1. **Identify Target Function**
   - Determine which function needs implementation
   - Find function in relevant analysis document
   - Example: FOG_10019 → docs/analysis/FOG_BINARY_ANALYSIS.md

2. **Extract Implementation Details**
   - Read function documentation
   - Note parameters, return type, calling convention
   - Identify dependencies and called functions
   - Review pseudocode or assembly if provided

3. **Implement Function**
   - Create function signature matching analysis
   - Implement behavior based on documented description
   - Add error handling and validation
   - Use same calling convention (__cdecl, __stdcall, __thiscall, __fastcall)

4. **Cross-Reference**
   - Check if function is called from game.exe analysis (GAME_EXE_BINARY_ANALYSIS.md)
   - Verify parameters match call sites
   - Ensure return values are handled correctly

## Directory Structure and Organization

```
OpenD2/
├── Game/                          # Core game executable
│   ├── diablo2.cpp               # Main game initialization (DONE)
│   ├── Diablo2.hpp               # Core structures (DONE)
│   ├── Platform_Windows.cpp      # Windows-specific code (PARTIAL)
│   ├── Modules.cpp               # Module system (TODO)
│   └── GameLoop.cpp              # Main game loop (TODO)
│
├── Client/                        # D2Client reimplementation
│   ├── D2Client.cpp              # Client-side logic (TODO)
│   └── D2Client.hpp              # Client interface (DONE)
│
├── Server/                        # D2Game reimplementation
│   ├── D2Game.cpp                # Server-side logic (TODO)
│   └── D2Game.hpp                # Server interface (DONE)
│
├── Shared/                        # Common code and DLL interfaces
│   ├── D2Shared.cpp              # Shared utilities (DONE)
│   ├── D2Shared.hpp              # Shared headers (DONE)
│   ├── D2Stubs.cpp               # Function stubs (REPLACE)
│   │
│   ├── Fog.h/.cpp                # FOG library (TODO)
│   ├── Storm.h/.cpp              # Storm library (TODO)
│   ├── D2Win.h/.cpp              # D2Win library (TODO)
│   ├── D2Gfx.h/.cpp              # D2Gfx library (TODO)
│   ├── D2Sound.h/.cpp            # D2Sound library (TODO)
│   ├── D2Net.h/.cpp              # D2Net library (TODO)
│   ├── D2MCPClient.h/.cpp        # D2MCPClient library (TODO)
│   ├── D2Common.h/.cpp           # D2Common library (TODO)
│   └── D2Launch.h/.cpp           # D2Launch library (TODO)
│
├── docs/
│   ├── analysis/                  # Binary analysis documents
│   │   ├── GAME_EXE_BINARY_ANALYSIS.md       (248 KB - PRIMARY REFERENCE)
│   │   ├── FOG_BINARY_ANALYSIS.md
│   │   ├── STORM_BINARY_ANALYSIS.md
│   │   ├── D2WIN_BINARY_ANALYSIS.md
│   │   ├── D2GFX_BINARY_ANALYSIS.md
│   │   ├── D2SOUND_BINARY_ANALYSIS.md
│   │   ├── D2NET_BINARY_ANALYSIS.md
│   │   ├── D2CLIENT_BINARY_ANALYSIS.md
│   │   ├── D2COMMON_BINARY_ANALYSIS.md
│   │   └── [18 more analysis documents]
│   │
│   └── REIMPLEMENTATION_PLAN.md   # This document
│
└── build/
    └── Release/
        └── *.dll                  # 29 pre-built DLLs for testing
```

## Testing Strategy

### Level 1: Unit Testing
- Test individual functions in isolation
- Mock dependencies where needed
- Verify return values and side effects

### Level 2: Integration Testing
- Test modules working together
- Test with mix of original/reimplemented DLLs
- Verify compatibility with original game data

### Level 3: System Testing
- Run full game with reimplemented components
- Test single-player mode
- Test multiplayer mode (TCP/IP)
- Test with various command-line options

### Level 4: Compatibility Testing
- Test with original MPQ files
- Test save file loading/saving
- Test with original game clients (multiplayer)

## Development Tools and Resources

### Required Tools
- ✅ Visual Studio 2019+ (MSVC compiler)
- ✅ CMake 3.15+
- ✅ Git for version control
- ⚠️ Ghidra (optional, for enhanced analysis)

### Reference Materials
1. **Binary Analysis Documents** (docs/analysis/)
   - Primary resource for implementation details
   - Contains function signatures, behavior, call graphs

2. **Original Game Files**
   - game.exe and DLLs for behavior verification
   - MPQ archives for testing

3. **Community Resources**
   - Phrozen Keep (Diablo II modding community)
   - D2Mods forums
   - GitHub repositories (check for existing reimplementation efforts)

4. **Technical Documentation**
   - Windows API documentation (MSDN)
   - DirectX SDK documentation
   - Network protocol documentation (if available)

## Risk Mitigation

### Technical Risks

1. **Incomplete Analysis**
   - **Risk**: Some functions may not be fully documented
   - **Mitigation**: Cross-reference multiple sources, use Ghidra for deep dive
   - **Fallback**: Link against original DLL for missing functions

2. **Complex Algorithms**
   - **Risk**: DRLG (dungeon generation), pathfinding may be complex
   - **Mitigation**: Study existing community research, implement incrementally
   - **Fallback**: Use reference implementation from original DLL initially

3. **Hardware-Specific Code**
   - **Risk**: DirectX, Glide, OpenGL rendering paths
   - **Mitigation**: Start with GDI (simplest), add 3D rendering later
   - **Fallback**: Support windowed mode only initially

4. **Networking Complexity**
   - **Risk**: Network protocol, packet handling, synchronization
   - **Mitigation**: Start with TCP/IP, defer Battle.net
   - **Fallback**: Single-player mode works without networking

### Legal Risks

1. **Copyright Concerns**
   - **Risk**: Reimplementation might face legal challenges
   - **Mitigation**: Clean-room approach, no copied code, GPL license
   - **Best Practice**: Don't distribute copyrighted assets (MPQ files)

2. **Trademark Issues**
   - **Risk**: Using "Diablo II" name
   - **Mitigation**: Use "OpenD2" name, clearly state unofficial status
   - **Best Practice**: Require users to own original game

## Success Metrics

### Phase 1 Success Criteria
- ✅ FOG library functions implemented
- ✅ Module system framework in place
- ✅ Platform initialization working
- ✅ Project compiles without errors

### Phase 2 Success Criteria
- ✅ Game window opens
- ✅ Basic graphics rendering works
- ✅ Archive loading (MPQ) functional
- ✅ Splash screen displays

### Phase 3 Success Criteria
- ✅ Sound system initializes
- ✅ Audio playback works (music/SFX)
- ✅ Volume controls functional

### Phase 4 Success Criteria
- ✅ Game launches and shows main menu
- ✅ Module transitions work (menu → character select → game)
- ✅ Game loop runs at correct framerate

### Phase 5 Success Criteria
- ✅ TCP/IP multiplayer connects
- ✅ Multiple clients can join game
- ✅ Network synchronization works

### Phase 6 Success Criteria (ULTIMATE GOAL)
- ✅ Full single-player game playable
- ✅ Multiplayer over TCP/IP functional
- ✅ Compatible with original game files
- ✅ Save games work
- ✅ All 5 acts completable

## Timeline Estimate

| Phase | Duration | Dependencies | Risk |
|-------|----------|--------------|------|
| Phase 1: Core Infrastructure | 2 weeks | None | Low |
| Phase 2: Graphics/Window | 2 weeks | Phase 1 | Medium |
| Phase 3: Audio | 1 week | Phase 1 | Low |
| Phase 4: Game Loop | 2 weeks | Phase 1-3 | Medium |
| Phase 5: Networking | 2 weeks | Phase 4 | High |
| Phase 6: Game Logic | 8+ weeks | All previous | Very High |
| **TOTAL** | **17+ weeks** | | |

**Note**: This is an aggressive timeline. Actual implementation may take longer depending on:
- Developer experience with reverse engineering
- Time availability
- Complexity of undocumented features
- Testing and debugging requirements

## Next Steps (Immediate Actions)

### 1. Set Up Development Environment (If Not Done)
```bash
# Clone repository
cd C:\Users\benam\source\cpp\game-exe

# Verify build system works
cd build
cmake ..
cmake --build . --config Release

# Test current implementation
.\Release\game.exe -w
```

### 2. Choose Implementation Path

**Option A: With Ghidra MCP**
1. Install Ghidra and Ghidra MCP server
2. Load game.exe in Ghidra project
3. Configure Claude Code to use Ghidra MCP
4. Begin interactive analysis and implementation

**Option B: Without Ghidra (RECOMMENDED FOR NOW)**
1. Start with Phase 1 (Core Infrastructure)
2. Implement FOG functions from FOG_BINARY_ANALYSIS.md
3. Test with existing DLLs in build/Release/
4. Proceed to Phase 2 once foundation is solid

### 3. Create First Implementation (FOG Library)

**File to create**: `Shared/Fog.cpp`

Based on docs/analysis/FOG_BINARY_ANALYSIS.md, implement:
```cpp
// Fog.cpp
#include "Fog.h"
#include <windows.h>
#include <stdio.h>

// Initialize Fog system
int FOG_10019(char* gameName, int param2, char* version, int majorVersion) {
    // Implementation based on binary analysis
    // - Register application name
    // - Initialize version info
    // - Set up global state
    return 1;  // Success
}

// Initialize logging system
int FOG_10021(char* logPath) {
    // Implementation based on binary analysis
    // - Create log directory
    // - Open log file
    // - Set up logging callbacks
    return 1;  // Success
}

// Additional functions...
```

### 4. Test Incrementally
```bash
# Build after each function
cmake --build . --config Release

# Test with original DLLs
.\Release\game.exe -w -log
```

### 5. Document Progress
- Update TODO comments in code
- Document deviations from original behavior
- Track completed functions in separate checklist

## Appendix A: Key Functions from Binary Analysis

### Game.exe Entry Points (GAME_EXE_BINARY_ANALYSIS.md)

| Function | Address | Purpose | Priority |
|----------|---------|---------|----------|
| CRTStartup | 0x0040122e | C Runtime entry | ✅ DONE |
| D2ServerMain | 0x00407600 | Main game function | 🔴 TODO |
| InitializeD2Game | 0x00407200 | Initialize game | 🔴 TODO |
| RunGameMainLoop | 0x00407650 | Main game loop | 🔴 TODO |
| LoadGameDLLs | 0x00407450 | Load D2 DLLs | 🔴 TODO |

### FOG Library Functions (FOG_BINARY_ANALYSIS.md)

| Function | Purpose | Priority |
|----------|---------|----------|
| FOG_10019 | Init system | 🟡 HIGH |
| FOG_10021 | Init log manager | 🟡 HIGH |
| FOG_10089 | Init async data | 🟡 HIGH |
| FOG_10090 | Destroy async data | 🟢 MEDIUM |
| FOG_10101 | Set working directory | 🟡 HIGH |
| FOG_10143 | Kill fog memory | 🟢 MEDIUM |
| FOG_10218 | Init mask table | 🟢 MEDIUM |

### D2WIN Library Functions (D2WIN_BINARY_ANALYSIS.md)

| Function | Purpose | Priority |
|----------|---------|----------|
| D2WIN_10000 | Create window | 🔴 CRITICAL |
| D2WIN_10001 | Init sprite cache | 🟡 HIGH |
| D2WIN_10002 | Destroy sprite cache | 🟢 MEDIUM |
| D2WIN_10036 | Unload archives | 🟡 HIGH |
| D2WIN_10037 | Load archives | 🔴 CRITICAL |
| D2WIN_10171 | Load expansion archives | 🟡 HIGH |

### D2GFX Library Functions (D2GFX_BINARY_ANALYSIS.md)

| Function | Purpose | Priority |
|----------|---------|----------|
| D2GFX_10001 | Destroy window | 🟢 MEDIUM |
| D2GFX_10011 | Set perspective | 🟢 LOW |
| D2GFX_10015 | Low-end graphics | 🟢 LOW |
| D2GFX_10018 | Set gamma | 🟢 LOW |
| D2GFX_10020 | Enable vsync | 🟢 LOW |

## Appendix B: Command-Line Options Reference

From Game/diablo2.cpp (already implemented):

| Option | Section | Purpose |
|--------|---------|---------|
| -w | VIDEO | Windowed mode |
| -3dfx | VIDEO | Use 3dfx Glide |
| -opengl | VIDEO | Use OpenGL |
| -d3d | VIDEO | Use Direct3D |
| -ns | DEBUG | No sound |
| -log | DEBUG | Enable logging |
| -act N | INTERFACE | Start in Act N |
| -diff N | INTERFACE | Difficulty (0-2) |

## Appendix C: Data Structures Reference

From Game/Diablo2.hpp (already defined):

```cpp
// Main configuration structure (0x3C7 bytes)
struct D2GameConfigStrc {
    DWORD dwExpansion;          // +00
    BYTE  bWindowed;            // +04
    BYTE  b3DFX;                // +05
    BYTE  bOpenGL;              // +06
    // ... [see Diablo2.hpp for complete definition]
};

// OpenD2-specific configuration
struct OpenD2ConfigStrc {
    char szBasePath[1024];      // Base game files path
    char szHomePath[1024];      // User data path
    char szModPath[1024];       // Mod path
};
```

## Conclusion

This plan provides a comprehensive roadmap for continuing the OpenD2 reimplementation. The key advantages of the current approach:

1. **Comprehensive Documentation**: 22 detailed binary analysis documents
2. **Partial Implementation**: Core infrastructure already in place
3. **Available Resources**: Pre-built DLLs for testing and reference
4. **Clear Priorities**: Phase-by-phase implementation plan
5. **Fallback Options**: Can use original DLLs for unimplemented components

**Recommended Next Action**: Begin Phase 1 implementation (FOG library) using the existing binary analysis documents as the primary reference. Ghidra MCP integration can be added later for enhanced analysis capabilities.

---

**Document Version**: 1.0
**Last Updated**: November 7, 2025
**Author**: OpenD2 Development Team
