# Native Windows Testing Guide

**Date**: November 7, 2025
**Purpose**: Test game.exe on native Windows (not MSYS2) to isolate environment issues

---

## Problem Statement

Game.exe crashes when loading Storm.dll, but we're running through **MSYS2/MinGW bash**, not true native Windows.

### Evidence:
```bash
$ uname -a
MINGW64_NT-10.0-26100 Ben-PC 3.4.10-87d57229.x86_64 2024-02-14 20:17 UTC x86_64 Msys
```

This is **NOT** native Windows - it's MSYS2's compatibility layer, similar to Wine.

### Why This Matters:

Storm.dll's DllMain does this on load:
```c
// Storm.dll entry point (from Ghidra)
if (param_2 == 1) {  // DLL_PROCESS_ATTACH
    // Initialize 4 critical sections (safe)
    InitializeCriticalSection(lpCriticalSection);

    // Initialize memory protection (CRASHES HERE)
    InitializeMemoryProtection();  // ← Custom function
}
```

The `InitializeMemoryProtection()` function likely:
- Calls `VirtualProtect()` to set memory permissions
- Configures DEP (Data Execution Prevention)
- Sets up heap security features

**These Windows APIs may not work correctly in MSYS2.**

---

## Testing on Native Windows

### Method 1: Run Batch File from Windows Explorer ⭐ EASIEST

1. Open Windows File Explorer
2. Navigate to: `C:\Users\benam\source\cpp\game-exe\build\Release\`
3. **Double-click** `test_native_windows.bat`
4. A Command Prompt window will open and run the test
5. Check the output and log file

### Method 2: Run from Windows Command Prompt

1. Press `Win+R`, type `cmd`, press Enter
2. Run:
```cmd
cd C:\Users\benam\source\cpp\game-exe\build\Release
test_native_windows.bat
```

### Method 3: Run from PowerShell

1. Press `Win+X`, select "Windows PowerShell"
2. Run:
```powershell
cd C:\Users\benam\source\cpp\game-exe\build\Release
.\test_native_windows.bat
```

### Method 4: Direct Execution

```cmd
cd C:\Users\benam\source\cpp\game-exe\build\Release
game.exe -w
```

---

## Expected Results

### If It Works on Native Windows ✅

**Output:**
```
Starting game.exe with windowed mode (-w)...
============================================================================

[Game window opens]

Game.exe exited with code: 0
============================================================================

Contents of game_dll_loading.log:
============================================================================
ATTEMPTING TO LOAD: Storm.dll
============================================
Calling LoadLibraryA(Storm.dll)...
LoadLibraryA() returned: 0x10000000
SUCCESS: Storm.dll loaded at address 0x10000000
============================================================================
```

**This means:**
- ✅ Our game.exe implementation is CORRECT
- ✅ All 24 export functions are sufficient
- ❌ MSYS2 is the problem (can't handle Storm.dll's memory protection)

**Solution:** Always run game.exe from native Windows, not MSYS2 bash.

### If It Still Crashes on Native Windows ❌

**Output:**
```
Starting game.exe with windowed mode (-w)...
============================================================================

[Crash - no window appears]

Game.exe exited with code: 139
============================================================================

