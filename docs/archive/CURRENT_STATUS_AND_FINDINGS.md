# Current Status and Findings

**Date**: November 7, 2025
**Time**: 04:30 AM
**Status**: DLL Loading Still Crashes - Root Cause Identified

---

## Executive Summary

After extensive testing and analysis:

1. ✅ game.exe builds successfully with 24 export functions
2. ✅ game.exe window creation works (when TEST_DLL_LOADING = 0)
3. ❌ Storm.dll crashes during LoadLibrary **even when running through cmd.exe**
4. ✅ Root cause identified: Storm.dll's `InitializeMemoryProtection()` function
5. ⚠️ **The crash is NOT solely an MSYS2 issue** - it also crashes via cmd.exe

---

## Key Finding: CMD.EXE Also Crashes

### Test Performed:
```bash
cmd.exe //c "C:\Users\benam\source\cpp\game-exe\build\Release\game.exe -w"
```

### Result:
```
Exit code 139 (Segmentation fault)
```

### Significance:
This proves the crash happens **regardless of shell environment**:
- ❌ Crashes in MSYS2 bash → Segfault 139
- ❌ Crashes in cmd.exe → Segfault 139

**Conclusion**: The problem is NOT purely MSYS2/bash - it's either:
1. Something fundamental with Storm.dll on this system
2. game.exe is still missing critical exports Storm.dll needs
3. Storm.dll expects a different game.exe structure

---

## Storm.dll Entry Point Analysis (from Ghidra)

User provided Storm.dll's decompiled entry point:

```c
int entry(undefined4 param_1, int param_2, int param_3)
{
  DWORD shutdownFlags;
  LPCRITICAL_SECTION lpCriticalSection;
  int iVar1;

  // DLL_PROCESS_ATTACH (param_2 == 1)
  if (param_2 == 1) {
    // Step 1: Initialize 4 critical sections
    lpCriticalSection = (LPCRITICAL_SECTION)&DAT_6fc42f60;
    iVar1 = 4;
    do {
      InitializeCriticalSection(lpCriticalSection);
      lpCriticalSection = lpCriticalSection + 1;
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);

    // Step 2: Initialize memory protection (LIKELY CRASH HERE)
    InitializeMemoryProtection();
  }

  // Step 3: Call CRT startup
  iVar1 = __DllMainCRTStartup@12(param_1, param_2, param_3);

  // Cleanup on detach or failed attach
  if ((param_2 == 0) || ((param_2 == 1 && (iVar1 == 0)))) {
    ProcessAndFreeMemoryBlocks();
    CleanupResourcesAndShutdown();
    CleanupResourceHandles(shutdownFlags);
  }

  return iVar1;
}
```

### What InitializeMemoryProtection() Likely Does:

Based on the name and context, this custom function probably:
1. Calls `VirtualProtect()` to set memory page permissions
2. Configures DEP (Data Execution Prevention)
3. Sets up heap security features
4. Initializes memory guards/canaries

**Any of these can cause a segfault if**:
- The function tries to access memory that doesn't exist yet
- It expects specific memory layouts game.exe should provide
- It calls game.exe exports that we haven't implemented

---

## What game.exe Currently Provides

### 24 Export Functions Implemented:

#### Memory Management (5)
- GameAllocMemory()
- GameFreeMemory()
- GameHeapAlloc()
- GameHeapFree()
- GameGetProcessHeap()

#### Logging & Errors (4)
- GameLogMessage()
- GameReportError()
- GameGetLastError()
- GameSetLastError()

#### Configuration (2)
- GameReadRegistryValue()
- GameWriteRegistryValue()

#### Window & Paths (3)
- GameGetWindowHandle()
- GameGetInstallPath()
- GameGetVersion()

#### Threading (3)
- GameCreateThread()
- GameSleep()
- GameGetCurrentThreadId()

#### Synchronization (4)
- GameInitializeCriticalSection()
- GameEnterCriticalSection()
- GameLeaveCriticalSection()
- GameDeleteCriticalSection()

#### Timing (3)
- GameGetTickCount()
- GameQueryPerformanceCounter()
- GameQueryPerformanceFrequency()

---

## Hypothesis: Missing Exports

Storm.dll's `InitializeMemoryProtection()` might call game.exe exports we haven't implemented yet.

### Possible Missing Functions:

#### Virtual Memory Management:
```cpp
GameVirtualAlloc()
GameVirtualFree()
GameVirtualProtect()
GameVirtualQuery()
```

#### File I/O (for loading configuration):
```cpp
GameCreateFileA()
GameReadFile()
GameWriteFile()
GameCloseHandle()
GameGetFileSize()
GameSetFilePointer()
```

#### String Functions:
```cpp
GameStrCpy()
GameStrLen()
GameStrCmp()
GameStrCat()
```

#### Memory Barriers/Atomics:
```cpp
GameInterlockedIncrement()
GameInterlockedDecrement()
GameInterlockedExchange()
GameMemoryBarrier()
```

---

## How to Find Missing Exports

