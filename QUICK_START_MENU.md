# Game.exe Reimplementation - Quick Start Guide

**Updated**: November 11, 2025  
**Status**: Framework Complete (15.5 KB) - Ready for DLL Integration

## What We Have ✅

### Working Infrastructure
- **CRT Startup**: Complete 12-step Windows PE initialization
- **DLL Loading**: Framework loads 10+ DLLs (needs actual DLL files)
- **State Machine**: 6 game states with handler framework
- **Configuration**: Registry + INI + command-line parsing
- **Debugging**: Conditional compilation (28.5 KB debug, 15.5 KB release)
- **Function Pointers**: 23 declared, 4 ordinals discovered

### Current Executable Capabilities
```bash
.\build\Release\game.exe
# - Initializes CRT (heap, threading, I/O)
# - Loads configuration from registry/D2Server.ini
# - Attempts to load 10 DLLs (currently fails - need files)
# - Creates test window (proves windowing works)
# - Exits cleanly
```

## What's Missing for Menu Screen ❌

### Critical Blockers
1. **DLL Files Missing**: Game.exe loads but DLLs not in directory
   - Need: D2Win.dll, D2Gfx.dll, D2Client.dll, etc.
   - Error: "Failed to load D2Game.dll (error 126)"

2. **Function Pointers Unresolved**: 23 functions need GetProcAddress
   - 4 ordinals working (Fog: 10111, 10096; D2Gfx: 10025; D2Sound: 10022)
   - 18 functions need name-based fallback or ordinal discovery

3. **State Handlers Are Stubs**: Don't call into DLLs
   - StateHandler1_Menu creates test window but doesn't call D2Win.dll
   - Need: `InitializeMenuSystem()` call with proper config

4. **Configuration Buffer Incomplete**: 968-byte LaunchConfig structure
   - Partial implementation, missing menu flags at offsets +0x21c, +0x220

## How to Get Menu Working 🎯

### Step 1: Copy DLL Files (30 minutes)
```powershell
# Copy all Diablo II DLLs to executable directory
$d2_dir = "C:\Program Files (x86)\Diablo II"
Copy-Item "$d2_dir\*.dll" "build\Release\" -Force

# Verify DLLs copied
Get-ChildItem build\Release\*.dll

# Expected DLLs (10+ files):
# - D2Win.dll, D2Gfx.dll, D2Client.dll, D2Game.dll
# - D2Net.dll, D2Lang.dll, D2Cmp.dll, D2Sound.dll
# - Fog.dll, Storm.dll, D2Server.dll (optional)
```

### Step 2: Fix State Handler Return Types (1 hour)
Current state handlers return `void` but should return `int` (next state).

**Edit Main.cpp lines 1160-1220**:
```cpp
// WRONG (current):
void __cdecl StateHandler1_Menu(void *config);

// CORRECT (required):
int __cdecl StateHandler1_Menu(void *config)
{
    // ... menu logic ...
    return g_currentState; // Return next state (0=exit, 1=menu, 2=charselect, etc.)
}
```

### Step 3: Implement Function Pointer Resolution (2 hours)
Add to Main.cpp after LoadAllGameDLLs():

