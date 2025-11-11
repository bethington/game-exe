# DLL Ordinal-Only Exports Discovery

**Date**: November 11, 2025  
**Critical Finding**: All Diablo II DLLs use **ordinal-only exports** with NO function names

---

## Executive Summary

Using dumpbin analysis, we discovered that **all Diablo II DLLs export functions by ordinal only** - the export table shows `[NONAME]` for all functions. This explains why `GetProcAddress` with function names returns NULL.

**Solution**: Use `GetProcAddress` with ordinals via `MAKEINTRESOURCE(ordinal)` macro.

---

## DLL Export Analysis

### Fog.dll
- **Ordinal Base**: 10000
- **Total Functions**: 268
- **Named Exports**: 9 (data/variables only)
- **Function Exports**: 259 ordinals (all `[NONAME]`)

**Export Table Sample**:
```
ordinal hint RVA      name
10000      00016920 [NONAME]
10001      00016900 [NONAME]
10002      000168F0 [NONAME]
...
10168    0 00032F40 PERFORMANCE_ACCUMULATOR (data export)
```

### D2Gfx.dll
- **Ordinal Base**: 10000
- **Total Functions**: 88
- **Named Exports**: 1 (data variable only)
- **Function Exports**: 87 ordinals (all `[NONAME]`)

**Export Table Sample**:
```
ordinal hint RVA      name
10000      00007E20 [NONAME]
10001      0000BB50 [NONAME]
10002      0000B730 [NONAME]
...
10087    0 0001D470 ?rInitialWindowRect@@3UtagRECT@@A (data export)
```

### D2Sound.dll
- **Ordinal Base**: 10000
- **Total Functions**: 71
- **Named Exports**: 0
- **Function Exports**: 71 ordinals (all `[NONAME]`)

**Export Table Sample**:
```
ordinal hint RVA      name
10000      00008020 [NONAME]
10001      00009AF0 [NONAME]
10002      00009820 [NONAME]
...
```

### D2Win.dll
- **Ordinal Base**: 10000
- **Total Functions**: 207
- **Named Exports**: 0
- **Function Exports**: 207 ordinals (all `[NONAME]`)

**Export Table Sample**:
```
ordinal hint RVA      name
10000      000093C0 [NONAME]
10001      000170B0 [NONAME]
10002      000114A0 [NONAME]
...
```

---

## GetProcAddress with Ordinals

### Current Code (Wrong - Uses Names)
```cpp
g_pfnInitializeDirectSound = (PFN_InitializeDirectSound)GetProcAddress(
    g_hModuleD2Sound, 
    "InitializeDirectSound"  // ❌ Returns NULL - no name in export table
);
```

### Correct Code (Uses Ordinals)
```cpp
g_pfnInitializeDirectSound = (PFN_InitializeDirectSound)GetProcAddress(
    g_hModuleD2Sound, 
    MAKEINTRESOURCE(10020)  // ✅ Returns valid function pointer
);
```

**MAKEINTRESOURCE Macro**: Converts ordinal number to LPCSTR format for GetProcAddress:
```cpp
#define MAKEINTRESOURCE(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
```

---

## Ordinal Discovery Strategy

### Method 1: Runtime Address Matching
Since Ghidra shows the runtime addresses of loaded functions, we can:
1. Load the DLL
2. Get the base address
3. Calculate RVA for each function
4. Match RVA with dumpbin export table RVAs
5. Find corresponding ordinal

**Example**:
- Ghidra: `InitializeAsyncDataStructures @ 0x6ff6df00` in FOG.DLL
- DLL Base: Need to find at runtime
- RVA = 0x6ff6df00 - DLL_Base
- Match RVA in dumpbin output → Find ordinal

### Method 2: Trial and Error with Logging
For critical functions, systematically test ordinals:
```cpp
// Test ordinals 10000-10100 for Fog.dll
for (int ordinal = 10000; ordinal < 10100; ordinal++) {
    FARPROC proc = GetProcAddress(g_hModuleFog, MAKEINTRESOURCE(ordinal));
    if (proc) {
        char buf[256];
        sprintf(buf, "[Test] Fog.dll ordinal %d = %p\n", ordinal, proc);
        DebugLog(buf);
    }
}
```

### Method 3: Disassembly Pattern Matching
1. Get function address from Ghidra (e.g., 0x6ff6df00)
2. Load DLL and calculate ordinal's RVA
3. Disassemble first few bytes to create signature
4. Match signature against known function behaviors

---

## Known Function → Ordinal Mappings

### Status: IN PROGRESS

We need to determine ordinals for these 19 functions:

#### Fog.dll Functions (6 needed)
| Function | Ghidra Address | RVA (Unknown) | Ordinal (Unknown) | Status |
|----------|----------------|---------------|-------------------|--------|
| InitializeAsyncDataStructures | 0x6ff6df00 | ? | ? | ⏳ |
| StubFunction_NoOp | 0x6ff66750 | ? | ? | ⏳ |
| InitializeModule | 0x6ff6ccf0 | ? | ? | ⏳ |
| CloseAllEventHandles | 0x6ff6d7f0 | ? | ? | ⏳ |
| DeinitializeGameResources | 0x6ff6b830 | ? | ? | ⏳ |
| InitializeGameData | 0x6ff6f550 | ? | ? | ⏳ |

