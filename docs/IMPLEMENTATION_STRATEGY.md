# Game.exe Reimplementation Strategy - Updated November 11, 2025

## Executive Summary

**Current State**: Game.exe framework complete (15.5 KB) with CRT startup, DLL loading, state machine, and debugging infrastructure.

**Gap Analysis**: The menu screen doesn't appear because **Game.exe doesn't render the menu directly**. The original binary calls into D2Win.dll and D2Client.dll which handle all UI rendering, menu logic, and graphics. Our state handlers are stubs that need to call actual DLL functions.

**Path to Menu Screen**: 
1. Load D2Win.dll, D2Client.dll, D2Gfx.dll successfully (currently failing with error 126)
2. Resolve function pointers from these DLLs (23 functions identified)
3. Call `InitializeMenuSystem()` → D2Win.dll during game loop initialization
4. State handlers must call into D2Client.dll to render menu frames
5. Ensure proper window handle is passed to D2Win/D2Gfx for rendering target

---

## Phase 1: Critical DLL Integration (IMMEDIATE PRIORITY)

### Problem: DLLs Fail to Load
**Error**: `LoadGameDLL WARNING: Failed to load D2Game.dll (error 126)` for all DLLs

**Root Cause Analysis**:
- Error 126 = ERROR_MOD_NOT_FOUND (DLL or its dependencies not found)
- DLLs must be in same directory as game.exe OR in PATH
- DLLs may have dependencies (MSVCRT, DirectX, etc.)

**Solution Steps**:
```bash
# 1. Copy all Diablo II DLLs to build\Release\
copy "C:\Program Files (x86)\Diablo II\*.dll" build\Release\

# 2. Verify DLL dependencies with Dependency Walker
# Download: https://www.dependencywalker.com/
depends.exe build\Release\D2Win.dll

# 3. Test minimal DLL load
Get-Content build\Release\game.log | Select-String "DLL|ordinal"
```

### Function Pointer Resolution Strategy

Based on Ghidra analysis of `InitializeAndRunGameMainLoop` @ 0x00407600, Game.exe calls 23 DLL functions:

**Phase 1.1: Critical Initialization Functions** (6 functions)
```cpp
// Fog.dll ordinals (WORKING - discovered via binary search)
InitializeSubsystem2       // Ordinal 10111 @ 0x004074ba
InitializeSubsystem4       // Ordinal 10096 @ 0x004074a8

// D2Gfx.dll ordinals (WORKING)
InitializeGraphicsSubsystem // Ordinal 10025 @ 0x004074d8

// D2Sound.dll ordinals (WORKING)
InitializeDirectSound       // Ordinal 10022 @ 0x004074ae

// D2Win.dll (NAME-BASED - need ordinal discovery)
InitializeMenuSystem        // @ 0x004074f6
CleanupMenuSystem           // @ 0x004074f0
```

**Phase 1.2: Graphics/Renderer Functions** (5 functions)
```cpp
// D2Gfx.dll (need ordinal discovery)
InitializeRenderer          // @ 0x004074ea
PrepareGraphicsShutdown     // @ 0x004074c0
ShutdownGraphics            // @ 0x0040750e
GetWindowHandle             // @ 0x00407526

// D2Client.dll (need ordinal discovery)
ValidateSystemRequirements  // @ 0x7b331000 (external)
GetDefaultScreenMode        // @ 0x004074e4
```

**Phase 1.3: Peripheral/Settings Functions** (6 functions)
```cpp
// D2Win.dll (need ordinal discovery)
SetFramerateLock            // @ 0x00407508
SetFPSDisplayMode           // @ 0x00407502
ApplyGammaCorrection        // @ 0x00407520
EnableWideAspectRatio       // @ 0x00407514

// D2Sound.dll (need ordinal discovery)
EnableSound                 // @ 0x0040751a

// Storm.dll (need ordinal discovery)
WriteRegistryDwordValue     // @ 0x0040745a
```

**Phase 1.4: Subsystem Management** (6 functions)
```cpp
// Various DLLs (need identification)
InitializeSubsystem3        // @ 0x00407496
CloseEngineSubsystem        // @ 0x004074d2
ShutdownSubsystem6          // @ 0x004074fc
CloseAllEventHandlers       // @ 0x0040749c
ShutdownExternalSubsystem   // @ 0x004074a2
InitializeGameData          // (need address)
```