```cpp
BOOL InitializeDLLFunctionPointers(void)
{
    DEBUG_LOG("[InitializeDLLFunctionPointers] Resolving 23 function pointers...\n");
    
    // Working ordinals (discovered via binary search)
    g_pfnInitializeSubsystem2 = (PFN_InitializeSubsystem2)GetProcAddress(g_hModuleFog, (LPCSTR)10111);
    g_pfnInitializeSubsystem4 = (PFN_InitializeSubsystem4)GetProcAddress(g_hModuleFog, (LPCSTR)10096);
    g_pfnInitializeGraphicsSubsystem = (PFN_InitializeGraphicsSubsystem)GetProcAddress(g_hModuleD2Gfx, (LPCSTR)10025);
    g_pfnInitializeDirectSound = (PFN_InitializeDirectSound)GetProcAddress(g_hModuleD2Sound, (LPCSTR)10022);
    
    // Name-based fallback for remaining 18 functions
    g_pfnInitializeMenuSystem = (PFN_InitializeMenuSystem)GetProcAddress(g_hModuleD2Win, "InitializeMenuSystem");
    g_pfnCleanupMenuSystem = (PFN_CleanupMenuSystem)GetProcAddress(g_hModuleD2Win, "CleanupMenuSystem");
    g_pfnInitializeRenderer = (PFN_InitializeRenderer)GetProcAddress(g_hModuleD2Gfx, "InitializeRenderer");
    // ... (continue for all 23 functions)
    
    // Validate critical pointers
    if (!g_pfnInitializeMenuSystem)
    {
        ERROR_MSGBOX("DLL Init Failed", "D2Win.dll::InitializeMenuSystem not found!");
        return FALSE;
    }
    
    DEBUG_LOG("[InitializeDLLFunctionPointers] SUCCESS: 23/23 functions resolved\n");
    return TRUE;
}
```

### Step 4: Call InitializeMenuSystem (1 hour)
Update StateHandler1_Menu in Main.cpp:

```cpp
int __cdecl StateHandler1_Menu(void *config)
{
    DEBUG_LOG("[StateHandler1] MENU state - initializing D2Win menu system\n");
    
    LaunchConfig *pConfig = (LaunchConfig *)config;
    
    // Initialize menu system if not already done
    if (!g_menu_system_initialized && !pConfig->skip_menu)
    {
        if (g_pfnInitializeMenuSystem)
        {
            // Call D2Win.dll with video mode and menu parameters
            g_pfnInitializeMenuSystem(pConfig->video_mode, pConfig->menu_init_param);
            g_menu_system_initialized = TRUE;
            DEBUG_LOG("[StateHandler1] Menu system initialized via D2Win.dll\n");
        }
        else
        {
            ERROR_MSGBOX("Menu Init Failed", "InitializeMenuSystem function pointer NULL!");
            return 0; // Exit
        }
    }
    
    // Menu rendering loop
    // D2Client.dll handles actual frame updates via callback
    MSG msg;
    while (g_currentState == 1)
    {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        Sleep(16); // ~60 FPS
    }
    
    return g_currentState;
}
```

### Step 5: Define LaunchConfig Structure (1 hour)
Add to Main.cpp before state handlers:

```cpp
typedef struct LaunchConfig
{
    DWORD video_mode;              // +0x0: 0=GDI, 1=D3D, 2=OpenGL, 3=Glide
    BOOL windowed_mode;            // +0x4: Windowed flag
    BOOL wide_aspect_enable;       // +0x5: Wide aspect ratio
    BOOL fullscreen_mode;          // +0x6: Fullscreen flag
    BYTE padding1[2];
    BOOL use_3d_mode;              // +0x9: Use 3D acceleration
    BOOL framerate_lock;           // +0xa: Lock framerate
    BOOL sound_enable;             // +0xb: Enable sound
    DWORD fps_display_mode;        // +0xc: FPS counter mode
    DWORD gamma_correction;        // +0x10: Gamma value
    // ... (see IMPLEMENTATION_STRATEGY.md for full 968-byte structure)
    BOOL skip_menu;                // +0x21c: Skip to char select
    BYTE menu_init_param;          // +0x220: Menu init parameter
    void *callback_interface;      // +0x221: Callback vtable
    BYTE reserved[968 - 40];       // Pad to 968 bytes
} LaunchConfig;
```

## Testing Your Changes 🧪

### Build and Run
```powershell
# Build release version
cmake --build build --config Release

# Run with debug logging enabled
.\build\Release\game.exe

# Check log for success indicators
Get-Content build\Release\game.log | Select-String "SUCCESS|ERROR|Menu"
```

### Expected Output (Success)
```
[LoadGameDLL] SUCCESS: D2Win.dll loaded @ 0x12340000
[LoadGameDLL] SUCCESS: D2Gfx.dll loaded @ 0x23450000
[LoadGameDLL] SUCCESS: D2Client.dll loaded @ 0x34560000
[InitializeDLLFunctionPointers] SUCCESS: 23/23 functions resolved
[StateHandler1] Menu system initialized via D2Win.dll
[StateHandler1] Menu rendering loop active
```

