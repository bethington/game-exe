# DLL Integration Notes

## Current Status

**Issue**: game.exe creates a window but doesn't show the Diablo 2 opening screen

## Investigation Results

### What We Discovered

1. **DLLs are Present**: The `build/Release/` folder contains 30 Diablo 2 DLLs including:
   - Fog.dll (358 KB)
   - D2Win.dll (813 KB)
   - D2Gfx.dll (121 KB)
   - D2sound.dll (96 KB)
   - D2Launch.dll (166 KB)
   - D2Client.dll
   - And 24 more...

2. **DLL Loading Crashes**: Attempting to dynamically load these DLLs causes immediate crashes

3. **Possible Reasons**:
   - These DLLs might be from a mod (Project Diablo 2, Path of Diablo, etc.)
   - They expect to be loaded BY an original game.exe, not BE the implementation
   - They have dependencies on each other (D2Win needs Fog, D2Gfx needs Storm, etc.)
   - DllMain initialization might fail without proper context

4. **DLL Dependencies** (from analysis):
   ```
   D2Win.dll    → depends on Fog.dll, Storm.dll
   D2Gfx.dll    → depends on Fog.dll, Storm.dll
   D2sound.dll  → depends on Fog.dll, Storm.dll
   D2Launch.dll → depends on D2Win, D2Gfx, D2sound, Storm, Fog
   ```

## Solutions Attempted

### Attempt 1: Stub Implementations
- Created our own implementations of FOG, D2WIN, D2GFX, D2SOUND
- **Result**: Window opens but no graphics rendering
- **Reason**: Stubs don't actually load MPQs or render graphics

### Attempt 2: Dynamic DLL Loading
- Used LoadLibrary/GetProcAddress to load real DLLs at runtime
- **Result**: Immediate crash before any output
- **Reason**: DLL initialization fails, likely due to missing dependencies or wrong load order

## Recommended Solutions

### Solution 1: Use Original game.exe (Easiest)
**Status**: ⭐ Recommended for playing the game

If you want to actually PLAY Diablo 2:
1. Keep your original `Game.exe` from Diablo 2 installation
2. Use it with these DLLs
3. Our OpenD2 game.exe is for development/reimplementation only

### Solution 2: Load DLLs in Correct Order (Complex)
**Status**: 🔧 For developers

To make OpenD2 game.exe work with original DLLs:

1. **Load Core DLLs First**:
   ```cpp
   LoadLibrary("Storm.dll");    // Must be first
   LoadLibrary("Fog.dll");      // Depends on Storm
   LoadLibrary("D2CMP.dll");    // Compression
   LoadLibrary("D2Win.dll");    // Depends on Fog, Storm
   LoadLibrary("D2Gfx.dll");    // Depends on Fog
   LoadLibrary("D2sound.dll");  // Depends on Fog
   // Then game-specific DLLs
   LoadLibrary("D2Common.dll");
   LoadLibrary("D2Lang.dll");
   LoadLibrary("D2Net.dll");
   LoadLibrary("D2Game.dll");
   LoadLibrary("D2Client.dll");
   LoadLibrary("D2Launch.dll");
   ```

2. **Handle DllMain Failures**: Some DLLs might crash if loaded without proper context

3. **Provide Required Exports**: DLLs might call back into game.exe expecting certain exports

### Solution 3: Reimplementation from Scratch (Long-term)
**Status**: 📝 Current project goal

This is what we're working on:
1. Implement each DLL function based on binary analysis
2. Build complete, working implementations
3. Achieve feature parity with original game

**Progress**:
- ✅ FOG functions stubbed (logging works)
- ✅ D2WIN functions stubbed (window creation works)
- ✅ D2GFX functions stubbed
- ✅ D2SOUND functions stubbed
- ⏳ MPQ loading (needs implementation)
- ⏳ Graphics rendering (needs implementation)
- ⏳ Game logic (massive undertaking)

## What's Working Now

Our current OpenD2 game.exe:
- ✅ Compiles successfully (20 KB)
- ✅ Runs without crashing (when using stub implementations)
- ✅ Creates a game window
- ✅ Initializes logging system
- ✅ Parses command-line arguments
- ✅ Attempts to load D2Launch.dll