---

## Phase 2: State Machine Integration (HIGH PRIORITY)

### Current State Machine Implementation
From Main.cpp lines 1160-1220, we have 6 state handler stubs:
- State 0: Exit
- State 1: Menu (creates test window but doesn't call D2Win.dll)
- State 2: Character Select
- State 3: In Game
- State 4: Loading
- State 5: Credits

### Disassembly Analysis @ 0x00407816-0x00407837

**State Handler Table @ 0x0040c964**:
```
Offset  Address    State Handler Function
------  ---------- ----------------------
+0x00   0x0040a5c0 StateHandler0_Exit
+0x04   0x0040a5b0 StateHandler1_Menu
+0x08   0x0040a5a0 StateHandler2_CharSelect
+0x0c   0x0040a594 StateHandler3_InGame
+0x10   0x0040a584 StateHandler4_Loading
+0x14   0x0040a574 StateHandler5_Credits
```

**State Machine Loop Logic** @ 0x004077e4-0x00407837:
```nasm
004077e4: CMP EAX, EBX              ; Test state == 0
004077e6: JZ exit_loop              ; Exit if state 0
004077e8: CMP EAX, 0x2              ; Check if state == 2 (CharSelect)
004077eb: JNZ skip_menu_cleanup     
; [Menu cleanup if transitioning from menu...]
00407816: CMP EAX, EBX              ; Validate state >= 0
00407818: JL force_exit
0040781a: CMP EAX, 0x5              ; Validate state <= 5
0040781d: JG force_exit
0040781f: MOV ESI, [EAX*4 + 0x40c964] ; Load handler from table
00407826: PUSH EDI                  ; Push config pointer
00407827: CALL 0x00407550           ; Call state handler via thunk
0040782c: JMP 0x00407830            
0040782e: XOR EAX, EAX              ; Invalid state → force exit
00407830: CMP EAX, EBX              
00407832: MOV [0x0040cf30], EAX    ; Store new state globally
00407837: JNZ loop_continue         ; Continue if state != 0
```

**Key Insight**: State handlers **MUST RETURN THE NEXT STATE VALUE**. Our current stubs return void - this breaks the state machine!

### Corrected State Handler Signatures
```cpp
// WRONG (current implementation):
void __cdecl StateHandler1_Menu(void *config);

// CORRECT (matches disassembly):
int __cdecl StateHandler1_Menu(void *config);
// Returns: next state (0=exit, 1=menu, 2=charselect, 3=ingame, etc.)
```

### State 1 (Menu) Implementation Requirements

From disassembly @ 0x004077b9-0x004077db:
```nasm
004077b9: MOV AL, [EDI + 0x21c]      ; Load skip_menu flag from config +0x21c
004077bf: TEST AL, AL                ; Test skip flag
004077c1: MOV EAX, [0x0040cf30]      ; Load current state
004077c6: JNZ skip_init              ; Jump if skip_menu==1
004077c8: CMP EAX, 0x2               ; Test state != 2
004077cb: JZ skip_init               ; Skip if already in state 2
004077cd: MOVZX EDX, [EDI + 0x220]   ; Load menu init param from +0x220
004077d4: MOV ECX, [EDI]             ; Load video mode from config +0x0
004077d6: CALL 0x004074f6            ; *** CALL InitializeMenuSystem() ***
004077db: MOV EAX, [0x0040cf30]      ; Reload state
004077e0: MOV [ESP + 0x10], ESI      ; Store menu active flag
```

**Critical Function Call**: `InitializeMenuSystem()` @ 0x004074f6 is called with:
- ECX = video mode (from config +0x0)
- EDX = menu init parameter (from config +0x220)

This function is in **D2Win.dll** and initializes the entire menu UI system.

### State Handler Implementation Template
```cpp
int __cdecl StateHandler1_Menu(void *config)
{
    DEBUG_LOG("[StateHandler1] MENU state - initializing D2Win menu system\n");
    
    // Cast config to access video mode and menu parameters
    LaunchConfig *pConfig = (LaunchConfig *)config;
    
    // Check if menu initialization is needed
    if (!pConfig->skip_menu && g_currentState != 2)
    {
        // Call D2Win.dll::InitializeMenuSystem
        if (g_pfnInitializeMenuSystem)
        {
            g_pfnInitializeMenuSystem(pConfig->video_mode, pConfig->menu_init_param);
            g_menu_system_active = TRUE;
            DEBUG_LOG("[StateHandler1] Menu system initialized\n");
        }
        else
        {
            ERROR_MSGBOX("Menu Init Failed", "D2Win.dll::InitializeMenuSystem not found!");
            return 0; // Exit
        }
    }
    
    // Menu rendering loop (simplified)
    // In reality, D2Client.dll handles frame rendering via callback
    MSG msg;
    while (g_currentState == 1)
    {
        // Process Windows messages
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return 0; // Exit state
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        
        // D2Client.dll updates menu state and may change g_currentState
        // via callback mechanism (not yet implemented)
        
        Sleep(16); // ~60 FPS
    }
    
    // Return next state (set by D2Client.dll or user input)
    return g_currentState;
}
```

---

## Phase 3: Configuration Buffer Structure (MEDIUM PRIORITY)

### LaunchConfig Structure Definition
From disassembly analysis of `InitializeAndRunD2Server` @ 0x00408250 and state machine @ 0x00407600:

```cpp
typedef struct LaunchConfig
{
    // +0x000: Video configuration
    DWORD video_mode;              // +0x0: 0=GDI, 1=D3D, 2=OpenGL, 3=Glide, 4=auto
    
    // +0x004: Window settings
    BOOL windowed_mode;            // +0x4: TRUE=windowed, FALSE=fullscreen
    BOOL wide_aspect_enable;       // +0x5: Enable 16:9/16:10 aspect ratios
    BOOL fullscreen_mode;          // +0x6: TRUE=fullscreen (inverse of windowed)
    
    // +0x009: 3D mode settings
    BOOL use_3d_mode;              // +0x9: TRUE=use 3D acceleration
    BOOL framerate_lock;           // +0xa: TRUE=lock to 25 FPS (original behavior)
    BOOL sound_enable;             // +0xb: TRUE=enable sound
    
    // +0x00c: Display settings
    DWORD fps_display_mode;        // +0xc: 0=off, 1=show FPS counter
    DWORD gamma_correction;        // +0x10: Gamma correction value
    
    // +0x05c: Configuration validation bytes
    BYTE config_valid_0;           // +0x5c: Validation byte 0
    BYTE config_valid_1;           // +0x5e: Validation byte 1
    BYTE config_valid_2;           // +0x5f: Validation byte 2
    BYTE config_valid_3;           // +0x61: Validation byte 3
    
    // +0x200: Extended configuration
    BYTE extended_config_byte;     // +0x200: Unknown config byte
    BYTE menu_config_byte;         // +0x202: Menu configuration flag
    
    // +0x21c: Menu control flags
    BOOL skip_menu;                // +0x21c: TRUE=skip main menu, go to char select
    BYTE menu_init_param;          // +0x220: Menu initialization parameter
    
    // +0x221: Callback interface
    void *callback_interface;      // +0x221: Pointer to callback vtable
                                   //         +0x10 offset = shutdown callback
    
    // Total size: 968 bytes (0x3c8) based on zero-fill loop @ 0x004082f7
    BYTE reserved[968 - sizeof(DWORD)*4 - sizeof(BOOL)*7 - sizeof(BYTE)*5 - sizeof(void*)];
    
} LaunchConfig;
```

**Configuration Initialization** @ 0x004082f7-0x00408309:
```nasm
004082f7: LEA ESI, [ESP + 0x10]      ; Load config buffer address
004082fb: MOV ECX, 0xF2              ; Set counter = 0xF2 (242 DWORDs)
00408300: XOR EAX, EAX               ; Zero EAX
00408302: REP STOSD                  ; Zero-fill 242*4 = 968 bytes
00408304: MOV [ESI], AL              ; Zero final byte
```

---

## Phase 4: Function Discovery Roadmap (LOW PRIORITY)

### Ordinal Discovery Strategy
We've successfully discovered 4 ordinals using binary search. Apply same method to remaining 18 functions:

**Discovery Script Template**:
```python
# Ghidra MCP Script: test_ordinal_range.py
import ghidra_bridge

# Test ordinal range for D2Win.dll
dll_name = "D2Win.dll"
start_ordinal = 10001
end_ordinal = 10050

for ordinal in range(start_ordinal, end_ordinal):
    try:
        # Attempt to get function by ordinal
        func = get_proc_address(dll_handle, ordinal)
        if func:
            print(f"FOUND: {dll_name} ordinal {ordinal} @ {hex(func)}")
    except:
        pass
```

**Priority Order for Ordinal Discovery**:
1. **D2Win.dll** (7 functions) - Menu system, graphics init, peripherals
2. **D2Gfx.dll** (3 functions) - Renderer, graphics shutdown
3. **D2Client.dll** (2 functions) - System validation, screen mode
4. **Fog.dll** (4 functions) - Subsystems 2,3,4, engine close
5. **Storm.dll** (1 function) - Registry write
6. **D2Sound.dll** (1 function) - Enable sound

---

## Implementation Roadmap: Path to Menu Screen

### Milestone 1: DLL Loading Success (1-2 hours)
**Goal**: All DLLs load without error 126

**Tasks**:
- [ ] Copy Diablo II DLLs to `build\Release\`
- [ ] Verify dependencies with Dependency Walker
- [ ] Test DLL loading with enhanced logging
- [ ] Validate DLL exports with dumpbin

**Success Criteria**:
```
[LoadGameDLL] Loading D2Win.dll...
[LoadGameDLL] SUCCESS: D2Win.dll loaded @ 0x12340000
[LoadGameDLL] SUCCESS: D2Gfx.dll loaded @ 0x23450000
[LoadGameDLL] SUCCESS: D2Client.dll loaded @ 0x34560000
```

### Milestone 2: Function Pointer Resolution (2-3 hours)
**Goal**: 23 function pointers resolved and callable

**Tasks**:
- [ ] Implement `InitializeDLLFunctionPointers()` in Main.cpp
- [ ] Test 4 working ordinals (Fog: 10111, 10096; D2Gfx: 10025; D2Sound: 10022)
- [ ] Add fallback to GetProcAddress by name for remaining 18 functions
- [ ] Validate function signatures with test calls

**Success Criteria**:
```
[InitializeDLLFunctionPointers] Resolving 23 function pointers...
[InitializeDLLFunctionPointers] SUCCESS: Fog.dll::10111 → InitializeSubsystem2
[InitializeDLLFunctionPointers] SUCCESS: D2Win.dll::InitializeMenuSystem (by name)
[InitializeDLLFunctionPointers] 23/23 functions resolved successfully
```

### Milestone 3: State Handler Corrections (1 hour)
**Goal**: State handlers return int and integrate with state machine

**Tasks**:
- [ ] Fix all 6 state handler signatures: `void` → `int`
- [ ] Implement state transition logic in each handler
- [ ] Test state machine loop with corrected handlers
- [ ] Validate state transitions (1→2, 2→3, etc.)

**Success Criteria**:
```
[State Machine] Entering state 1 (MENU)
[StateHandler1] Menu system active, returning state 1
[State Machine] State handler returned: 1 (continue)
```

### Milestone 4: InitializeMenuSystem Integration (1-2 hours)
**Goal**: D2Win.dll::InitializeMenuSystem called successfully

**Tasks**:
- [ ] Define LaunchConfig structure matching 968-byte layout
- [ ] Initialize config buffer with proper video mode and flags
- [ ] Call `InitializeMenuSystem(video_mode, menu_param)` in StateHandler1
- [ ] Verify menu system initialized without crash

**Success Criteria**:
```
[StateHandler1] Calling D2Win.dll::InitializeMenuSystem(mode=1, param=0)
[D2Win.dll] Menu system initialized successfully
[StateHandler1] Menu active, entering rendering loop
```

### Milestone 5: Menu Rendering (2-4 hours)
**Goal**: Actual Diablo II main menu visible on screen

**Tasks**:
- [ ] Implement frame rendering callback mechanism
- [ ] Hook D2Client.dll menu update function
- [ ] Pass window handle to D2Gfx for rendering target
- [ ] Process input events and pass to D2Client.dll
- [ ] Handle menu state transitions (menu→char select)

**Success Criteria**:
- Menu background image visible
- Menu buttons clickable
- Music playing (if sound enabled)
- Can navigate to character select screen

---

## Current Blockers and Solutions

### Blocker 1: DLL Load Failures (Error 126)
**Impact**: HIGH - Nothing works without DLLs  
**Solution**: Copy DLLs to build directory, verify dependencies  
**Estimated Fix Time**: 30 minutes

### Blocker 2: State Handlers Return void
**Impact**: MEDIUM - State machine broken but not crashing  
**Solution**: Change return type to int, return next state  
**Estimated Fix Time**: 1 hour

### Blocker 3: InitializeMenuSystem Not Called
**Impact**: HIGH - Menu can't appear without initialization  
**Solution**: Call function in StateHandler1 with proper params  
**Estimated Fix Time**: 2 hours (including config structure setup)

### Blocker 4: Missing Function Pointers (18/23)
**Impact**: MEDIUM - Some features won't work but can fallback  
**Solution**: Use GetProcAddress by name for now, discover ordinals later  
**Estimated Fix Time**: 1 hour for name-based fallback

---

## Testing Strategy

### Unit Tests
```cpp
// Test 1: DLL loading
void Test_LoadAllDLLs()
{
    ASSERT(LoadAllGameDLLs() == TRUE);
    ASSERT(g_hModuleD2Win != NULL);
    ASSERT(g_hModuleD2Gfx != NULL);
}

// Test 2: Function pointer resolution
void Test_ResolveFunctionPointers()
{
    ASSERT(InitializeDLLFunctionPointers() == TRUE);
    ASSERT(g_pfnInitializeMenuSystem != NULL);
    ASSERT(g_pfnInitializeGraphicsSubsystem != NULL);
}

// Test 3: State machine
void Test_StateMachine()
{
    g_currentState = 1;
    int nextState = StateHandler1_Menu(&config);
    ASSERT(nextState >= 0 && nextState <= 5);
}
```

### Integration Tests
1. **Test Run 1**: Load DLLs only, verify no crashes
2. **Test Run 2**: Load + resolve function pointers, verify NULL checks work
3. **Test Run 3**: Load + resolve + call InitializeMenuSystem, verify no crash
4. **Test Run 4**: Full initialization + state machine loop
5. **Test Run 5**: Full run expecting visible menu screen

---

## Success Metrics

### Phase 1 Complete:
- ✅ All 10+ DLLs load successfully (no error 126)
- ✅ 23/23 function pointers resolved (ordinals or by name)
- ✅ No crashes during DLL initialization

### Phase 2 Complete:
- ✅ State handlers return int correctly
- ✅ State machine loop executes without hang
- ✅ Can transition between states (1→2→3)

### Phase 3 Complete:
- ✅ LaunchConfig structure matches 968-byte layout
- ✅ InitializeMenuSystem called without crash
- ✅ D2Win.dll reports successful menu initialization

### Phase 4 Complete (FINAL GOAL):
- ✅ **Main menu screen visible on display**
- ✅ Menu background image rendered
- ✅ Menu buttons visible and interactive
- ✅ Can navigate to character select screen
- ✅ Music and sound effects play

---

## File Organization

### Keep (Core Documentation):
- `COMPLETE_FUNCTION_HIERARCHY.md` - Complete call graph (UPDATE with state machine findings)
- `IMPLEMENTATION_STRATEGY.md` - This file (CURRENT)
- `ORDINAL_DISCOVERY_SUCCESS.md` - Ordinal discovery methodology (REFERENCE)
- `SIZE_OPTIMIZATION_COMPLETE.md` - Size reduction techniques (REFERENCE)

### Archive (Historical Context):
Move to `docs/archive/`:
- `IMPLEMENTATION_STATUS.md` - Outdated status (Nov 7)
- `FINAL_IMPLEMENTATION_STATUS.md` - Duplicate of above
- `FINAL_STATUS.md` - Duplicate
- `BUILD_SUCCESS_REPORT.md` - Outdated build info
- `PROGRESS_SUMMARY.md` - Outdated progress tracking
- `CURRENT_STATUS_AND_FINDINGS.md` - Superseded by this doc

### Remove (Redundant):
- `FINAL_IMPLEMENTATION.md` - Merged into IMPLEMENTATION_STRATEGY.md
- `DLL_LOADING_DEBUG.md` - Info integrated above
- `DLL_INTEGRATION_NOTES.md` - Info integrated above
- `NATIVE_WINDOWS_TEST.md` - Test results, not needed
- `CODE_REVIEW_AND_MISSING_FUNCTIONS.md` - Superseded by this analysis

---

## Next Actions (Priority Order)

1. **IMMEDIATE** (30 min): Copy Diablo II DLLs to `build\Release\` directory
2. **IMMEDIATE** (1 hour): Fix state handler signatures (void → int)
3. **HIGH** (2 hours): Implement `InitializeDLLFunctionPointers()` with name-based fallback
4. **HIGH** (2 hours): Define `LaunchConfig` structure and initialize properly
5. **HIGH** (1 hour): Integrate `InitializeMenuSystem()` call in StateHandler1
6. **MEDIUM** (3 hours): Implement frame rendering callback system
7. **MEDIUM** (2 hours): Test menu display and iterate on rendering issues
8. **LOW** (ongoing): Discover remaining 18 ordinals via binary search

---

**Document Version**: 2.0  
**Last Updated**: November 11, 2025  
**Status**: 🎯 ACTIVE - Clear path to menu screen identified

---

## Phase 3: Critical Subsystem Functions (Priority: HIGH)

Based on high xref count from Ghidra search, these are the most-called utility functions:

### Memory Management (xref_count: 91-16)
1. `_free` @ 0x00403602 (91 xrefs) - Custom free implementation
2. `_malloc` @ 0x004037b2 (16 xrefs) - Custom malloc
3. `AllocateMemoryWithFallback` @ 0x00403a69 (7 xrefs)

### Thread Safety (xref_count: 17)
1. `ReleaseCriticalSectionByLockId` @ 0x00403983 (17 xrefs)
2. `__lock` @ 0x00403a38 (17 xrefs)
3. `GetThreadErrnoAddress` @ 0x00405f24 (17 xrefs)

### String Operations (xref_count: 12-8)
1. `_strlen` @ 0x00403680 (12 xrefs)
2. `StringConcatenate` @ 0x00404680 (8 xrefs)
3. `CopyStringOptimized` @ 0x00404670 (5 xrefs)

### Error Handling (xref_count: 8-7)
1. `ValidateStackCookie` @ 0x00402064 (8 xrefs)
2. `TerminateOnFloatingPointError` @ 0x004063b2 (8 xrefs)
3. `__amsg_exit` @ 0x004011e5 (7 xrefs) - Already implemented ✅

#### Implementation Plan:
These can be implemented in **parallel** as utility functions since they have no dependencies.

```bash
# Batch analyze all utility functions
for func in _free _malloc _strlen StringConcatenate; do
    mcp_ghidra_get_decompiled_code(function_address="<address>")
done
```

---

## Phase 4: Main Game Loop Functions (Priority: CRITICAL)

### Target: InitializeAndRunGameMainLoop (called by InitializeAndRunD2Server)
**Purpose**: The actual game engine entry point  
**Status**: Not yet identified in current code

This is the **most critical missing piece**. Need to find this function:

```bash
# Search for main loop entry
mcp_ghidra_search_functions_enhanced(
    name_pattern=".*MainLoop.*|.*GameLoop.*|.*RunGame.*",
    regex=true,
    min_xrefs=2
)

# Also search for functions called BY InitializeAndRunD2Server
mcp_ghidra_get_function_callees(name="InitializeAndRunD2Server", limit=100)
```

---

## Recommended Implementation Order

### Week 1: Core Configuration and Initialization
1. ✅ **DONE**: CRTStartup (complete)
2. ⏳ **IN PROGRESS**: Enhance `InitializeAndRunD2Server` with all 23 steps
3. ⏳ **NEW**: Implement `InitializeServerSubsystem` (critical dependency)
4. ⏳ **NEW**: Implement `ProcessVersionStringOrdinal10019`
5. ⏳ **NEW**: Implement `ExtractModStateKeywordFromCmdLine`
6. ⏳ **NEW**: Add launcher synchronization (DIABLO_II_OK event)

### Week 2: Utility Functions (Can parallelize)
1. Memory management: `_malloc`, `_free`, `AllocateMemoryWithFallback`
2. String operations: `_strlen`, `StringConcatenate`, `CopyStringOptimized`
3. Thread safety: `__lock`, `ReleaseCriticalSectionByLockId`
4. Error handling: `ValidateStackCookie`, `TerminateOnFloatingPointError`

### Week 3: Main Game Loop
1. Find and identify `InitializeAndRunGameMainLoop`
2. Implement game loop message pump
3. Implement render thread
4. Implement update thread
5. Implement window message handlers

### Week 4: Service and Security
1. `InitializeServiceDispatcher` for NT Service support
2. `SetupProcessSecurityRestrictions` for sandboxing
3. Testing and validation

---

## Workflow for Each Function

### Standard Implementation Procedure:

```bash
# Step 1: Get decompiled code with comments
mcp_ghidra_get_decompiled_code(function_address="0xXXXXXXXX", refresh_cache=false)

# Step 2: Get full function analysis (parameters, locals, callees, callers)
mcp_ghidra_analyze_function_complete(name="FunctionName", 
                                      include_variables=true,
                                      include_callees=true,
                                      include_disasm=false)

# Step 3: If decompilation is unclear, get disassembly
mcp_ghidra_get_disassembly(function_address="0xXXXXXXXX", as_text=true)

# Step 4: Get assembly context for complex operations
mcp_ghidra_get_assembly_context(xref_sources="0xaddr1,0xaddr2", context_instructions=10)

# Step 5: Check function completeness score
mcp_ghidra_analyze_function_completeness(function_address="0xXXXXXXXX")
```

### Implementation Template:

```cpp
/*
 * FunctionName @ 0xXXXXXXXX
 * Original: X:\trunk\Diablo2\Source\Game\<File>.cpp
 * 
 * Algorithm: (from Ghidra decompilation)
 * 1. Step 1 description
 * 2. Step 2 description
 * ...
 * 
 * Parameters:
 *   param1 - Description (type from Ghidra)
 *   param2 - Description (type from Ghidra)
 * 
 * Returns:
 *   Return value description
 * 
 * Callees: (from mcp_ghidra_analyze_function_complete)
 *   - Callee1 @ 0xXXXXXXXX
 *   - Callee2 @ 0xXXXXXXXX
 * 
 * Called by: (from Ghidra analysis)
 *   - Caller1 @ 0xXXXXXXXX
 *   - Caller2 @ 0xXXXXXXXX
 * 
 * Xrefs: XX references
 */
ReturnType __callconv FunctionName(ParamType1 param1, ParamType2 param2)
{
    // Local variables (from Ghidra locals list)
    
    // Implementation matching Ghidra decompilation logic
    
    // Logging for verification
    char logMsg[256];
    sprintf(logMsg, "[FunctionName] Operation completed\n");
    DebugLog(logMsg);
    
    return result;
}
```

---

## Testing Strategy

### Build and Test After Each Function:
```bash
# 1. Build Release
cmake --build build --config Release

# 2. Run and capture log
.\build\Release\game.exe
Get-Content .\build\Release\game.log

# 3. Verify function was called (check log output)
# 4. Verify no crashes or errors
# 5. Check memory usage (should stay ~16MB for simple functions)
```

### Validation Checklist:
- [ ] Function signature matches Ghidra analysis
- [ ] All callees are implemented or stubbed
- [ ] All local variables declared with correct types
- [ ] Algorithm matches decompiled logic step-by-step
- [ ] Debug logging added for verification
- [ ] Build succeeds without warnings
- [ ] Executable runs without crashes
- [ ] Log output shows expected function calls

---

## Advanced Ghidra MCP Techniques

### When Decompilation Fails:
```bash
# 1. Force refresh decompilation cache
mcp_ghidra_get_decompiled_code(function_address="0xXXXXXXXX", refresh_cache=true)

# 2. Fall back to disassembly analysis
mcp_ghidra_get_disassembly(function_address="0xXXXXXXXX", as_text=true)

# 3. Analyze assembly patterns around key instructions
mcp_ghidra_get_assembly_context(
    xref_sources="0xaddr1,0xaddr2",
    context_instructions=15,
    include_patterns="CALL,JMP,MOV,LEA,PUSH"
)

# 4. Check for flow overrides or bad analysis
mcp_ghidra_analyze_function_completeness(function_address="0xXXXXXXXX")
```

### For Data Structures:
```bash
# 1. Analyze structure usage
mcp_ghidra_analyze_struct_field_usage(address="0xXXXXXXXX", max_functions=20)

# 2. Detect array bounds
mcp_ghidra_detect_array_bounds(address="0xXXXXXXXX")

# 3. Get field access context
mcp_ghidra_get_field_access_context(struct_address="0xXXXXXXXX", field_offset=4)
```

---

## Immediate Next Steps (Today)

### 1. Enhance InitializeAndRunD2Server (2-3 hours)
```bash
# Get full implementation details
mcp_ghidra_get_decompiled_code(function_address="0x00408250", refresh_cache=true)

# Get all callees
mcp_ghidra_get_function_callees(name="InitializeAndRunD2Server", limit=50)

# Implement missing 15 steps (out of 23 total)
```

### 2. Find and Stub Critical Missing Functions (1 hour)
```bash
# Find InitializeAndRunGameMainLoop
mcp_ghidra_search_functions_enhanced(
    name_pattern=".*GameMainLoop.*|.*InitializeAndRun.*",
    regex=true
)

# Find InitializeServerSubsystem
mcp_ghidra_search_functions_enhanced(name_pattern=".*ServerSubsystem.*", regex=true)
```

### 3. Create Function Stub File (1 hour)
Create `Game/D2Functions.cpp` with all identified functions as stubs:
```cpp
// All functions from Ghidra with proper signatures
// Implementation status tracked in comments
```

### 4. Update Build and Test (30 minutes)
```bash
cmake --build build --config Release
.\build\Release\game.exe
# Verify new functions are called in game.log
```

---

## Long-Term Goals

### Milestone 1: Complete Core Initialization (Week 1-2)
- All InitializeAndRunD2Server steps implemented
- Configuration system fully functional
- Launcher synchronization working

### Milestone 2: Utility Functions Complete (Week 2-3)
- All high-xref utility functions implemented
- Memory management stable
- Thread safety verified

### Milestone 3: Game Loop Running (Week 3-4)
- InitializeAndRunGameMainLoop implemented
- Message pump functional
- Render and update threads created
- Window responds to messages

### Milestone 4: DLL Integration (Week 4-6)
- D2Common, D2Game, D2Client exports properly called
- Game logic beginning to execute
- Data files loading (MPQ archives)

### Milestone 5: Playable Demo (Week 6-8)
- Character creation screen appears
- Game world renders
- Basic movement works
- Sound/music plays

---

## Questions to Answer with Ghidra MCP

1. **What is InitializeAndRunGameMainLoop's real name?**
   - Search for functions called by InitializeAndRunD2Server
   - Check for "MainLoop", "GameLoop", "RunGame" patterns

2. **What does InitializeServerSubsystem do?**
   - Get decompiled code for this critical function
   - Analyze its callees to understand dependencies

3. **What is func_0x7b331080?**
   - This is in D2Common.dll (external)
   - Need to analyze D2Common exports

4. **What are the 4 validation bytes at config offsets +0x5c,+0x5e,+0x5f,+0x61?**
   - Analyze data structure at that location
   - Check xrefs to understand usage

5. **How does the 968-byte video config buffer work?**
   - Use mcp_ghidra_analyze_data_region
   - Check field access patterns

---

## Resources and References

- **Ghidra Project**: Open in Ghidra GUI for visual analysis
- **Binary**: `Game.exe` @ base address 0x00400000
- **Documentation**: 
  - `docs/GAME_EXE_BINARY_ANALYSIS.md` - Function catalog
  - `docs/COMPLETE_FUNCTION_HIERARCHY.md` - Call hierarchy
  - `docs/IMPLEMENTATION_STATUS.md` - Current progress
- **MCP Server**: GhidraMCP running on localhost
- **Build Output**: `build/Release/game.exe` (15.5 KB)
- **Debug Log**: `build/Release/game.log` (auto-created)

---

## Summary: Best Approach

✅ **Depth-First Call Graph Implementation**

Start from `CRTStartup` ✅ → `D2ServerMain` ✅ → **`InitializeAndRunD2Server`** ⏳

For each function:
1. Use Ghidra MCP to get decompiled code
2. Analyze callees and implement them first (recursively)
3. Implement parent function using child functions
4. Test at each level with build + run + log analysis
5. Move to next sibling function

This ensures **complete, stable, tested implementation** at each level before proceeding deeper.

**Immediate Focus**: Complete `InitializeAndRunD2Server` with all 23 steps, then find and implement `InitializeAndRunGameMainLoop`.
