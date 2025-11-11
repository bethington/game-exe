# Storm.dll Entry Point Analysis

**Date**: November 7, 2025
**Source**: Ghidra decompilation of Storm.dll entry point

---

## Entry Point Code (Decompiled)

```c
int entry(undefined4 param_1, int param_2, int param_3)
{
  DWORD shutdownFlags;
  LPCRITICAL_SECTION lpCriticalSection;
  int iVar1;

  // DLL_PROCESS_ATTACH (param_2 == 1)
  if (param_2 == 1) {
    lpCriticalSection = (LPCRITICAL_SECTION)&DAT_6fc42f60;
    iVar1 = 4;
    do {
      InitializeCriticalSection(lpCriticalSection);
      lpCriticalSection = lpCriticalSection + 1;
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);
    InitializeMemoryProtection();
  }

  // Call CRT startup
  iVar1 = __DllMainCRTStartup@12(param_1, param_2, param_3);

  // DLL_PROCESS_DETACH or failed attach
  if ((param_2 == 0) || ((param_2 == 1 && (iVar1 == 0)))) {
    ProcessAndFreeMemoryBlocks();
    CleanupResourcesAndShutdown();
    CleanupResourceHandles(shutdownFlags);
  }

  return iVar1;
}
```

---

## Analysis

### Parameters
- `param_1` = `HINSTANCE hinstDLL` (DLL module handle)
- `param_2` = `DWORD fdwReason` (reason code)
  - 0 = DLL_PROCESS_DETACH
  - 1 = DLL_PROCESS_ATTACH  ← This is what executes on load
  - 2 = DLL_THREAD_ATTACH
  - 3 = DLL_THREAD_DETACH
- `param_3` = `LPVOID lpReserved` (reserved)

### What Happens on DLL_PROCESS_ATTACH (param_2 == 1):

#### Step 1: Initialize 4 Critical Sections
```c
lpCriticalSection = (LPCRITICAL_SECTION)&DAT_6fc42f60;
iVar1 = 4;
do {
    InitializeCriticalSection(lpCriticalSection);
    lpCriticalSection = lpCriticalSection + 1;
    iVar1 = iVar1 + -1;
} while (iVar1 != 0);
```

**Purpose**: Initialize 4 CRITICAL_SECTION structures for thread synchronization.