What's NOT working:
- ❌ No graphics rendering (need real D2Gfx implementation)
- ❌ No MPQ archive loading (need real Storm/D2Win implementation)
- ❌ No opening screen (need D2Launch.dll working)
- ❌ No actual gameplay (need D2Client/D2Game/D2Common)

## Why You See a Black Window

When you run our game.exe:

1. ✅ WinMain starts
2. ✅ InitializeDllLoader attempts to load DLLs
3. ❌ **CRASHES HERE** - One of the DLLs fails to load or initialize
4. (Never reached) Would show opening screen if DLLs worked

OR if using stub implementations:

1. ✅ WinMain starts
2. ✅ FOG_10021, FOG_10019 create logs
3. ✅ D2WIN_10000 creates black window
4. ✅ D2WIN_10037 reports "archives loaded" (but doesn't actually load them)
5. ❌ **STOPS HERE** - No graphics to render, so window stays black
6. ✅ Shows MessageBox "No modules loaded"
7. ✅ Exits after 3 seconds

## How to Get the Full Game Working

### Option A: Use Original Game
```bash
# In your Diablo 2 installation folder
Game.exe -w    # Original game.exe with all DLLs
```

### Option B: Continue Development
Follow the REIMPLEMENTATION_PLAN.md:

**Next Steps**:
1. Implement Storm.dll functions (MPQ loading)
2. Implement D2Gfx.dll functions (rendering)
3. Integrate these into game.exe
4. Test rendering of opening screen assets
5. Implement D2Launch.dll (menu system)
6. And so on...

**Time Estimate**: Months to years for full implementation

## Technical Details

### DLL Export Format

Diablo 2 DLLs use **ordinal exports**, not names:
- FOG_10019 = Ordinal 10019
- D2WIN_10000 = Ordinal 10000
- etc.

This is why we use:
```cpp
GetProcAddress(hDll, (LPCSTR)10019)  // By ordinal
```

Not:
```cpp
GetProcAddress(hDll, "FOG_10019")    // By name (doesn't work)
```

### Why DLLs Crash

Possible reasons for crashes when loading:

1. **Missing Dependencies**:
   ```
   Fog.dll needs: KERNEL32, USER32, ADVAPI32, WS2_32
   D2Win.dll needs: Fog.dll, Storm.dll, KERNEL32, GDI32
   ```

2. **DllMain Initialization**:
   DLLs execute code in DllMain when loaded. If that code:
   - Accesses invalid memory
   - Calls missing functions
   - Expects certain registry keys
   - Needs specific environment
   ...it will crash

3. **Version Mismatch**:
   These DLLs might be for:
   - Diablo 2 v1.14d
   - Project Diablo 2 Season X
   - Path of Diablo
   - SlashDiablo
   - etc.

   Our game.exe might not match their expectations

## Recommendations

**For Playing Diablo 2**:
→ Use the original Game.exe that came with your DLLs

**For Learning/Development**:
→ Continue with our stub implementations
→ Add features incrementally
→ Reference the binary analysis documents

**For Full Reimplementation**:
→ Follow REIMPLEMENTATION_PLAN.md
→ Implement one subsystem at a time
→ Test with original game data
→ Compare behavior with original game.exe

## Files

- **game.exe** (our build): 20 KB
- **Game.exe** (original): ~70 KB
- Difference: Original has all the initialization code that DLLs expect

## Conclusion

Our OpenD2 game.exe is a **development project** for reimplementing Diablo 2 from scratch. It's not currently compatible with the original DLLs because:

1. DLLs expect specific initialization from original game.exe
2. DLLs have complex interdependencies
3. DLLs require proper loading order
4. Full reimplementation is a massive undertaking

**Current state**: Foundation is solid, window creation works, logging works, but graphics/gameplay requires months more development.

**To play Diablo 2 now**: Use the original Game.exe with these DLLs.

**To contribute to OpenD2**: Help implement the missing functions following REIMPLEMENTATION_PLAN.md!

---

**Last Updated**: November 7, 2025
**Project**: OpenD2 - Phase 1 Complete (Foundation)
**Next Phase**: MPQ Loading and Graphics Rendering