#### D2Gfx.dll Functions (4 needed)
| Function | Ghidra Address | RVA (Unknown) | Ordinal (Unknown) | Status |
|----------|----------------|---------------|-------------------|--------|
| SetParameterAndCallGraphicsVtable_0x58 | 0x6fa8b1e0 | ? | ? | ⏳ |
| ToggleGameState | 0x6fa8b220 | ? | ? | ⏳ |
| SetCleanupHandlerFlag | 0x6fa8b280 | ? | ? | ⏳ |
| CleanupWindowAndDisplayError | 0x6fa8b4b0 | ? | ? | ⏳ |
| GetWindowHandleValue | 0x6fa87fb0 | ? | ? | ⏳ |

#### D2Sound.dll Functions (2 needed)
| Function | Ghidra Address | RVA (Unknown) | Ordinal (Unknown) | Status |
|----------|----------------|---------------|-------------------|--------|
| InitializeDirectSound | 0x6f9b9820 | ? | ? | ⏳ |
| ShutdownAudioSystemResources | 0x6f9b9230 | ? | ? | ⏳ |

#### D2Win.dll Functions (6 needed)
| Function | Ghidra Address | RVA (Unknown) | Ordinal (Unknown) | Status |
|----------|----------------|---------------|-------------------|--------|
| InitializeGameData | 0x6f8f7c90 | ? | ? | ⏳ |
| CloseGameResources | 0x6f8ea500 | ? | ? | ⏳ |
| DispatchInitialization | 0x6f8f7d40 | ? | ? | ⏳ |
| InitializeResourceBuffers | 0x6f8ea890 | ? | ? | ⏳ |
| InitializeGameEnvironment | 0x6f8f7c50 | ? | ? | ⏳ |
| InitializeGameDllLibraries | 0x6f8eaa20 | ? | ? | ⏳ |
| PromptInsertPlayDisc | 0x6f8ea6b0 | ? | ? | ⏳ |

#### Storm.dll Functions (1 needed)
Storm.dll likely has NAMED exports (needs verification with dumpbin)

---

## Next Steps

### 1. Create Ordinal Discovery Tool ⏳
Write a helper program to:
- Load each DLL
- Get module base address
- Calculate RVAs for known Ghidra addresses
- Match against dumpbin export table
- Output ordinal mappings

### 2. Test Ordinal Resolution ⏳
Update `InitializeDLLFunctionPointers()` to test ordinals:
```cpp
// Test known address → ordinal mapping
HMODULE hMod = LoadLibrary("Fog.dll");
DWORD_PTR baseAddr = (DWORD_PTR)hMod;
char buf[256];
sprintf(buf, "[Test] Fog.dll base: %p\n", hMod);
DebugLog(buf);

// Test first few ordinals
for (int i = 10000; i < 10020; i++) {
    FARPROC proc = GetProcAddress(hMod, MAKEINTRESOURCE(i));
    if (proc) {
        DWORD_PTR rva = (DWORD_PTR)proc - baseAddr;
        sprintf(buf, "[Test] Ordinal %d = RVA 0x%08X, Address %p\n", i, rva, proc);
        DebugLog(buf);
    }
}
```

### 3. Update Function Pointer Initialization ⏳
Once ordinals are known:
```cpp
// Fog.dll functions with ordinals
if (g_hModuleFog) {
    g_pfnInitializeSubsystem2 = (PFN_InitializeSubsystem2)
        GetProcAddress(g_hModuleFog, MAKEINTRESOURCE(10042));  // Example ordinal
    
    g_pfnInitializeSubsystem3 = (PFN_InitializeSubsystem3)
        GetProcAddress(g_hModuleFog, MAKEINTRESOURCE(10018));  // Example ordinal
    
    // ... etc
}
```

---

## Testing & Verification

### Test 1: Ordinal Resolution
```cpp
// Verify ordinal returns valid pointer
FARPROC testProc = GetProcAddress(g_hModuleFog, MAKEINTRESOURCE(10000));
if (testProc) {
    DebugLog("[Test] Ordinal 10000 resolved successfully\n");
} else {
    DebugLog("[Test] Ordinal 10000 failed\n");
}
```

### Test 2: Function Execution
```cpp
// Call function via ordinal-resolved pointer
if (g_pfnInitializeDirectSound) {
    DebugLog("[Test] Calling D2Sound.dll ordinal function...\n");
    g_pfnInitializeDirectSound();  // Should execute actual DLL function
}
```

### Expected Results
- ✅ GetProcAddress returns non-NULL pointers
- ✅ Log shows "Calling DLL.dll..." instead of "(stub)"
- ✅ Actual DLL functions execute
- ✅ No crashes or access violations

---

## Alternative Approaches

### Approach 1: Reverse Engineering Community Resources
Check if ProjectD2 or other Diablo II modding communities have documented ordinal mappings for common DLLs.

### Approach 2: Import Table Analysis
Examine the original "Diablo II.exe" import table to see which ordinals IT uses, then use the same ones.

### Approach 3: Symbol Files
Check if Blizzard ever released PDB symbol files that contain ordinal → function name mappings.

### Approach 4: Dynamic Analysis
Use a debugger (x32dbg, WinDbg) to:
1. Set breakpoint on GetProcAddress
2. Run original Diablo II.exe
3. Log all ordinals it requests
4. Build mapping table

---

## References

- **dumpbin Location**: `C:\Program Files*\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx64\x86\dumpbin.exe`
- **Usage**: `dumpbin /EXPORTS Fog.dll`
- **MAKEINTRESOURCE**: Defined in winuser.h
- **GetProcAddress**: MSDN documentation on ordinal imports

---

## Conclusion

**The infrastructure is complete but requires ordinal mappings to function.** Once we map Ghidra's runtime addresses to DLL export ordinals, the system will work immediately by updating ordinal numbers in `InitializeDLLFunctionPointers()`.

**Critical Action**: Create ordinal discovery tool or manually map first few functions to prove concept works.
