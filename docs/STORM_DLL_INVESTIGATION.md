# Storm.dll Investigation

**Date**: November 7, 2025
**Purpose**: Determine why Storm.dll crashes during LoadLibrary

---

## What We Know:

### 1. Storm.dll Dependencies (from ldd)
```
ntdll.dll       - Windows NT Layer
KERNEL32.DLL    - Windows Kernel
KERNELBASE.dll  - Windows Base APIs
msvcrt.dll      - Microsoft C Runtime
```

**Conclusion**: Storm.dll does NOT import from game.exe. It only uses standard Windows DLLs.

### 2. File Information
```
Type: PE32 executable (DLL) Intel 80386
Architecture: 32-bit x86
Platform: MS Windows
Sections: 5
```

### 3. From Binary Analysis
- 394 KB size
- 1,704 functions
- 200+ exports
- Self-contained (provides services to other DLLs)
- No dependencies on game.exe

---

## Why It's Crashing:

Since Storm.dll doesn't import from game.exe, the crash during LoadLibrary is likely:

### Theory 1: Wine/MSYS Incompatibility (MOST LIKELY)
- Storm.dll's DllMain does something Wine doesn't support
- Possible issues:
  - Creating threads during DllMain
  - Accessing hardware/devices
  - Using undocumented Windows APIs
  - Timing-sensitive operations

### Theory 2: Missing System DLLs
- Storm.dll might dynamically load other DLLs via LoadLibrary
- Those DLLs might be missing

### Theory 3: Architecture Mismatch
- game.exe is 32-bit, Storm.dll is 32-bit
- But Wine might have issues with 32-bit DLL loading

---

## What to Check in Ghidra:

Since you have Storm.dll open in Ghidra, please look for:

### 1. Find DllMain Function
Search for:
- `DllMain`
- `_DllMainCRTStartup`
- `DllEntryPoint`

### 2. Analyze DllMain Code
When you find it, check what it does when `fdwReason == 1` (DLL_PROCESS_ATTACH):

```c
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // Handle to DLL module
    DWORD fdwReason,     // Reason for calling function
    LPVOID lpReserved)   // Reserved
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // ← Check what happens here
            break;
    }
    return TRUE;
}
```

### 3. Look for These Problematic Operations:

**Thread Creation** (not allowed in DllMain):
- `CreateThread()`
- `_beginthread()`
- `_beginthreadex()`

**Synchronization** (can deadlock):
- `LoadLibrary()` (recursive loading)
- `GetProcAddress()` on other DLLs
- Waiting on mutexes/events

**File I/O** (can hang):
- `CreateFile()`
- `ReadFile()`
- `WriteFile()`

**Registry Access** (can be slow):
- `RegOpenKeyEx()`
- `RegQueryValueEx()`

**Memory Operations**:
- Large allocations
- Heap creation
- VirtualAlloc

### 4. Check for Dynamic LoadLibrary Calls

Search for strings in Storm.dll:
```
"D2Common.dll"
"D2Game.dll"
"D2Gfx.dll"
```

If Storm.dll tries to load OTHER DLLs during its DllMain, that could cause crashes.

---

## Ghidra Search Commands:

### Find DllMain:
1. Window → Functions
2. Search for "DllMain" or "CRTStartup"
3. Double-click to decompile

### Find LoadLibrary Calls:
1. Search → For Strings
2. Look for ".dll" strings
3. Find references to those strings
4. Check if they're passed to LoadLibraryA/LoadLibraryW

### Find CreateThread Calls:
1. Search → Program Text
2. Search for "CreateThread"
3. Or search for bytes: `FF 15` (call instruction to import table)

---

## Alternative: Check Storm.dll Exports

If you can't find DllMain easily, check what Storm.dll exports:

1. Window → Symbol Table
2. Filter by "Export"
3. Look for initialization functions like:
   - `SInitialize`
   - `StormInit`
   - `Initialize`

These might be called AFTER DllMain, so DllMain might be minimal.

---

## Quick Test Without Ghidra:

Try loading Storm.dll with a minimal test program:

```cpp
// test.cpp
#include <Windows.h>
#include <stdio.h>

int main() {
    printf("About to load Storm.dll...\n");

    HMODULE h = LoadLibraryA("Storm.dll");

    if (h) {
        printf("SUCCESS: Storm.dll loaded at %p\n", h);
        FreeLibrary(h);
    } else {
        printf("FAILED: Error %d\n", GetLastError());
    }

    return 0;
}
```

Compile and run:
```bash
cl test.cpp
test.exe
```

If this crashes, the problem is Storm.dll's DllMain itself, not our game.exe.

---

## What to Report Back:

Please check in Ghidra and tell me:

1. **Does Storm.dll have a DllMain function?**
   - If yes, what does it do?

2. **Does Storm.dll call LoadLibrary on other DLLs?**
   - If yes, which DLLs?

3. **Does Storm.dll create threads in DllMain?**
   - If yes, that's the problem

4. **Does Storm.dll have initialization exports?**
   - Like `StormInitialize()` or similar?

This will tell us if we need to:
- A) Load DLLs differently (delay-loading)
- B) Call initialization functions manually
- C) Fix the Wine environment
- D) Use native Windows

---

## Next Steps Based on Findings:

### If DllMain creates threads:
→ We need to load DLLs with `DONT_RESOLVE_DLL_REFERENCES` flag and manually resolve imports

### If DllMain calls other DLLs:
→ We need to load dependencies first

### If DllMain is minimal:
→ The crash is a Wine issue, test on native Windows

### If Storm.dll has init exports:
→ We call those manually after loading