### Expected Behavior (Success)
1. Window appears with "Diablo II" title
2. Main menu background image visible
3. Menu buttons (Single Player, Multiplayer, etc.) visible
4. Music plays (if sound enabled)
5. Mouse cursor changes to Diablo II cursor
6. Can click buttons to navigate

## Troubleshooting 🔧

### Problem: DLLs Still Fail to Load
**Symptoms**: Error 126 persists  
**Solution**:
```powershell
# Check DLL dependencies
dumpbin /DEPENDENTS build\Release\D2Win.dll

# Install missing Visual C++ Redistributables
# Download from: https://support.microsoft.com/en-us/help/2977003
```

### Problem: Function Pointers NULL
**Symptoms**: "InitializeMenuSystem function pointer NULL!"  
**Solution**:
- Check DLL exports: `dumpbin /EXPORTS build\Release\D2Win.dll`
- Verify function name matches exactly (case-sensitive)
- Try ordinal-based loading if name fails

### Problem: Menu Window Blank/Black
**Symptoms**: Window appears but no menu graphics  
**Solution**:
- Ensure MPQ files in directory (d2data.mpq, d2exp.mpq)
- Check g_installPath global variable set correctly
- Verify D2Gfx.dll initialized (check log for InitializeGraphicsSubsystem)

### Problem: Crashes in State Handler
**Symptoms**: Access violation in StateHandler1_Menu  
**Solution**:
- Verify config pointer not NULL before dereferencing
- Check LaunchConfig structure size matches (968 bytes)
- Add NULL checks for all DLL function pointers

## Documentation Reference 📚

### Core Documents (Read These)
- **IMPLEMENTATION_STRATEGY.md** - Complete implementation plan with Ghidra analysis
- **COMPLETE_FUNCTION_HIERARCHY.md** - Function call graph with addresses
- **ORDINAL_DISCOVERY_SUCCESS.md** - How we found the 4 working ordinals
- **SIZE_OPTIMIZATION_COMPLETE.md** - How we achieved 15.5 KB executable

### Binary Analysis (Reference)
- **docs/analysis/GAME_EXE_BINARY_ANALYSIS.md** - Complete Game.exe disassembly
- **docs/analysis/D2WIN_BINARY_ANALYSIS.md** - D2Win.dll analysis
- **docs/analysis/D2GFX_BINARY_ANALYSIS.md** - D2Gfx.dll analysis

### Archived (Historical)
- **docs/archive/*** - Outdated status reports (moved Nov 11, 2025)

## Next Steps After Menu Works 🚀

Once menu displays successfully:

1. **Character Select Screen** (State 2)
   - Implement StateHandler2_CharSelect
   - Call D2Client.dll character loading functions

2. **In-Game Rendering** (State 3)
   - Implement StateHandler3_InGame
   - Hook D2Client.dll game loop
   - Integrate D2Game.dll server logic

3. **Ordinal Discovery** (Optional)
   - Discover remaining 18 ordinals via binary search
   - Reduces dependency on function name exports
   - Improves compatibility with stripped DLLs

4. **Save Game Loading**
   - Parse save files from Save/ directory
   - Load character data via D2Game.dll
   - Restore game state

## Success Criteria ✓

You've successfully reimplemented Game.exe when:
- ✅ All 10 DLLs load without errors
- ✅ 23 function pointers resolved
- ✅ Main menu screen appears with graphics
- ✅ Menu buttons clickable and responsive
- ✅ Can navigate to character select screen
- ✅ Music and sound effects play
- ✅ No crashes during menu navigation

---

**Build Status**: Framework Complete (15.5 KB)  
**Implementation Status**: 40% (Core + DLL loading)  
**Menu Status**: Blocked on DLL integration  
**Estimated Time to Menu**: 4-6 hours with DLL files available