**Memory Location**: `0x6fc42f60` (inside Storm.dll's data section)

**Critical Section Size**: 24 bytes on 32-bit Windows, so:
- CS #1: 0x6fc42f60 - 0x6fc42f77
- CS #2: 0x6fc42f78 - 0x6fc42f8f
- CS #3: 0x6fc42f90 - 0x6fc42fa7
- CS #4: 0x6fc42fa8 - 0x6fc42fbf

**API Used**: `InitializeCriticalSection()` from KERNEL32.dll

**Risk**: This should be safe - it's a standard Windows API call.

#### Step 2: Initialize Memory Protection
```c
InitializeMemoryProtection();
```

**Purpose**: Unknown internal function (not a Windows API)

**Possible Actions**:
- Set up DEP (Data Execution Prevention)
- Configure heap protection
- Set up memory guards
- Initialize allocator structures

**Risk**: ⚠️ HIGH - This is a custom function that could do anything. Likely candidate for crash.

#### Step 3: Call CRT Startup
```c
iVar1 = __DllMainCRTStartup@12(param_1, param_2, param_3);
```

**Purpose**: Initialize C Runtime Library

**Actions**:
- Initialize heap
- Set up exception handlers
- Initialize static C++ objects
- Call constructors for global objects

**Risk**: ⚠️ MEDIUM - CRT startup can fail if environment is not compatible

---

## Why It's Crashing

### Most Likely Cause: `InitializeMemoryProtection()`

This custom function is probably trying to:

1. **Call `VirtualProtect()`** to set memory page permissions
2. **Set up DEP/NX** (Data Execution Prevention / No Execute)
3. **Configure heap flags** for security

Any of these can fail or crash on Wine/MSYS because:
- Wine's memory management differs from Windows
- Wine might not fully support `VirtualProtect()`
- DEP/NX might not be properly emulated

### Secondary Cause: `__DllMainCRTStartup@12`

The CRT startup might:
- Try to allocate from a heap that doesn't exist yet
- Initialize global objects that crash
- Call Windows APIs that Wine doesn't support

---

## What We Know:

### ✅ Things That Should Work:
1. **InitializeCriticalSection()** - Standard Windows API, well-supported
2. **Memory allocation** - Storm.dll has its own heap management
3. **DLL loading mechanism** - Our game.exe correctly calls LoadLibrary

### ❌ Things That Probably Don't Work on Wine:
1. **InitializeMemoryProtection()** - Custom memory protection setup
2. **DEP/NX configuration** - Wine's memory model differs
3. **Heap security features** - Wine may not support all heap flags

---

## Solutions

### Solution 1: Test on Native Windows ⭐⭐⭐ HIGHEST PRIORITY

Copy `build/Release/` to a Windows 10/11 machine and run:
```cmd
game.exe -w
```

If it works on native Windows, problem is confirmed as Wine compatibility.

### Solution 2: Patch Storm.dll (Advanced)

Use Ghidra to:
1. Find `InitializeMemoryProtection()` function
2. Replace its body with `return;` (NOP it out)
3. Save patched Storm.dll

This would skip the problematic initialization.

### Solution 3: Hook/Override the Function

Create a wrapper DLL that:
1. Exports all Storm.dll functions
2. Forwards calls to real Storm.dll
3. Intercepts `InitializeMemoryProtection()` and makes it a no-op

### Solution 4: Use Different Loading Method

Instead of `LoadLibrary()`, use:
```c
HMODULE h = LoadLibraryEx("Storm.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
```

This loads the DLL but doesn't call DllMain. We then manually resolve imports.

---

## Next Steps

### Immediate Test:
Run the newly built game.exe with diagnostics:
```bash
cd build/Release
./game.exe -w 2>&1 | tee output.log
```

The diagnostic message boxes will tell us:
1. Whether LoadLibrary is called
2. Whether it reaches the try block
3. Whether it crashes with an exception code

### If It Still Crashes on Wine:

The crash is definitely `InitializeMemoryProtection()` or CRT startup failing on Wine.

**Recommendation**: Test on native Windows to confirm the implementation is correct.

### If You Want to Debug Further in Ghidra:

1. Find `InitializeMemoryProtection()` function:
   - Search → For Strings → "Memory" or "Protection"
   - Or follow the call from entry point

2. Decompile it and see what it does:
   - Does it call `VirtualProtect()`?
   - Does it call `HeapSetInformation()`?
   - Does it modify memory page permissions?

---

## Expected Behavior (Native Windows)

On native Windows, Storm.dll's entry should:
1. ✅ Initialize 4 critical sections (< 1ms)
2. ✅ Call InitializeMemoryProtection() (< 1ms)
3. ✅ Call __DllMainCRTStartup@12 (< 5ms)
4. ✅ Return TRUE
5. ✅ LoadLibrary() returns valid HMODULE

**Total time**: < 10ms

---

## Conclusion

**Finding**: Storm.dll's DllMain is relatively simple - it initializes critical sections and memory protection.

**Problem**: The `InitializeMemoryProtection()` custom function likely does operations that Wine/MSYS doesn't support.

**Solution**: Test on native Windows to verify our implementation is correct.

**If it works on Windows**: Problem is Wine, not our code. ✅
**If it still crashes on Windows**: Need to investigate what InitializeMemoryProtection expects.

---

## Code Changes Made

Modified `D2ServerMain.cpp` to:
- Load DLLs BEFORE creating window (in case window was the issue)
- Added extensive diagnostic logging
- Added message boxes to show exactly where crash occurs

**Build Status**: ✅ Compiled successfully
**Next Action**: Test on native Windows