Contents of game_dll_loading.log:
============================================================================
ATTEMPTING TO LOAD: Storm.dll
============================================
Calling LoadLibraryA(Storm.dll)...
============================================================================
```

**This means:**
- Our game.exe is missing something Storm.dll needs
- Need to investigate further with debugger

---

## Comparison: MSYS2 vs Native Windows

### Running in MSYS2 Bash:
```bash
cd /c/Users/benam/source/cpp/game-exe/build/Release
./game.exe -w
```

**Environment:**
- Shell: bash (MSYS2)
- Runtime: MSYS2 compatibility layer
- Windows APIs: Partially emulated
- DLL Loading: May not work correctly
- Memory Protection: May fail

### Running in Native Windows:
```cmd
cd C:\Users\benam\source\cpp\game-exe\build\Release
game.exe -w
```

**Environment:**
- Shell: cmd.exe (native)
- Runtime: Windows native
- Windows APIs: Fully supported
- DLL Loading: Works as designed
- Memory Protection: Fully functional

---

## Why MSYS2 Might Fail

MSYS2 provides a Unix-like environment on Windows, but it's not perfect:

### 1. Memory Management Differences
- MSYS2 uses its own heap allocator
- `VirtualProtect()` may not work correctly
- DEP/NX configuration may fail

### 2. DLL Loading Behavior
- MSYS2's LoadLibrary wrapper may differ
- TEB/PEB structures may be incompatible
- DllMain execution environment differs

### 3. Exception Handling
- SEH (Structured Exception Handling) may not catch all exceptions
- Hard segfaults bypass __try/__except blocks
- Stack unwinding may fail

---

## Debugging on Native Windows

If the batch file test still crashes, use a debugger:

### Option 1: x64dbg (Free)

1. Download: https://x64dbg.com
2. Install x32dbg (32-bit version)
3. Open game.exe in x32dbg
4. Set breakpoint on LoadLibraryA
5. Run and step through Storm.dll's DllMain
6. See exactly where it crashes

### Option 2: WinDbg (Official Microsoft)

1. Download: Windows SDK
2. Run: `windbg -o game.exe -w`
3. Set breakpoint: `bp LoadLibraryA`
4. Continue execution: `g`
5. Step into DllMain: `t` (trace)

### Option 3: Visual Studio Debugger

1. Open Visual Studio
2. File → Open → Project/Solution
3. Select `build/game.sln`
4. Right-click game project → Properties
5. Debugging → Command Arguments: `-w`
6. Press F5 to debug

---

## What We've Implemented

Our game.exe provides **24 export functions** for DLLs:

### Memory Management (5)
- GameAllocMemory()
- GameFreeMemory()
- GameHeapAlloc()
- GameHeapFree()
- GameGetProcessHeap()

### Logging & Errors (4)
- GameLogMessage()
- GameReportError()
- GameGetLastError()
- GameSetLastError()

### Configuration (2)
- GameReadRegistryValue()
- GameWriteRegistryValue()

### Window & Paths (3)
- GameGetWindowHandle()
- GameGetInstallPath()
- GameGetVersion()

### Threading (3)
- GameCreateThread()
- GameSleep()
- GameGetCurrentThreadId()

### Synchronization (4)
- GameInitializeCriticalSection()
- GameEnterCriticalSection()
- GameLeaveCriticalSection()
- GameDeleteCriticalSection()

### Timing (3)
- GameGetTickCount()
- GameQueryPerformanceCounter()
- GameQueryPerformanceFrequency()

**These should be sufficient for Storm.dll** based on Ghidra analysis.

---

## Next Steps Based on Results

### If Native Windows Test Succeeds:

1. ✅ Implementation is COMPLETE
2. ✅ Document that game.exe must run from native Windows
3. ✅ Add note to README about MSYS2 incompatibility
4. ✅ Continue with remaining DLL integration
5. ✅ Test full game functionality

### If Native Windows Test Fails:

1. Use debugger to find exact crash location
2. Check Storm.dll import table for missing functions
3. Implement additional export functions as needed
4. Consider patching Storm.dll to bypass InitializeMemoryProtection()
5. Try LoadLibraryEx with DONT_RESOLVE_DLL_REFERENCES flag

---

## FAQ

**Q: Why did you say "I am currently on a real Windows machine"?**

A: You ARE on a Windows machine, but running commands through MSYS2 bash. MSYS2 adds a compatibility layer that can interfere with native Windows DLL loading.

**Q: How do I know if I'm in MSYS2 or native Windows?**

A: Check your terminal:
- MSYS2: Prompt shows `MINGW64`, can run `uname -a`
- Native: Prompt shows `C:\>`, `uname` doesn't exist

**Q: Can I use MSYS2 for development?**

A: Yes, for building with CMake, editing files, running git. But for TESTING the built game.exe, use native Windows.

**Q: What if I don't have native Windows?**

A: You DO have native Windows (Windows 10/11). Just run the batch file from File Explorer or cmd.exe instead of MSYS2 bash.

---

## Summary

**Current Status:**
- ✅ game.exe implementation complete (24 exports)
- ✅ Builds successfully
- ✅ Window creation works
- ⚠️ DLL loading crashes in MSYS2
- ⏳ Native Windows test pending

**Hypothesis:**
- MSYS2 environment causes Storm.dll's InitializeMemoryProtection() to fail
- Native Windows will work correctly

**Action Required:**
- Run `test_native_windows.bat` from Windows Explorer or cmd.exe
- Report results

---

**Created**: November 7, 2025
**Status**: Ready for Native Windows Testing