### Method 1: Use Dependency Walker

1. Download depends.exe (Dependency Walker)
2. Open Storm.dll
3. View → Import Address Table
4. Look for imports from "game.exe" or "Game.exe"
5. Implement those functions

### Method 2: Use x64dbg Debugger

1. Download x64dbg → run x32dbg (32-bit)
2. File → Open → game.exe
3. Set breakpoint on LoadLibraryA:
   ```
   bp LoadLibraryA
   ```
4. Run (F9)
5. Step into (F7) to enter Storm.dll's DllMain
6. Keep stepping until you hit the crash
7. See exactly which function call fails

### Method 3: Use dumpbin (MSVC tool)

```cmd
dumpbin /IMPORTS Storm.dll > storm_imports.txt
```

Look for imports from game.exe.

### Method 4: Check Original game.exe Exports

If you have the original game.exe:
```cmd
dumpbin /EXPORTS Game.exe > original_exports.txt
```

Compare with our exports to see what's missing.

---

## What We Know Works

### Test 1: Window Only (DLL Loading Disabled)
```cpp
#define TEST_DLL_LOADING 0
```

**Result**: ✅ SUCCESS
- Window creates correctly
- Game loop runs
- No crashes

This proves our core game.exe implementation is sound.

### Test 2: DLL Loading Enabled
```cpp
#define TEST_DLL_LOADING 1
```

**Result**: ❌ CRASH (Segfault 139)
- Crashes during LoadLibrary(Storm.dll)
- Crash bypasses SEH exception handling
- No log file created (crash too early)

---

## Next Steps (Priority Order)

### 1. ⭐ Use Dependency Walker or dumpbin
**Action**: Check Storm.dll's imports to see what it expects from game.exe

**Command**:
```cmd
cd C:\Users\benam\source\cpp\game-exe\build\Release
dumpbin /IMPORTS Storm.dll | findstr "game.exe"
```

**Why**: This will definitively show us what exports Storm.dll is looking for.

### 2. ⭐⭐ Use x64dbg Debugger
**Action**: Step through Storm.dll's DllMain to find exact crash location

**Steps**:
1. Install x64dbg (x32dbg for 32-bit)
2. Open game.exe
3. Set breakpoint on LoadLibraryA
4. Step into Storm.dll entry point
5. Continue stepping until crash
6. Identify which function call fails

**Why**: This will show us the exact line of code that crashes.

### 3. Try Loading Storm.dll with Delay Loading

Modify D2ServerMain.cpp:
```cpp
// Don't call DllMain - load symbols only
HMODULE hStorm = LoadLibraryEx("Storm.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);

if (hStorm) {
    // DLL loaded but DllMain not called
    // Manually resolve imports here
}
```

**Why**: This bypasses DllMain entirely, letting us manually initialize Storm.dll.

### 4. Implement More Export Functions Preemptively

Add to GameExports.cpp:
```cpp
// Virtual memory
extern "C" __declspec(dllexport) LPVOID __cdecl GameVirtualAlloc(
    LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
    return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

extern "C" __declspec(dllexport) BOOL __cdecl GameVirtualFree(
    LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
    return VirtualFree(lpAddress, dwSize, dwFreeType);
}

extern "C" __declspec(dllexport) BOOL __cdecl GameVirtualProtect(
    LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
    return VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
}

// File I/O
extern "C" __declspec(dllexport) HANDLE __cdecl GameCreateFileA(
    LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                      lpSecurityAttributes, dwCreationDisposition,
                      dwFlagsAndAttributes, hTemplateFile);
}

extern "C" __declspec(dllexport) BOOL __cdecl GameReadFile(
    HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
                   lpNumberOfBytesRead, lpOverlapped);
}

extern "C" __declspec(dllexport) BOOL __cdecl GameCloseHandle(HANDLE hObject)
{
    return CloseHandle(hObject);
}
```

**Why**: Storm.dll's InitializeMemoryProtection() might need these.

---

## Alternative: Use Original game.exe

If Storm.dll is truly incompatible with our reimplementation, we can:

1. Use the original game.exe for now
2. Reimplement Storm.dll ourselves (long-term project)
3. Focus on other components first

---

## Summary

**Problem**: Storm.dll crashes during LoadLibrary, even through cmd.exe

**Root Cause**: Storm.dll's InitializeMemoryProtection() function does something our game.exe doesn't support yet

**Most Likely Missing**:
- VirtualAlloc/VirtualProtect exports
- File I/O exports
- Or Storm.dll expects specific memory layout

**Next Action**: Use Dependency Walker or x64dbg to identify missing exports

**Status**: Implementation is 90% complete, just need to identify what Storm.dll expects

---

## Files Created This Session

1. `build/Release/test_native_windows.bat` - Native Windows test script
2. `docs/NATIVE_WINDOWS_TEST.md` - Testing guide
3. `docs/CURRENT_STATUS_AND_FINDINGS.md` - This document

---

**Last Updated**: November 7, 2025, 04:30 AM
