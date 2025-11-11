# Game.exe Reimplementation Strategy Using Ghidra MCP

## Current Status (November 11, 2025)

### ✅ Completed (Level 1-2)
- **CRTStartup** @ 0x0040122e - 12-step Windows PE loader initialization
- **D2ServerMain** @ 0x00408540 - Main game orchestration (stub)
- **Basic Infrastructure**: Window creation, DLL loading framework, message loop
- **Configuration System**: D2Server.ini support, registry fallback, 3-tier priority
- **Debug Logging**: File + debugger + console output to `game.log`

### 🎯 Next Priority (Level 3)

## Implementation Strategy: Call Graph Hierarchy Approach

### Strategy Overview
Follow the **depth-first call hierarchy** starting from `CRTStartup` → `D2ServerMain` → immediate callees, implementing each function with Ghidra MCP decompiled code as reference.

### Key Principles
1. **Depth-First Implementation**: Complete one call path fully before moving to siblings
2. **Stub-Then-Implement**: Create function signatures immediately, implement gradually
3. **Test at Each Level**: Build and test after each function implementation
4. **Reference Decompilation**: Use `mcp_ghidra_get_decompiled_code` for each function
5. **Validate with Disassembly**: Use `mcp_ghidra_get_disassembly` when logic is unclear

---

## Phase 1: Core Initialization Functions (Priority: HIGH)

### Target: InitializeAndRunD2Server @ 0x00408250
**Status**: Partially implemented as `InitializeD2ServerMain`  
**Current Implementation**: Basic registry/INI loading  
**Ghidra Reveals**: 23-step initialization sequence, much more complex than current stub

#### Implementation Plan:
```
InitializeAndRunD2Server (0x00408250)
├─ [1/23] Extract command line argument from argv array ✅ (exists)
├─ [2/23] Format version string with "v%d.%02d" template ⏳ TODO
├─ [3/23] InitializeServerSubsystem ⏳ TODO - Critical!
├─ [4/23] ProcessVersionStringOrdinal10019 ⏳ TODO
├─ [5/23] OpenEventA("DIABLO_II_OK") for launcher sync ⏳ TODO
├─ [6/23] SetEvent to notify launcher ⏳ TODO
├─ [7/23] InitializeCommandLineSettings ⏳ TODO
├─ [8/23] func_0x7b331080 (external D2Common module) ⏳ STUB
├─ [9/23] ExtractModStateKeywordFromCmdLine ⏳ TODO
├─ [10/23] Zero-fill 968-byte video config buffer ✅ (partial)
├─ [11/23] LoadVideoSettingsFromConfigFile ✅ (exists as ReadRegistryConfig)
├─ [12/23] ParseCommandLineIntoConfig ✅ (exists as ParseCommandLine)
├─ [13/23] Validate 4 config bytes at +0x5c,+0x5e,+0x5f,+0x61 ⏳ TODO
├─ [14-21] Registry fallback logic (HKCU → HKLM) ✅ (exists)
└─ [22/23] InitializeAndRunGameMainLoop ⏳ TODO - This is the actual game entry!
```

**Callees to Implement** (from Ghidra analysis):
1. `LoadVideoSettingsFromConfigFile` - Load INI settings (existing, needs enhancement)
2. `InitializeCommandLineSettings` - Command-line parsing (existing, needs validation)
3. `ParseCommandLineIntoConfig` - Override INI with cmdline (exists)
4. `InitializeServerSubsystem` - **CRITICAL** - Initializes core server
5. `ProcessVersionStringOrdinal10019` - Version validation
6. `ExtractModStateKeywordFromCmdLine` - Extract render mode (-d3d, -glide, etc.)
7. `InitializeAndRunGameMainLoop` - **CRITICAL** - Actual game main loop entry

### Action Items:
```bash
# 1. Get full decompilation
mcp_ghidra_get_decompiled_code(function_address="0x00408250")

# 2. Get callees with full analysis
mcp_ghidra_analyze_function_complete(name="InitializeAndRunD2Server")

# 3. For each callee, get decompiled code
mcp_ghidra_get_decompiled_code(function_address="<callee_address>")

# 4. Implement callee by callee, depth-first
```

---

## Phase 2: Service and Security Infrastructure (Priority: MEDIUM)

### Target: InitializeServiceDispatcher @ 0x004084b0
**Purpose**: Windows NT Service support  
**Calls**: `StartServiceCtrlDispatcher`, `RegisterServiceCtrlHandler`

This enables running as Windows Service (e.g., dedicated server mode).

### Target: SetupProcessSecurityRestrictions @ 0x00408120
**Purpose**: Security sandbox and privilege restrictions  
**Calls**: Unknown (need Ghidra analysis)

#### Implementation Plan:
```bash
# Get service dispatcher implementation
mcp_ghidra_get_decompiled_code(function_address="0x004084b0")
mcp_ghidra_analyze_function_complete(name="InitializeServiceDispatcher")

# Get security setup implementation
mcp_ghidra_get_decompiled_code(function_address="0x00408120")
mcp_ghidra_analyze_function_complete(name="SetupProcessSecurityRestrictions")
```

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
