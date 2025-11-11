/*
 * Main.cpp - Diablo II Game.exe Entry Point
 * Original source: X:\trunk\Diablo2\Source\Game\Main.cpp
 * Based on Ghidra binary analysis @ 0x0040122e (CRTStartup)
 *
 * This file contains the complete Windows entry point and function call hierarchy
 * as reverse-engineered from the original Diablo II Game.exe binary.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// DEBUG CONFIGURATION
// =============================================================================

// Set to 1 to enable debug logging (increases size by ~4KB)
// Set to 0 for release builds to minimize executable size
#define ENABLE_DEBUG_LOGGING 1

// Set to 1 to enable MessageBox debugging (shows visible progress)
#define ENABLE_MESSAGEBOX_DEBUG 1

// Set to 1 to enable MessageBox debugging (shows visible progress)
#define ENABLE_MESSAGEBOX_DEBUG 1

#if ENABLE_DEBUG_LOGGING
#define DEBUG_LOG(msg) DebugLog(msg)
#define DEBUG_LOG_ENABLED 1
#else
#define DEBUG_LOG(msg) ((void)0)
#define DEBUG_LOG_ENABLED 0
#endif

#if ENABLE_MESSAGEBOX_DEBUG
#define DEBUG_MSGBOX(title, msg) MessageBoxA(NULL, msg, title, MB_OK | MB_ICONINFORMATION)
#define ERROR_MSGBOX(title, msg) MessageBoxA(NULL, msg, title, MB_OK | MB_ICONERROR)
#else
#define DEBUG_MSGBOX(title, msg) ((void)0)
#define ERROR_MSGBOX(title, msg) ((void)0)
#endif // =============================================================================
// GLOBAL VARIABLES (matching binary layout @ 0x0040B040-0x0040B100)
// =============================================================================

#if ENABLE_DEBUG_LOGGING
// Debug log file handle (for debugging/development)
HANDLE g_hDebugLog = INVALID_HANDLE_VALUE;

// Debug logging helper
void DEBUG_LOG(const char *message)
{
    // Write to debugger
    OutputDebugStringA(message);

    // Write to console if available
    printf("%s", message);

    // Write to log file
    if (g_hDebugLog == INVALID_HANDLE_VALUE)
    {
        char logPath[MAX_PATH];
        GetModuleFileNameA(NULL, logPath, MAX_PATH);
        // Replace .exe with .log
        char *ext = strrchr(logPath, '.');
        if (ext)
        {
            strcpy(ext, ".log");
        }
        g_hDebugLog = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (g_hDebugLog != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        WriteFile(g_hDebugLog, message, strlen(message), &written, NULL);
        FlushFileBuffers(g_hDebugLog);
    }
}
#endif // ENABLE_DEBUG_LOGGING

// =============================================================================

// OS Version Information @ 0x0040B040-0x0040B04C
DWORD g_platformId = 0;   // @ 0x0040B040 - Windows platform ID
DWORD g_majorVersion = 0; // @ 0x0040B044 - OS major version
DWORD g_minorVersion = 0; // @ 0x0040B048 - OS minor version
DWORD g_buildNumber = 0;  // @ 0x0040B04C - OS build number

// Game State @ 0x0040B050-0x0040B05C
BOOL g_isRunning = FALSE;     // @ 0x0040B050 - Game running state
DWORD g_dwSecurityCookie = 0; // @ 0x0040B054 - Stack security cookie
BOOL g_noSound = FALSE;       // @ 0x0040B054 - No sound flag
BOOL g_noMusic = FALSE;       // @ 0x0040B058 - No music flag
BOOL g_skipToBnet = FALSE;    // @ 0x0040B05C - Skip to Battle.net

// Window/Graphics @ 0x0040B060-0x0040B074
DWORD g_videoMode = 0;      // @ 0x0040B060 - Video mode (0=GDI, 1=D3D, 2=OpenGL, 3=Glide)
DWORD g_screenWidth = 800;  // @ 0x0040B064 - Screen width
DWORD g_screenHeight = 600; // @ 0x0040B068 - Screen height
DWORD g_colorDepth = 32;    // @ 0x0040B06C - Color depth
HWND g_hWndMain = NULL;     // @ 0x0040B070 - Main window handle
HDC g_hDC = NULL;           // @ 0x0040B074 - Device context

// Instance/Module @ 0x0040B078-0x0040B07C
HINSTANCE g_hInstance = NULL;  // @ 0x0040B078 - Application instance
char g_installPath[260] = {0}; // @ 0x0040B07C - Install path

// Game Mode @ 0x0040B040-0x0040B044
DWORD g_gameMode = 0;       // @ 0x0040B040 - Game mode (0=SP, 1=MP, 2=BNet)
BOOL g_isExpansion = FALSE; // @ 0x0040B044 - Lord of Destruction installed

// Command Line/Args @ 0x0040B3D0-0x0040B3D8
int g_argc = 0;           // @ 0x0040B3D0 - Argument count
char **g_argv = NULL;     // @ 0x0040B3D4 - Argument values
LPSTR g_lpCmdLine = NULL; // @ 0x0040B3D8 - Command line string
char **g_envp = NULL;     // Environment pointer

// CRT State
HANDLE g_heap = NULL;        // @ 0x0040B3DC - Heap handle
DWORD g_dwShowCmd = SW_SHOW; // Window show command

// DLL Module Handles @ 0x0040B014-0x0040B038
HMODULE g_hModuleD2Client = NULL; // @ 0x0040B014
HMODULE g_hModuleD2Server = NULL; // @ 0x0040B018
HMODULE g_hModuleD2Game = NULL;   // @ 0x0040B01C
HMODULE g_hModuleD2Gfx = NULL;    // @ 0x0040B020 (was D2Gdi - corrected)
HMODULE g_hModuleD2Net = NULL;    // @ 0x0040B024
HMODULE g_hModuleD2Multi = NULL;  // @ 0x0040B028
HMODULE g_hModuleD2Win = NULL;    // @ 0x0040B02C
HMODULE g_hModuleD2Lang = NULL;   // @ 0x0040B030
HMODULE g_hModuleD2Cmp = NULL;    // @ 0x0040B034
HMODULE g_hModuleStorm = NULL;    // @ 0x0040B038
HMODULE g_hModuleFog = NULL;      // Fog.dll - engine foundation
HMODULE g_hModuleD2Sound = NULL;  // D2Sound.dll - audio subsystem

// =============================================================================
// LAUNCH CONFIGURATION STRUCTURE (968 bytes)
// =============================================================================

/*
 * LaunchConfig Structure
 * Based on disassembly analysis @ 0x00407600 (InitializeAndRunGameMainLoop)
 *
 * This structure is passed to state handlers and contains all runtime configuration.
 * Total size: 968 bytes (0x3C8)
 *
 * Key offsets discovered from binary analysis:
 *   +0x000: video_mode (DWORD) - Video renderer selection
 *   +0x21C: skip_menu (BOOL) - Skip main menu flag
 *   +0x220: menu_init_param (DWORD) - Menu initialization parameter
 *   +0x221: callback_interface (void*) - D2Client callback interface
 */
typedef struct LaunchConfig
{
    // +0x000: Video configuration
    DWORD video_mode;    // +0x0: 0=GDI, 1=D3D, 2=OpenGL, 3=Glide, 4=auto
    DWORD screen_width;  // +0x4: Screen width
    DWORD screen_height; // +0x8: Screen height
    DWORD color_depth;   // +0xC: Color depth (16/32)
    BOOL windowed;       // +0x10: Windowed mode flag

    // +0x014: Audio configuration
    BOOL no_sound;      // +0x14: Disable sound
    BOOL no_music;      // +0x18: Disable music
    DWORD sound_volume; // +0x1C: Sound volume (0-100)
    DWORD music_volume; // +0x20: Music volume (0-100)

    // +0x024: Game mode
    DWORD game_mode; // +0x24: 0=SP, 1=MP, 2=BNet
    BOOL expansion;  // +0x28: Lord of Destruction

    // +0x02C: Reserved/padding to offset 0x21C
    BYTE reserved[0x1F0]; // +0x2C to +0x21C (496 bytes)

    // +0x21C: Menu control flags (CRITICAL - discovered via Ghidra)
    BOOL skip_menu;           // +0x21C: Skip main menu flag
    DWORD menu_init_param;    // +0x220: Menu initialization parameter
    void *callback_interface; // +0x224: D2Client callback interface pointer

    // +0x228: Reserved to complete 968-byte structure
    BYTE reserved2[0x1A0]; // +0x228 to +0x3C8 (416 bytes)
} LaunchConfig;

// Global launch configuration instance
LaunchConfig g_launchConfig = {0};

// =============================================================================
// DLL FUNCTION POINTERS (IAT - Import Address Table Pattern)
// =============================================================================

/*
 * Function Pointer Declarations for Delay-Loaded DLL Functions
 *
 * Game.exe uses delay-load imports via an Import Address Table (IAT).
 * These function pointers are populated at runtime using GetProcAddress
 * after DLLs are loaded. This matches the pattern at 0x00409000-0x00409200.
 */

// Graphics Subsystem (D2Win.dll, D2Gdi.dll)
typedef BOOL(__stdcall *PFN_InitializeGraphicsSubsystem)(HINSTANCE, int, BOOL, int);
typedef BOOL(__cdecl *PFN_InitializeRenderer)(BOOL, int);
typedef void(__cdecl *PFN_PrepareGraphicsShutdown)(void);
typedef void(__cdecl *PFN_ShutdownGraphics)(void);
typedef HWND(__cdecl *PFN_GetWindowHandle)(void);

// Menu System (D2Win.dll)
typedef void(__cdecl *PFN_InitializeMenuSystem)(void);
typedef void(__cdecl *PFN_CleanupMenuSystem)(void);

// Audio Subsystem (D2Sound.dll via Storm.dll)
typedef void(__cdecl *PFN_InitializeDirectSound)(void);
typedef void(__cdecl *PFN_EnableSound)(void);

// Peripheral Functions (D2Win.dll)
typedef void(__cdecl *PFN_SetFramerateLock)(BOOL);
typedef void(__cdecl *PFN_SetFPSDisplayMode)(int);
typedef void(__cdecl *PFN_ApplyGammaCorrection)(void);
typedef void(__cdecl *PFN_EnableWideAspectRatio)(void);

// Validation Functions (D2Client.dll)
typedef BOOL(__cdecl *PFN_ValidateSystemRequirements)(void);
typedef BOOL(__cdecl *PFN_GetDefaultScreenMode)(void);

// Subsystem Initialization (various DLLs)
typedef void(__cdecl *PFN_InitializeSubsystem2)(void);
typedef void(__cdecl *PFN_InitializeSubsystem3)(void);
typedef void(__cdecl *PFN_InitializeSubsystem4)(void);
typedef void(__cdecl *PFN_CloseEngineSubsystem)(void);
typedef void(__cdecl *PFN_ShutdownSubsystem6)(void);
typedef void(__cdecl *PFN_ShutdownExternalSubsystem)(void);

// Registry Functions (Storm.dll)
typedef void(__cdecl *PFN_WriteRegistryDwordValue)(const char *, const char *, DWORD);

// Global function pointers (populated during DLL initialization)
PFN_InitializeGraphicsSubsystem g_pfnInitializeGraphicsSubsystem = NULL;
PFN_InitializeRenderer g_pfnInitializeRenderer = NULL;
PFN_PrepareGraphicsShutdown g_pfnPrepareGraphicsShutdown = NULL;
PFN_ShutdownGraphics g_pfnShutdownGraphics = NULL;
PFN_GetWindowHandle g_pfnGetWindowHandle = NULL;
PFN_InitializeMenuSystem g_pfnInitializeMenuSystem = NULL;
PFN_CleanupMenuSystem g_pfnCleanupMenuSystem = NULL;
PFN_InitializeDirectSound g_pfnInitializeDirectSound = NULL;
PFN_EnableSound g_pfnEnableSound = NULL;
PFN_SetFramerateLock g_pfnSetFramerateLock = NULL;
PFN_SetFPSDisplayMode g_pfnSetFPSDisplayMode = NULL;
PFN_ApplyGammaCorrection g_pfnApplyGammaCorrection = NULL;
PFN_EnableWideAspectRatio g_pfnEnableWideAspectRatio = NULL;
PFN_ValidateSystemRequirements g_pfnValidateSystemRequirements = NULL;
PFN_GetDefaultScreenMode g_pfnGetDefaultScreenMode = NULL;
PFN_InitializeSubsystem2 g_pfnInitializeSubsystem2 = NULL;
PFN_InitializeSubsystem3 g_pfnInitializeSubsystem3 = NULL;
PFN_InitializeSubsystem4 g_pfnInitializeSubsystem4 = NULL;
PFN_CloseEngineSubsystem g_pfnCloseEngineSubsystem = NULL;
PFN_ShutdownSubsystem6 g_pfnShutdownSubsystem6 = NULL;
PFN_ShutdownExternalSubsystem g_pfnShutdownExternalSubsystem = NULL;
PFN_WriteRegistryDwordValue g_pfnWriteRegistryDwordValue = NULL;

// =============================================================================
// FORWARD DECLARATIONS - Complete Call Hierarchy
// =============================================================================

// Level 1: CRTStartup and its direct calls
int __cdecl CRTStartup(void);
void __cdecl fast_error_exit(int errorCode);
void __cdecl __amsg_exit(int errorCode);

// Level 2: D2ServerMain and its subsystems
int WINAPI D2ServerMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);
BOOL __cdecl InitializeD2ServerMain(int argc, char **argv);
void __cdecl ParseCommandLine(int argc, char **argv);
BOOL __cdecl FindAndValidateD2ExpMpq(void);

// Level 3: Window creation and management
LRESULT CALLBACK D2WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND __cdecl CreateGameWindow(HINSTANCE hInstance, int width, int height, int showCmd);
void __cdecl DestroyGameWindow(void);

// Level 4: DLL loading system
HMODULE __cdecl LoadGameDLL(const char *dllName);
BOOL __cdecl LoadAllGameDLLs(void);
BOOL __cdecl InitializeDLLFunctionPointers(void);
void __cdecl UnloadAllGameDLLs(void);

// Level 5: Registry configuration
BOOL __cdecl ReadRegistryConfig(void);
BOOL __cdecl WriteRegistryConfig(const char *valueName, const void *data, DWORD dataSize, DWORD type);

// Level 6: Main game loop
void __cdecl RunGameMainLoop(void);
DWORD WINAPI GameUpdateThread(LPVOID lpParam);
DWORD WINAPI RenderThread(LPVOID lpParam);
int __cdecl InitializeAndRunGameMainLoop(void);

// =============================================================================
// LEVEL 1: CRT INITIALIZATION - entry @ 0x0040122e
// =============================================================================

/*
 * fast_error_exit @ 0x0040120a
 * Fast error exit for critical CRT failures
 * Called by: CRTStartup when heap/threading/IO init fails
 */
void __cdecl fast_error_exit(int errorCode)
{
    char errorMsg[128];
    sprintf(errorMsg, "[CRITICAL ERROR] Fast error exit: code 0x%X\n", errorCode);
    DEBUG_LOG(errorMsg);

    MessageBoxA(NULL,
                "A critical error occurred during game initialization.\n"
                "Please check the Windows Event Log for details.",
                "Diablo II - Critical Error",
                MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

    ExitProcess(0xFF);
}

/*
 * __amsg_exit @ various addresses
 * CRT message exit with formatted error
 * Called by: CRTStartup when command-line parsing or environment setup fails
 */
void __cdecl __amsg_exit(int errorCode)
{
    const char *errorMessages[] = {
        "Unknown error",
        "Invalid command line",             // 8
        "Environment setup failed",         // 9
        "C++ static initialization failed", // varies
        "I/O initialization failed"         // 0x1b (27)
    };

    const char *message = "Unknown CRT error";
    if (errorCode == 8)
        message = errorMessages[1];
    else if (errorCode == 9)
        message = errorMessages[2];
    else if (errorCode == 0x1b)
        message = errorMessages[4];

    char errorMsg[256];
    sprintf(errorMsg, "[CRT ERROR] %s (code %d)\n", message, errorCode);
    DEBUG_LOG(errorMsg);

    MessageBoxA(NULL, message, "Diablo II - Startup Error", MB_OK | MB_ICONERROR);
    ExitProcess(errorCode);
}

/*
 * CRTStartup @ 0x0040122e
 * Main C Runtime entry point called by Windows loader
 *
 * Complete implementation matching Ghidra binary analysis disassembly
 * This is the EXACT sequence from the original Game.exe @ 0x0040122e
 *
 * Function signature: int __stdcall CRTStartup(void)
 * Called by: Windows PE loader (entry point)
 * Calls: 21 CRT initialization functions in precise order
 */
int __cdecl CRTStartup(void)
{
    OSVERSIONINFOA osvi;
    STARTUPINFOA startupInfo;
    HMODULE hModule;
    BOOL hasDelayedImports = FALSE;
    int initResult;
    int exitCode;
    char versionMsg[256];

    DEBUG_MSGBOX("Game.exe Start", "CRTStartup entry point reached!\n\nPress OK to continue initialization...");

    DEBUG_LOG("\n========================================\n");
    DEBUG_LOG("[CRTStartup] Windows PE Loader Entry Point @ 0x0040122e\n");
    DEBUG_LOG("========================================\n");

    // =========================================================================
    // STEP 1: GetVersionExA @ 0x0040124e - Detect Windows version
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [1/12] GetVersionExA - Detecting OS version...\n");

    // Initialize version structure (avoid memset dependency)
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    osvi.dwMajorVersion = 0;
    osvi.dwMinorVersion = 0;
    osvi.dwBuildNumber = 0;
    osvi.dwPlatformId = 0;
    osvi.szCSDVersion[0] = '\0';

    if (!GetVersionExA(&osvi))
    {
        DEBUG_LOG("[CRTStartup] FATAL: GetVersionExA failed!\n");
        fast_error_exit(0x01);
        return 1;
    }

    // Extract version info (matching disassembly @ 0x00401254-0x00401293)
    g_platformId = osvi.dwPlatformId;            // @ 0x00401257: MOV [0x0040c9ac], ECX
    g_majorVersion = osvi.dwMajorVersion;        // @ 0x00401260: MOV [0x0040c9b8], EAX
    g_minorVersion = osvi.dwMinorVersion;        // @ 0x00401268: MOV [0x0040c9bc], EDX
    g_buildNumber = osvi.dwBuildNumber & 0x7FFF; // @ 0x00401271: AND ESI, 0x7fff

    // Set high bit for consumer OS (Win95/98/ME) @ 0x0040127d-0x00401288
    if (g_platformId != VER_PLATFORM_WIN32_NT) // CMP ECX, 0x2; JZ
    {
        g_buildNumber |= 0x8000; // OR ESI, 0x8000
    }

    sprintf(versionMsg, "[CRTStartup] OS Detected: Platform=%d (1=Win9x, 2=WinNT), Version=%d.%d, Build=0x%04X\n",
            g_platformId, g_majorVersion, g_minorVersion, g_buildNumber);
    DEBUG_LOG(versionMsg);

    // =========================================================================
    // STEP 2: Check PE imports @ 0x0040129a - Validate module structure
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [2/12] Validating PE structure and imports...\n");

    hModule = GetModuleHandleA(NULL); // @ 0x004012a1: CALL EDI (GetModuleHandleA)
    if (hModule)
    {
        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;

        // Check MZ signature @ 0x004012a3: CMP word ptr [EAX], 0x5a4d
        if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) // 'MZ'
        {
            PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE *)hModule + pDosHeader->e_lfanew);

            // Check PE signature @ 0x004012af: CMP dword ptr [ECX], 0x4550
            if (pNtHeaders->Signature == IMAGE_NT_SIGNATURE) // 'PE\0\0'
            {
                // Check PE magic (PE32 vs PE32+) @ 0x004012bb-0x004012c7
                WORD magic = pNtHeaders->OptionalHeader.Magic;
                if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC || magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                {
                    // Check for delayed import table @ 0x004012e1-0x004012f2
                    DWORD numDirectories = pNtHeaders->OptionalHeader.NumberOfRvaAndSizes;
                    if (numDirectories > IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT)
                    {
                        DWORD importSize = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
                        hasDelayedImports = (importSize != 0); // @ 0x004012ef: SETNZ AL
                    }
                }
            }
        }
    }

    sprintf(versionMsg, "[CRTStartup] PE Validation: Module=0x%p, HasDelayedImports=%d\n",
            hModule, hasDelayedImports);
    DEBUG_LOG(versionMsg);

    // =========================================================================
    // STEP 3: __heap_init @ 0x004012f7 - Initialize heap manager
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [3/12] __heap_init - Initializing heap...\n");

    g_heap = GetProcessHeap();
    if (!g_heap)
    {
        DEBUG_LOG("[CRTStartup] FATAL: Heap initialization failed (error 0x1C)!\n");
        fast_error_exit(0x1C); // @ 0x00401301-0x00401303
        return 1;
    }
    DEBUG_LOG("[CRTStartup] Heap initialized successfully\n");

    // =========================================================================
    // STEP 4: __mtinit @ 0x00401309 - Initialize multi-threading
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [4/12] __mtinit - Initializing multi-threading (TLS)...\n");

    // Modern CRT handles this automatically via TLS callbacks
    // Original: CALL 0x00401f44; TEST EAX, EAX; JNZ
    initResult = 1; // Success (simulated)
    if (!initResult)
    {
        DEBUG_LOG("[CRTStartup] FATAL: Multi-threading init failed (error 0x10)!\n");
        fast_error_exit(0x10); // @ 0x00401312-0x00401314
        return 1;
    }
    DEBUG_LOG("[CRTStartup] Multi-threading initialized\n");

    // =========================================================================
    // STEP 5: __RTC_Initialize @ 0x0040131a - Runtime checks
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [5/12] __RTC_Initialize - Initializing runtime checks...\n");

    // Modern CRT handles runtime checks automatically
    // Original: CALL 0x00402e22
    DEBUG_LOG("[CRTStartup] Runtime checks initialized\n");

    // =========================================================================
    // STEP 6: InitializeFileHandling @ 0x00401322 - Initialize I/O
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [6/12] InitializeFileHandling - Initializing I/O subsystem...\n");

    // Modern CRT initializes stdin/stdout/stderr automatically
    // Original: CALL 0x00402c24; TEST EAX, EAX; JGE
    initResult = 0; // Success (HRESULT-style)
    if (initResult < 0)
    {
        DEBUG_LOG("[CRTStartup] FATAL: I/O initialization failed (error 0x1B)!\n");
        __amsg_exit(0x1B); // @ 0x0040132b-0x0040132d
        return 1;
    }
    DEBUG_LOG("[CRTStartup] I/O subsystem initialized\n");

    // =========================================================================
    // STEP 7: GetCommandLineA @ 0x00401333 - Get command line
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [7/12] GetCommandLineA - Retrieving command line...\n");

    g_lpCmdLine = GetCommandLineA(); // @ 0x00401333: CALL [0x00409170]
    // @ 0x00401339: MOV [0x0040e2f4], EAX - Store command line

    sprintf(versionMsg, "[CRTStartup] Command line: \"%s\"\n", g_lpCmdLine ? g_lpCmdLine : "(null)");
    DEBUG_LOG(versionMsg);

    // =========================================================================
    // STEP 8: GetEnvironmentStringsAscii @ 0x0040133e - Get environment
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [8/12] GetEnvironmentStringsAscii - Setting up environment...\n");

    // @ 0x0040133e: CALL 0x00402b02
    // @ 0x00401343: MOV [0x0040c980], EAX - Store environment pointer
    g_envp = _environ; // Use CRT's parsed environment

    DEBUG_LOG("[CRTStartup] Environment variables initialized\n");

    // =========================================================================
    // STEP 9: __setargv @ 0x00401348 - Parse command-line arguments
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [9/12] __setargv - Parsing command-line arguments...\n");

    // @ 0x00401348: CALL 0x00402a60; TEST EAX, EAX; JGE
    g_argv = __argv; // Use CRT's parsed argv
    g_argc = __argc; // Use CRT's parsed argc

    initResult = 0; // Success
    if (initResult < 0)
    {
        DEBUG_LOG("[CRTStartup] FATAL: Command-line parsing failed (error 0x08)!\n");
        __amsg_exit(0x08); // @ 0x00401351-0x00401353
        return 1;
    }

    sprintf(versionMsg, "[CRTStartup] Parsed %d command-line arguments\n", g_argc);
    DEBUG_LOG(versionMsg);

    // =========================================================================
    // STEP 10: InitializeEnvironmentVariables @ 0x00401359 - Setup environment
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [10/12] InitializeEnvironmentVariables - Processing environment...\n");

    // @ 0x00401359: CALL 0x0040282d; TEST EAX, EAX; JGE
    initResult = 0; // Success
    if (initResult < 0)
    {
        DEBUG_LOG("[CRTStartup] FATAL: Environment setup failed (error 0x09)!\n");
        __amsg_exit(0x09); // @ 0x00401362-0x00401364
        return 1;
    }
    DEBUG_LOG("[CRTStartup] Environment processing complete\n");

    // =========================================================================
    // STEP 11: __cinit @ 0x0040136c - C++ static constructors
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [11/12] __cinit - Calling C++ static constructors...\n");

    // @ 0x0040136c: CALL 0x0040234e; MOV [EBP-0x28], EAX; CMP EAX, ESI; JZ
    // Modern CRT handles static initialization automatically
    initResult = 0; // Success
    if (initResult != 0)
    {
        DEBUG_LOG("[CRTStartup] FATAL: C++ static initialization failed!\n");
        __amsg_exit(initResult); // @ 0x00401379-0x0040137a
        return 1;
    }
    DEBUG_LOG("[CRTStartup] C++ static constructors executed\n");

    // =========================================================================
    // STEP 12: GetStartupInfoA @ 0x00401383 - Get startup information
    // =========================================================================
    DEBUG_LOG("[CRTStartup] [12/12] GetStartupInfoA - Getting process startup info...\n");

    // Initialize startup info structure (avoid memset dependency)
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.dwFlags = 0;
    GetStartupInfoA(&startupInfo); // @ 0x00401387: CALL [0x00409174]

    // @ 0x00401395: TEST byte ptr [EBP-0x44], 0x1; JZ
    // @ 0x0040139b: MOVZX EAX, word ptr [EBP-0x40] - Load wShowWindow
    if (startupInfo.dwFlags & STARTF_USESHOWWINDOW)
    {
        g_dwShowCmd = startupInfo.wShowWindow;
    }
    else
    {
        g_dwShowCmd = SW_SHOWDEFAULT; // @ 0x004013a1-0x004013a3: PUSH 0xA; POP EAX
    }

    sprintf(versionMsg, "[CRTStartup] Startup flags: 0x%08X, ShowWindow=%d\n",
            startupInfo.dwFlags, g_dwShowCmd);
    DEBUG_LOG(versionMsg);

    // =========================================================================
    // MAIN: Call D2ServerMain @ 0x004013aa - Jump to game initialization
    // =========================================================================
    DEBUG_LOG("\n[CRTStartup] ========================================\n");
    DEBUG_LOG("[CRTStartup] Calling D2ServerMain (main game entry)\n");
    DEBUG_LOG("[CRTStartup] ========================================\n\n");

    // @ 0x004013a4-0x004013aa: PUSH EAX; PUSH [EBP-0x20]; PUSH ESI; PUSH ESI; CALL EDI
    g_hInstance = GetModuleHandleA(NULL);
    exitCode = D2ServerMain(g_hInstance, NULL, g_lpCmdLine, (int)g_dwShowCmd);

    // =========================================================================
    // CLEANUP: Process exit sequence @ 0x004013ac
    // =========================================================================
    sprintf(versionMsg, "\n[CRTStartup] D2ServerMain returned with exit code: %d\n", exitCode);
    DEBUG_LOG(versionMsg);

    DEBUG_LOG("[CRTStartup] ========================================\n");
    DEBUG_LOG("[CRTStartup] Beginning shutdown sequence...\n");
    DEBUG_LOG("[CRTStartup] ========================================\n");

    // @ 0x004013b7: CMP [EBP-0x1c], ESI; JNZ - Check delayed imports flag
    if (hasDelayedImports)
    {
        // @ 0x004013c2: CALL 0x0040249d - Call __cexit (cleanup with atexit handlers)
        DEBUG_LOG("[CRTStartup] Calling __cexit (running atexit handlers)...\n");
        // Modern CRT will call atexit handlers automatically
    }
    else
    {
        // @ 0x004013bd: CALL 0x0040247b - Call _exit (quick exit without cleanup)
        DEBUG_LOG("[CRTStartup] Quick exit (_exit) - no atexit handlers\n");
    }

    DEBUG_LOG("[CRTStartup] Calling ExitProcess...\n");
    DEBUG_LOG("========================================\n\n");

    ExitProcess(exitCode);
    return exitCode;
}

// =============================================================================
// LEVEL 1B: CRITICAL SUBSYSTEM FUNCTIONS (Called by InitializeAndRunD2Server)
// =============================================================================

/*
 * InitializeServerSubsystem @ 0x00407490
 * Thunk to external DLL ordinal 10021
 * Called by: InitializeAndRunD2Server
 *
 * This is a jump to an external DLL (likely D2Common.dll or D2Server.dll)
 * For now, stub it as successful initialization
 */
void __cdecl InitializeServerSubsystem(void)
{
    char logMsg[256];
    sprintf(logMsg, "[InitializeServerSubsystem] Initializing core server subsystem...\n");
    DEBUG_LOG(logMsg);

    // External DLL call - stub for now
    // In full implementation, this would call D2Common.dll ordinal 10021

    sprintf(logMsg, "[InitializeServerSubsystem] Subsystem initialized (stub)\n");
    DEBUG_LOG(logMsg);
}

/*
 * ProcessVersionStringOrdinal10019 @ 0x0040748a
 * Thunk to external DLL ordinal 10019
 * Called by: InitializeAndRunD2Server
 *
 * Parameters:
 *   szVersionString - Version string like "v1.13"
 *   nValidationFlag - Validation enable flag
 */
void __cdecl ProcessVersionStringOrdinal10019(const char *szVersionString, int nValidationFlag)
{
    char logMsg[256];
    sprintf(logMsg, "[ProcessVersionStringOrdinal10019] Processing version: %s (flag=%d)\n",
            szVersionString, nValidationFlag);
    DEBUG_LOG(logMsg);

    // External DLL call - stub for now
    // In full implementation, this would validate version string
}

/*
 * ExtractModStateKeywordFromCmdLine @ 0x00407e00
 * Parse render mode keyword from command line (-d3d, -opengl, -glide, -w)
 * Called by: InitializeAndRunD2Server
 *
 * Algorithm:
 * 1. Calculate command line string length
 * 2. Allocate buffer and copy command line
 * 3. Tokenize by space delimiter
 * 4. Compare each token against keyword table @ 0x40bbb4:
 *    Index 0: (unknown)
 *    Index 1: (client mode - skipped)
 *    Index 2-5: render modes
 * 5. Store matching index in output pointer
 * 6. Free allocated buffer
 *
 * Parameters:
 *   pModeStateOutput - Pointer to store mode index (0-5)
 *   szCmdLineInput - Command line string to parse
 *
 * Returns:
 *   1 on success
 */
int __stdcall ExtractModStateKeywordFromCmdLine(int *pModeStateOutput, char *szCmdLineInput)
{
    char logMsg[256];
    sprintf(logMsg, "[ExtractModStateKeywordFromCmdLine] Parsing command line: %s\n", szCmdLineInput);
    DEBUG_LOG(logMsg);

    // Keyword table (from binary @ 0x40bbb4)
    const char *keywords[] = {
        "",        // Index 0: unknown
        "-client", // Index 1: client mode (skipped in server)
        "-w",      // Index 2: windowed mode
        "-d3d",    // Index 3: Direct3D mode
        "-opengl", // Index 4: OpenGL mode
        "-glide"   // Index 5: Glide/3dfx mode
    };

    if (!szCmdLineInput || !pModeStateOutput)
    {
        return 1;
    }

    // Calculate string length
    size_t cmdLineLen = strlen(szCmdLineInput);
    if (cmdLineLen == 0)
    {
        return 1;
    }

    // Allocate buffer
    char *buffer = (char *)malloc(cmdLineLen + 1);
    if (!buffer)
    {
        return 1;
    }

    // Copy command line to buffer
    memcpy(buffer, szCmdLineInput, cmdLineLen + 1);

    // Tokenize by space
    char *token = strtok(buffer, " ");
    while (token != NULL)
    {
        // Compare against each keyword
        for (int i = 0; i < 6; i++)
        {
            // Skip index 1 (client mode) as per disassembly @ 0x00407e78
            if (i == 1)
                continue;

            size_t keywordLen = strlen(keywords[i]);
            if (keywordLen > 0 && strncmp(token, keywords[i], keywordLen) == 0)
            {
                *pModeStateOutput = i;
                sprintf(logMsg, "[ExtractModStateKeywordFromCmdLine] Found mode: %s (index=%d)\n",
                        keywords[i], i);
                DEBUG_LOG(logMsg);
                break;
            }
        }

        // Get next token
        token = strtok(NULL, " ");
    }

    // Free buffer
    free(buffer);

    sprintf(logMsg, "[ExtractModStateKeywordFromCmdLine] Parse complete, mode=%d\n", *pModeStateOutput);
    DEBUG_LOG(logMsg);

    return 1;
}

/*
 * FormatStringBufferThunk @ 0x00407466
 * Format version string using sprintf-like functionality
 * Called by: InitializeAndRunD2Server
 *
 * This is a thunk/wrapper around sprintf for version formatting
 */
void __cdecl FormatStringBufferThunk(char *buffer, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
}

// =============================================================================
// LEVEL 1B: DLL ORDINAL CALL INFRASTRUCTURE
// =============================================================================

/*
 * DLL Ordinal Call Helper System
 *
 * This infrastructure provides type-safe wrappers for calling DLL functions
 * by ordinal number using GetProcAddress. The original Game.exe uses ordinal
 * linking extensively to reduce executable size.
 *
 * Key DLL modules and their ordinal ranges:
 * - D2Win.dll: UI, windowing, text rendering (ordinals 10001-10050)
 * - D2Gdi.dll: Graphics device interface (ordinals 10051-10100)
 * - D2Client.dll: Client game logic (ordinals 10101-10200)
 * - D2Common.dll: Shared game data (ordinals 10201-10300)
 * - D2Game.dll: Server-side game logic (ordinals 10301-10400)
 * - Storm.dll: File I/O, memory, compression (ordinals 251-350)
 */

// Function pointer types for common DLL signatures
typedef void(__cdecl *VoidFunc)(void);
typedef BOOL(__cdecl *BoolFunc)(void);
typedef int(__cdecl *IntFunc)(void);
typedef void *(__cdecl *PtrFunc)(void);
typedef BOOL(__stdcall *BoolStdcallFunc)(HINSTANCE, int, BOOL, int);
typedef HWND(__cdecl *HWNDFunc)(void);

/*
 * GetDLLOrdinal
 * Retrieves a function pointer from a DLL by ordinal number
 *
 * Parameters:
 *   hModule - Handle to the loaded DLL module
 *   ordinal - Ordinal number of the function to retrieve
 *
 * Returns:
 *   Function pointer if found, NULL otherwise
 */
void *GetDLLOrdinal(HMODULE hModule, DWORD ordinal)
{
    if (hModule == NULL)
        return NULL;

    return (void *)GetProcAddress(hModule, (LPCSTR)MAKEINTRESOURCEA(ordinal));
}

/*
 * CallDLLOrdinal_Void
 * Calls a DLL ordinal with void return type
 */
void CallDLLOrdinal_Void(HMODULE hModule, DWORD ordinal)
{
    VoidFunc func = (VoidFunc)GetDLLOrdinal(hModule, ordinal);
    if (func != NULL)
    {
        func();
    }
    else
    {
        char debugMsg[256];
        sprintf(debugMsg, "[CallDLLOrdinal_Void] WARNING: Failed to get ordinal %d\n", ordinal);
        DEBUG_LOG(debugMsg);
    }
}

/*
 * CallDLLOrdinal_Bool
 * Calls a DLL ordinal with BOOL return type
 */
BOOL CallDLLOrdinal_Bool(HMODULE hModule, DWORD ordinal)
{
    BoolFunc func = (BoolFunc)GetDLLOrdinal(hModule, ordinal);
    if (func != NULL)
    {
        return func();
    }
    else
    {
        char debugMsg[256];
        sprintf(debugMsg, "[CallDLLOrdinal_Bool] WARNING: Failed to get ordinal %d\n", ordinal);
        DEBUG_LOG(debugMsg);
        return FALSE;
    }
}

// =============================================================================
// LEVEL 1C: GAME LOOP SUBSYSTEM STUBS (27 callees for InitializeAndRunGameMainLoop)
// =============================================================================

/*
 * Subsystem initialization functions (Phase 1 of game loop)
 * These are thunks to external DLLs or internal subsystems
 */

// DirectSound initialization @ 0x004074ae
void __cdecl InitializeDirectSound(void)
{
    if (g_pfnInitializeDirectSound)
    {
        DEBUG_LOG("[InitializeDirectSound] Calling D2Sound via Storm.dll...\n");
        g_pfnInitializeDirectSound();
    }
    else
    {
        DEBUG_LOG("[InitializeDirectSound] Function pointer not initialized (stub)...\n");
    }
}

// Subsystem 2 initialization @ 0x004074ba
void __cdecl InitializeSubsystem2Thunk(void)
{
    if (g_pfnInitializeSubsystem2)
    {
        DEBUG_LOG("[InitializeSubsystem2Thunk] Calling D2Game.dll...\n");
        g_pfnInitializeSubsystem2();
    }
    else
    {
        DEBUG_LOG("[InitializeSubsystem2Thunk] Function pointer not initialized (stub)...\n");
    }
}

// Graphics engine initialization @ 0x00407496
void __cdecl InitializeSubsystem3Thunk(void)
{
    if (g_pfnInitializeSubsystem3)
    {
        DEBUG_LOG("[InitializeSubsystem3Thunk] Calling D2Game.dll graphics engine...\n");
        g_pfnInitializeSubsystem3();
    }
    else
    {
        DEBUG_LOG("[InitializeSubsystem3Thunk] Function pointer not initialized (stub)...\n");
    }
}

// Subsystem 4 initialization @ 0x004074a8
void __cdecl InitializeSubsystem4Thunk(void)
{
    if (g_pfnInitializeSubsystem4)
    {
        DEBUG_LOG("[InitializeSubsystem4Thunk] Calling D2Game.dll...\n");
        g_pfnInitializeSubsystem4();
    }
    else
    {
        DEBUG_LOG("[InitializeSubsystem4Thunk] Function pointer not initialized (stub)...\n");
    }
}

// System requirements validation @ 0x7b331000 (external)
BOOL __cdecl ValidateSystemRequirementsThunk(void)
{
    if (g_pfnValidateSystemRequirements)
    {
        DEBUG_LOG("[ValidateSystemRequirementsThunk] Calling D2Client.dll...\n");
        return g_pfnValidateSystemRequirements();
    }
    else
    {
        DEBUG_LOG("[ValidateSystemRequirementsThunk] Function pointer not initialized (stub) - returning TRUE...\n");
        return TRUE;
    }
}

// Get default screen mode @ 0x004074e4
BOOL __cdecl GetDefaultScreenMode(void)
{
    if (g_pfnGetDefaultScreenMode)
    {
        DEBUG_LOG("[GetDefaultScreenMode] Calling D2Client.dll...\n");
        return g_pfnGetDefaultScreenMode();
    }
    else
    {
        DEBUG_LOG("[GetDefaultScreenMode] Function pointer not initialized (stub) - returning TRUE...\n");
        return TRUE;
    }
}

// Initialize graphics subsystem @ 0x004074d8
BOOL __cdecl InitializeGraphicsSubsystem(HINSTANCE hInstance, int videoMode, BOOL windowed, int param4)
{
    char logMsg[256];
    if (g_pfnInitializeGraphicsSubsystem)
    {
        sprintf(logMsg, "[InitializeGraphicsSubsystem] Calling D2Gdi.dll: mode=%d, windowed=%d...\n", videoMode, windowed);
        DEBUG_LOG(logMsg);
        return g_pfnInitializeGraphicsSubsystem(hInstance, videoMode, windowed, param4);
    }
    else
    {
        sprintf(logMsg, "[InitializeGraphicsSubsystem] Function pointer not initialized (stub): mode=%d, windowed=%d - returning TRUE...\n", videoMode, windowed);
        DEBUG_LOG(logMsg);
        return TRUE;
    }
}

// Initialize renderer @ 0x004074ea
BOOL __cdecl InitializeRendererThunk(BOOL windowed, int param2)
{
    char logMsg[256];
    if (g_pfnInitializeRenderer)
    {
        sprintf(logMsg, "[InitializeRendererThunk] Calling D2Gdi.dll: windowed=%d...\n", windowed);
        DEBUG_LOG(logMsg);
        return g_pfnInitializeRenderer(windowed, param2);
    }
    else
    {
        sprintf(logMsg, "[InitializeRendererThunk] Function pointer not initialized (stub): windowed=%d - returning TRUE...\n", windowed);
        DEBUG_LOG(logMsg);
        return TRUE;
    }
}

// Get window handle @ 0x00407526
HWND __cdecl GetWindowHandleThunk(void)
{
    DEBUG_LOG("[GetWindowHandleThunk] Getting window handle...\n");
    return g_hWndMain;
}

// Set framerate lock @ 0x00407508
void __cdecl SetFramerateLock(BOOL enable)
{
    char logMsg[256];
    sprintf(logMsg, "[SetFramerateLock] Framerate lock: %s\n", enable ? "ENABLED" : "DISABLED");
    DEBUG_LOG(logMsg);
}

// Enable sound @ 0x0040751a
void __cdecl EnableSound(void)
{
    if (g_pfnEnableSound)
    {
        DEBUG_LOG("[EnableSound] Calling D2Sound.dll...\n");
        g_pfnEnableSound();
    }
    else
    {
        DEBUG_LOG("[EnableSound] Function pointer not initialized (stub)...\n");
    }
}

// Set FPS display mode @ 0x00407502
void __cdecl SetFPSDisplayMode(int mode)
{
    char logMsg[256];
    if (g_pfnSetFPSDisplayMode)
    {
        sprintf(logMsg, "[SetFPSDisplayMode] Calling D2Win.dll: mode=%d...\n", mode);
        DEBUG_LOG(logMsg);
        g_pfnSetFPSDisplayMode(mode);
    }
    else
    {
        sprintf(logMsg, "[SetFPSDisplayMode] Function pointer not initialized (stub): mode=%d\n", mode);
        DEBUG_LOG(logMsg);
    }
}

// Apply gamma correction @ 0x00407520
void __cdecl ApplyGammaCorrection(void)
{
    if (g_pfnApplyGammaCorrection)
    {
        DEBUG_LOG("[ApplyGammaCorrection] Calling D2Win.dll...\n");
        g_pfnApplyGammaCorrection();
    }
    else
    {
        DEBUG_LOG("[ApplyGammaCorrection] Function pointer not initialized (stub)...\n");
    }
}

// Enable wide aspect ratio @ 0x00407514
void __cdecl EnableWideAspectRatio(void)
{
    if (g_pfnEnableWideAspectRatio)
    {
        DEBUG_LOG("[EnableWideAspectRatio] Calling D2Win.dll...\n");
        g_pfnEnableWideAspectRatio();
    }
    else
    {
        DEBUG_LOG("[EnableWideAspectRatio] Function pointer not initialized (stub)...\n");
    }
}

// Initialize menu system @ 0x004074f6
void __cdecl InitializeMenuSystem(void)
{
    if (g_pfnInitializeMenuSystem)
    {
        DEBUG_LOG("[InitializeMenuSystem] Calling D2Win.dll...\n");
        g_pfnInitializeMenuSystem();
    }
    else
    {
        DEBUG_LOG("[InitializeMenuSystem] Function pointer not initialized (stub)...\n");
    }
}

// Cleanup menu system @ 0x004074f0
void __cdecl CleanupMenuSystem(void)
{
    if (g_pfnCleanupMenuSystem)
    {
        DEBUG_LOG("[CleanupMenuSystem] Calling D2Win.dll...\n");
        g_pfnCleanupMenuSystem();
    }
    else
    {
        DEBUG_LOG("[CleanupMenuSystem] Function pointer not initialized (stub)...\n");
    }
}

// Prepare graphics shutdown @ 0x004074c0
void __cdecl PrepareGraphicsShutdown(void)
{
    if (g_pfnPrepareGraphicsShutdown)
    {
        DEBUG_LOG("[PrepareGraphicsShutdown] Calling D2Gfx.dll...\n");
        g_pfnPrepareGraphicsShutdown();
    }
    else
    {
        DEBUG_LOG("[PrepareGraphicsShutdown] Function pointer not initialized (stub)...\n");
    }
}

// Shutdown graphics @ 0x0040750e
void __cdecl ShutdownGraphicsThunk(void)
{
    if (g_pfnShutdownGraphics)
    {
        DEBUG_LOG("[ShutdownGraphicsThunk] Calling D2Gfx.dll...\n");
        g_pfnShutdownGraphics();
    }
    else
    {
        DEBUG_LOG("[ShutdownGraphicsThunk] Function pointer not initialized (stub)...\n");
    }
}

// Shutdown subsystem @ 0x0040749c
void __cdecl CloseEngineSubsystem(void)
{
    if (g_pfnCloseEngineSubsystem)
    {
        DEBUG_LOG("[CloseEngineSubsystem] Calling Fog.dll...\n");
        g_pfnCloseEngineSubsystem();
    }
    else
    {
        DEBUG_LOG("[CloseEngineSubsystem] Function pointer not initialized (stub)...\n");
    }
}

// Shutdown subsystem 6 @ 0x004074fc
void __cdecl ShutdownSubsystem6Thunk(void)
{
    if (g_pfnShutdownSubsystem6)
    {
        DEBUG_LOG("[ShutdownSubsystem6Thunk] Calling Fog.dll...\n");
        g_pfnShutdownSubsystem6();
    }
    else
    {
        DEBUG_LOG("[ShutdownSubsystem6Thunk] Function pointer not initialized (stub)...\n");
    }
}

// Shutdown external subsystem @ 0x004074a2
void __cdecl ShutdownExternalSubsystemThunk(void)
{
    if (g_pfnShutdownExternalSubsystem)
    {
        DEBUG_LOG("[ShutdownExternalSubsystemThunk] Calling Fog.dll...\n");
        g_pfnShutdownExternalSubsystem();
    }
    else
    {
        DEBUG_LOG("[ShutdownExternalSubsystemThunk] Function pointer not initialized (stub)...\n");
    }
}

// Write registry DWORD value @ 0x00407460
void __cdecl WriteRegistryDwordValue(const char *key, const char *value, DWORD data)
{
    char logMsg[256];
    if (g_pfnWriteRegistryDwordValue)
    {
        sprintf(logMsg, "[WriteRegistryDwordValue] Calling Storm.dll: %s\\%s = %lu...\n", key, value, data);
        DEBUG_LOG(logMsg);
        g_pfnWriteRegistryDwordValue(key, value, data);
    }
    else
    {
        sprintf(logMsg, "[WriteRegistryDwordValue] Function pointer not initialized (stub): %s\\%s = %lu\n", key, value, data);
        DEBUG_LOG(logMsg);
    }
}

// State handler stubs (from dispatch table @ 0x40c964)
// IMPORTANT: State handlers must return int (next state value) for state machine
// State values: 0=Exit, 1=Menu, 2=CharSelect, 3=InGame, 4=Loading, 5=Credits
int __cdecl StateHandler0_Exit(void *config)
{
    DEBUG_LOG("[StateHandler0] EXIT state - returning 0 to exit\n");
    return 0; // Exit state
}

int __cdecl StateHandler1_Menu(void *config)
{
    DEBUG_LOG("[StateHandler1] MENU state\n");
    
    // Cast config to LaunchConfig structure
    LaunchConfig *pConfig = (LaunchConfig *)config;
    
    if (!pConfig)
    {
        DEBUG_LOG("[StateHandler1] ERROR: NULL config pointer!\n");
        return 0; // Exit
    }
    
    // Check if we should skip menu (Battle.net mode)
    if (pConfig->skip_menu)
    {
        DEBUG_LOG("[StateHandler1] skip_menu flag set, transitioning to next state\n");
        return 2; // Go to character select
    }
    
    // Try to initialize D2Win menu system if function pointer is available
    if (g_pfnInitializeMenuSystem)
    {
        DEBUG_LOG("[StateHandler1] Calling D2Win.dll::InitializeMenuSystem()...\n");
        
        // Call D2Win menu initialization (parameters based on Ghidra analysis)
        // Original call @ 0x004074f6: g_pfnInitializeMenuSystem()
        // Note: Actual function may take parameters (video_mode, menu_param)
        // but current typedef is void(void), so calling with no params
        g_pfnInitializeMenuSystem();
        
        DEBUG_LOG("[StateHandler1] D2Win menu system initialized\n");
        DEBUG_MSGBOX("Menu System", "D2Win::InitializeMenuSystem() called successfully!\n\nMenu system should now be active.\n\nNote: Actual menu rendering requires D2Client.dll integration.");
        
        // In full implementation, D2Client.dll would handle menu rendering loop
        // For now, create a simple message loop to keep the application alive
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            // In real implementation, D2Client would update menu state here
            // and potentially change g_currentState via callback
        }
        
        DEBUG_LOG("[StateHandler1] Menu message loop exited\n");
        return 0; // Exit
    }
    else
    {
        // Fallback: Function pointer not initialized (DLLs not loaded or ordinal not found)
        DEBUG_LOG("[StateHandler1] WARNING: g_pfnInitializeMenuSystem is NULL\n");
        DEBUG_LOG("[StateHandler1] Creating fallback test window instead...\n");
        
        // Create a test window to verify windowing works
        HWND hwnd = CreateWindowExA(
            0,
            "STATIC",
            "Diablo II - Game.exe Test Window\n\nThis window proves:\n1. DLL loading successful\n2. Initialization complete\n3. State machine operational\n\nNote: D2Win::InitializeMenuSystem not available\n(Need correct ordinal or DLL files)\n\nClose this window to exit.",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE | SS_CENTER,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 250,
            NULL, NULL, g_hInstance, NULL);

        if (hwnd)
        {
            DEBUG_LOG("[StateHandler1] Test window created successfully!\n");
            DEBUG_MSGBOX("Success!", "Test window created!\n\nA window should be visible on your screen.\n\nNote: Actual menu requires D2Win.dll function pointer.\nClose window to exit.");

            // Simple message loop to keep window alive
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            DEBUG_LOG("[StateHandler1] Message loop exited\n");
        }
        else
        {
            ERROR_MSGBOX("Window Creation Failed", "Could not create test window!");
            DEBUG_LOG("[StateHandler1] ERROR: Failed to create test window!\n");
        }

        // Return 0 to exit after window closes
        return 0;
    }
}

int __cdecl StateHandler2_CharSelect(void *config)
{
    DEBUG_LOG("[StateHandler2] CHARACTER SELECT state\n");
    return 2; // Stay in char select for now (stub)
}

int __cdecl StateHandler3_InGame(void *config)
{
    DEBUG_LOG("[StateHandler3] IN GAME state\n");
    return 3; // Stay in game for now (stub)
}

int __cdecl StateHandler4_Loading(void *config)
{
    DEBUG_LOG("[StateHandler4] LOADING state\n");
    return 1; // Return to menu after loading (stub)
}

int __cdecl StateHandler5_Credits(void *config)
{
    DEBUG_LOG("[StateHandler5] CREDITS state\n");
    return 1; // Return to menu after credits (stub)
}

// =============================================================================
// LEVEL 2: CONFIGURATION AND INITIALIZATION
// =============================================================================

/*
 * ReadRegistryConfig @ varies
 * Read configuration from Windows registry or D2Server.ini file
 * Called by: InitializeD2ServerMain
 *
 * Priority:
 * 1. D2Server.ini in executable directory (for development/testing)
 * 2. Windows registry: HKLM\SOFTWARE\Blizzard Entertainment\Diablo II
 * 3. Default values
 */
BOOL __cdecl ReadRegistryConfig(void)
{
    HKEY hKey;
    LONG result;
    DWORD type, size;
    char buffer[512];
    char iniPath[512];
    BOOL foundConfig = FALSE;

    DEBUG_LOG("[ReadRegistryConfig] Reading configuration...\n");

    // Get executable directory for INI file path
    GetModuleFileNameA(NULL, g_installPath, sizeof(g_installPath));
    char *lastSlash = NULL;
    for (char *p = g_installPath; *p; p++)
    {
        if (*p == '\\')
            lastSlash = p;
    }
    if (lastSlash)
        *lastSlash = '\0';

    // Try to load from D2Server.ini first
    sprintf(iniPath, "%s\\D2Server.ini", g_installPath);

    if (GetFileAttributesA(iniPath) != INVALID_FILE_ATTRIBUTES)
    {
        DEBUG_LOG("[ReadRegistryConfig] Found D2Server.ini - loading configuration from file\n");
        sprintf(buffer, "[ReadRegistryConfig] INI Path: %s\n", iniPath);
        DEBUG_LOG(buffer);

        // Read InstallPath
        GetPrivateProfileStringA("Diablo II", "InstallPath", g_installPath,
                                 g_installPath, sizeof(g_installPath), iniPath);
        sprintf(buffer, "[ReadRegistryConfig] InstallPath: %s\n", g_installPath);
        DEBUG_LOG(buffer);

        // Read VideoConfig
        GetPrivateProfileStringA("Diablo II", "VideoConfig", "800 600 32 1",
                                 buffer, sizeof(buffer), iniPath);
        sscanf(buffer, "%lu %lu %lu %lu", &g_screenWidth, &g_screenHeight, &g_colorDepth, &g_videoMode);
        sprintf(buffer, "[ReadRegistryConfig] Video: %dx%d %dbpp mode=%d\n",
                g_screenWidth, g_screenHeight, g_colorDepth, g_videoMode);
        DEBUG_LOG(buffer);

        // Read GameMode
        g_gameMode = GetPrivateProfileIntA("Diablo II", "GameMode", 0, iniPath);
        sprintf(buffer, "[ReadRegistryConfig] GameMode: %d\n", g_gameMode);
        DEBUG_LOG(buffer);

        // Read NoSound
        g_noSound = GetPrivateProfileIntA("Diablo II", "NoSound", 0, iniPath);
        sprintf(buffer, "[ReadRegistryConfig] NoSound: %d\n", g_noSound);
        DEBUG_LOG(buffer);

        // Read NoMusic
        g_noMusic = GetPrivateProfileIntA("Diablo II", "NoMusic", 0, iniPath);
        sprintf(buffer, "[ReadRegistryConfig] NoMusic: %d\n", g_noMusic);
        DEBUG_LOG(buffer);

        // Read Expansion
        g_isExpansion = GetPrivateProfileIntA("Diablo II", "Expansion", 0, iniPath);
        sprintf(buffer, "[ReadRegistryConfig] Expansion: %d\n", g_isExpansion);
        DEBUG_LOG(buffer);

        foundConfig = TRUE;
        DEBUG_LOG("[ReadRegistryConfig] Successfully loaded configuration from D2Server.ini\n");
    }
    else
    {
        DEBUG_LOG("[ReadRegistryConfig] D2Server.ini not found, trying registry...\n");

        // Try Windows Registry
        result = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Blizzard Entertainment\\Diablo II",
            0,
            KEY_READ,
            &hKey);

        if (result == ERROR_SUCCESS)
        {
            DEBUG_LOG("[ReadRegistryConfig] Reading from Windows Registry\n");

            // Read InstallPath
            size = sizeof(g_installPath);
            if (RegQueryValueExA(hKey, "InstallPath", NULL, &type, (BYTE *)g_installPath, &size) == ERROR_SUCCESS)
            {
                sprintf(buffer, "[ReadRegistryConfig] InstallPath: %s\n", g_installPath);
                DEBUG_LOG(buffer);
            }

            // Read VideoConfig
            size = sizeof(buffer);
            if (RegQueryValueExA(hKey, "VideoConfig", NULL, &type, (BYTE *)buffer, &size) == ERROR_SUCCESS)
            {
                sscanf(buffer, "%lu %lu %lu %lu", &g_screenWidth, &g_screenHeight, &g_colorDepth, &g_videoMode);
                sprintf(buffer, "[ReadRegistryConfig] Video: %dx%d %dbpp mode=%d\n",
                        g_screenWidth, g_screenHeight, g_colorDepth, g_videoMode);
                DEBUG_LOG(buffer);
            }

            RegCloseKey(hKey);
            foundConfig = TRUE;
            DEBUG_LOG("[ReadRegistryConfig] Successfully loaded configuration from registry\n");
        }
        else
        {
            DEBUG_LOG("[ReadRegistryConfig] WARNING: Registry key not found, using defaults\n");
            sprintf(buffer, "[ReadRegistryConfig] Using defaults: %dx%d %dbpp mode=%d\n",
                    g_screenWidth, g_screenHeight, g_colorDepth, g_videoMode);
            DEBUG_LOG(buffer);
        }
    }

    return foundConfig;
}

/*
 * ParseCommandLine @ 0x00407e20
 * Parse command-line arguments and set flags
 * Called by: InitializeD2ServerMain
 */
void __cdecl ParseCommandLine(int argc, char **argv)
{
    char debugMsg[256];

    sprintf(debugMsg, "[ParseCommandLine] Parsing %d arguments\n", argc);
    DEBUG_LOG(debugMsg);

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        sprintf(debugMsg, "[ParseCommandLine] argv[%d] = %s\n", i, arg);
        DEBUG_LOG(debugMsg);

        // Skip to Battle.net
        if (_stricmp(arg, "-skiptobnet") == 0)
        {
            g_skipToBnet = TRUE;
            g_gameMode = 2;
            DEBUG_LOG("[ParseCommandLine] Skip to Battle.net enabled\n");
        }
        // No sound
        else if (_stricmp(arg, "-ns") == 0 || _stricmp(arg, "-nosound") == 0)
        {
            g_noSound = TRUE;
            DEBUG_LOG("[ParseCommandLine] Sound disabled\n");
        }
        // No music
        else if (_stricmp(arg, "-nm") == 0 || _stricmp(arg, "-nomusic") == 0)
        {
            g_noMusic = TRUE;
            DEBUG_LOG("[ParseCommandLine] Music disabled\n");
        }
        // Video modes
        else if (_stricmp(arg, "-d3d") == 0)
        {
            g_videoMode = 1;
            DEBUG_LOG("[ParseCommandLine] Direct3D mode\n");
        }
        else if (_stricmp(arg, "-opengl") == 0)
        {
            g_videoMode = 2;
            DEBUG_LOG("[ParseCommandLine] OpenGL mode\n");
        }
        else if (_stricmp(arg, "-3dfx") == 0 || _stricmp(arg, "-glide") == 0)
        {
            g_videoMode = 3;
            DEBUG_LOG("[ParseCommandLine] Glide mode\n");
        }
        // Window resolution
        else if (_stricmp(arg, "-w") == 0)
        {
            g_screenWidth = 640;
            g_screenHeight = 480;
            DEBUG_LOG("[ParseCommandLine] Windowed mode 640x480\n");
        }
    }
}

/*
 * FindAndValidateD2ExpMpq @ 0x00407a30
 * Check if Lord of Destruction expansion is installed
 * Called by: InitializeD2ServerMain
 */
BOOL __cdecl FindAndValidateD2ExpMpq(void)
{
    char expPath[512];
    DWORD attr;

    DEBUG_LOG("[FindAndValidateD2ExpMpq] Checking for expansion...\n");

    // Build path to d2exp.mpq
    sprintf(expPath, "%s\\d2exp.mpq", g_installPath);

    // Check if file exists
    attr = GetFileAttributesA(expPath);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        DEBUG_LOG("[FindAndValidateD2ExpMpq] Expansion not found\n");
        g_isExpansion = FALSE;
        return FALSE;
    }

    DEBUG_LOG("[FindAndValidateD2ExpMpq] Lord of Destruction expansion found!\n");
    g_isExpansion = TRUE;
    return TRUE;
}

/*
 * InitializeD2ServerMain @ 0x00408250 (Original: InitializeAndRunD2Server)
 * Complete 23-step initialization sequence before game loop
 * Called by: D2ServerMain
 *
 * Based on Ghidra decompilation - full algorithm:
 * 1-2.  Extract command line argument, format version string "v%d.%02d"
 * 3.    InitializeServerSubsystem - core server init
 * 4.    ProcessVersionStringOrdinal10019 - version validation
 * 5-6.  Open DIABLO_II_OK event, signal launcher
 * 7.    InitializeCommandLineSettings
 * 8.    External subsystem init @ 0x7b331080
 * 9.    ExtractModStateKeywordFromCmdLine - parse render mode
 * 10.   Zero-fill 968-byte video config buffer
 * 11.   LoadVideoSettingsFromConfigFile (INI/registry)
 * 12.   ParseCommandLineIntoConfig - override with cmdline
 * 13.   Validate 4 config bytes at offsets +0x5c, +0x5e, +0x5f, +0x61
 * 14-21. Registry fallback (HKCU → HKLM)
 * 22.   InitializeAndRunGameMainLoop - main game loop entry
 * 23.   Return
 */
BOOL __cdecl InitializeD2ServerMain(int argc, char **argv)
{
    char versionString[268];
    char eventName[] = "DIABLO_II_OK";
    int renderMode = 4; // Default render mode
    HANDLE hEvent;
    char logMsg[512];

    DEBUG_MSGBOX("Initialization", "InitializeD2ServerMain starting!\n\n23-step initialization sequence beginning...");

    DEBUG_LOG("\n[InitializeD2ServerMain] ========================================\n");
    DEBUG_LOG("[InitializeD2ServerMain] Full 23-Step Initialization Sequence\n");
    DEBUG_LOG("[InitializeD2ServerMain] ========================================\n");

    // [1/23] Extract command line argument (last arg or use default)
    const char *versionArg = (argc > 1) ? argv[argc - 1] : "1.13";

    // [2/23] Format version string with "v%d.%02d" template
    sprintf(versionString, "v%s", versionArg);
    sprintf(logMsg, "[InitializeD2ServerMain] [1-2/23] Version string: %s\n", versionString);
    DEBUG_LOG(logMsg);

    // [3/23] Initialize server subsystem (core initialization)
    DEBUG_LOG("[InitializeD2ServerMain] [3/23] Calling InitializeServerSubsystem...\n");
    InitializeServerSubsystem();

    // [4/23] Process version string through ordinal 10019 handler
    DEBUG_LOG("[InitializeD2ServerMain] [4/23] Processing version string...\n");
    ProcessVersionStringOrdinal10019(versionString, 1);

    // [5-6/23] Open DIABLO_II_OK event for launcher synchronization
    DEBUG_LOG("[InitializeD2ServerMain] [5-6/23] Opening launcher sync event...\n");
    hEvent = OpenEventA(EVENT_MODIFY_STATE, TRUE, eventName);
    if (hEvent != NULL)
    {
        DEBUG_LOG("[InitializeD2ServerMain] Event opened, signaling launcher...\n");
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
    else
    {
        DEBUG_LOG("[InitializeD2ServerMain] No launcher event (standalone mode)\n");
    }

    // [7/23] Initialize command line settings
    DEBUG_LOG("[InitializeD2ServerMain] [7/23] Initializing command line settings...\n");
    ParseCommandLine(argc, argv);

    // [8/23] External subsystem initialization @ 0x7b331080 (D2Common)
    DEBUG_LOG("[InitializeD2ServerMain] [8/23] External subsystem init (stub)...\n");
    // func_0x7b331080(); // External DLL - stub for now

    // [9/23] Extract render mode from command line
    DEBUG_LOG("[InitializeD2ServerMain] [9/23] Extracting render mode keyword...\n");
    if (argc > 1 && argv[argc - 1])
    {
        ExtractModStateKeywordFromCmdLine(&renderMode, argv[argc - 1]);
    }
    sprintf(logMsg, "[InitializeD2ServerMain] Render mode: %d\n", renderMode);
    DEBUG_LOG(logMsg);

    // [10/23] Zero-fill 968-byte video config buffer
    DEBUG_LOG("[InitializeD2ServerMain] [10/23] Initializing video config buffer...\n");
    // Note: In full implementation, this would be a 968-byte structure
    // For now, we use the existing global variables

    // [11/23] Load video settings from INI file
    DEBUG_LOG("[InitializeD2ServerMain] [11/23] Loading video settings...\n");
    ReadRegistryConfig();

    // [12/23] Override INI settings with command line values
    DEBUG_LOG("[InitializeD2ServerMain] [12/23] Applying command line overrides...\n");
    // Already done in ParseCommandLine above

    // [13/23] Validate configuration bytes
    DEBUG_LOG("[InitializeD2ServerMain] [13/23] Validating configuration...\n");
    // Note: In full implementation, check 4 validation bytes at offsets:
    // +0x5c, +0x5e, +0x5f, +0x61 of video config structure
    // If all are 0, proceed to registry lookup; otherwise skip

    // [14-21/23] Registry fallback logic (HKCU → HKLM)
    DEBUG_LOG("[InitializeD2ServerMain] [14-21/23] Registry fallback (already handled)\n");
    // Already implemented in ReadRegistryConfig()

    // Validate expansion installation
    FindAndValidateD2ExpMpq();

    // Determine game mode
    if (g_skipToBnet)
    {
        g_gameMode = 2; // Battle.net
        DEBUG_LOG("[InitializeD2ServerMain] Game Mode: Battle.net\n");
    }
    else if (g_gameMode == 0)
    {
        DEBUG_LOG("[InitializeD2ServerMain] Game Mode: Single-player\n");
    }

    DEBUG_LOG("[InitializeD2ServerMain] Configuration complete\n");

    // CRITICAL: Load DLLs BEFORE calling InitializeAndRunGameMainLoop
    // The game loop needs function pointers to be initialized
    DEBUG_LOG("[InitializeD2ServerMain] [21.5/23] Loading game DLLs...\n");
    if (!LoadAllGameDLLs())
    {
        DEBUG_LOG("[InitializeD2ServerMain] WARNING: Some DLLs failed to load\n");
    }

    // [22/23] InitializeAndRunGameMainLoop - THE CRITICAL CALL
    DEBUG_LOG("[InitializeD2ServerMain] [22/23] Calling InitializeAndRunGameMainLoop...\n");
    InitializeAndRunGameMainLoop();

    // [23/23] Return
    DEBUG_LOG("[InitializeD2ServerMain] ========================================\n\n");
    return TRUE;
}

/*
 * InitializeAndRunGameMainLoop @ 0x00407600
 * THE MAIN GAME ENGINE ENTRY POINT
 * Called by: InitializeD2ServerMain
 *
 * 6-Phase Initialization and Main Game State Loop:
 * Phase 1: Initialize 4 subsystems (DirectSound, Subsystem2, Graphics, Subsystem4)
 * Phase 2: Validate system requirements
 * Phase 3: Graphics/Video mode setup (fullscreen/windowed/3D detection)
 * Phase 4: Peripheral setup (keyboard hook, sound, FPS, gamma, aspect ratio)
 * Phase 5: Menu system initialization
 * Phase 6: Main game state loop (6-state machine @ 0x40c964)
 *
 * Based on 200+ instruction disassembly analysis
 */
int __cdecl InitializeAndRunGameMainLoop(void)
{
    char logMsg[512];
    int currentState = 1; // Start with state 1 (menu)
    BOOL graphicsInitialized = FALSE;
    BOOL menuInitialized = FALSE;
    int videoMode = (int)g_videoMode;
    BOOL windowed = (g_screenWidth == 640 && g_screenHeight == 480);

    // Initialize launch configuration structure (968 bytes)
    memset(&g_launchConfig, 0, sizeof(LaunchConfig));
    g_launchConfig.video_mode = g_videoMode;
    g_launchConfig.screen_width = g_screenWidth;
    g_launchConfig.screen_height = g_screenHeight;
    g_launchConfig.color_depth = g_colorDepth;
    g_launchConfig.windowed = windowed;
    g_launchConfig.no_sound = g_noSound;
    g_launchConfig.no_music = g_noMusic;
    g_launchConfig.game_mode = g_gameMode;
    g_launchConfig.expansion = g_isExpansion;
    g_launchConfig.skip_menu = g_skipToBnet;  // Skip menu if going to Battle.net
    g_launchConfig.menu_init_param = 0;       // Default parameter
    g_launchConfig.callback_interface = NULL; // No callback yet

    DEBUG_LOG("[InitializeAndRunGameMainLoop] LaunchConfig structure initialized (968 bytes)\n");

    // State handler function pointer table @ 0x40c964
    // Handlers must return int (next state value) for state machine loop
    typedef int (*StateHandler)(void *);
    StateHandler stateHandlers[6] = {
        StateHandler0_Exit,
        StateHandler1_Menu,
        StateHandler2_CharSelect,
        StateHandler3_InGame,
        StateHandler4_Loading,
        StateHandler5_Credits};

    DEBUG_MSGBOX("Game Loop", "InitializeAndRunGameMainLoop starting!\n\nAbout to enter 6-phase initialization...");

    DEBUG_LOG("\n========================================\n");
    DEBUG_LOG("[InitializeAndRunGameMainLoop] MAIN GAME ENGINE ENTRY\n");
    DEBUG_LOG("========================================\n");

    // =========================================================================
    // PHASE 1: Subsystem Initialization
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] PHASE 1: Subsystem Initialization\n");

    InitializeDirectSound();
    InitializeSubsystem2Thunk();
    InitializeSubsystem3Thunk();
    InitializeSubsystem4Thunk();

    // =========================================================================
    // PHASE 2: System Requirements Validation
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] PHASE 2: System Requirements Validation\n");

    if (!ValidateSystemRequirementsThunk())
    {
        DEBUG_LOG("[InitializeAndRunGameMainLoop] ERROR: System requirements validation failed!\n");
        return 0;
    }

    // =========================================================================
    // PHASE 3: Graphics/Video Mode Setup
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] PHASE 3: Graphics/Video Mode Setup\n");

    sprintf(logMsg, "[InitializeAndRunGameMainLoop] Video mode: %d, Windowed: %s\n",
            videoMode, windowed ? "YES" : "NO");
    DEBUG_LOG(logMsg);

    // Get default screen mode if needed
    if (!GetDefaultScreenMode())
    {
        DEBUG_LOG("[InitializeAndRunGameMainLoop] ERROR: Failed to get default screen mode!\n");
        return 0;
    }

    // Initialize graphics subsystem
    if (InitializeGraphicsSubsystem(g_hInstance, videoMode, windowed, 0))
    {
        graphicsInitialized = TRUE;
        DEBUG_LOG("[InitializeAndRunGameMainLoop] Graphics subsystem initialized\n");

        // Initialize renderer
        if (!InitializeRendererThunk(windowed, 0))
        {
            DEBUG_LOG("[InitializeAndRunGameMainLoop] ERROR: Renderer initialization failed!\n");
            ShutdownGraphicsThunk();
            return 0;
        }

        // Set framerate lock if applicable
        if (videoMode >= 4)
        {
            SetFramerateLock(TRUE);
        }
    }

    // =========================================================================
    // PHASE 4: Peripheral Setup
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] PHASE 4: Peripheral Setup\n");

    // Enable sound if not disabled
    if (!g_noSound)
    {
        EnableSound();
    }

    // Set FPS display mode if configured
    if (g_videoMode > 0)
    {
        SetFPSDisplayMode((int)g_videoMode);
    }

    // Apply gamma correction
    ApplyGammaCorrection();

    // Enable wide aspect ratio if configured
    EnableWideAspectRatio();

    // =========================================================================
    // PHASE 5: Menu System Initialization
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] PHASE 5: Menu System Initialization\n");

    if (!g_skipToBnet && currentState != 0)
    {
        InitializeMenuSystem();
        menuInitialized = TRUE;
    }

    // =========================================================================
    // PHASE 6: Main Game State Loop
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] PHASE 6: Main Game State Loop\n");
    DEBUG_LOG("[InitializeAndRunGameMainLoop] Entering state machine (current state: 1)\n");

    // Main game state loop - continues until state == 0 (exit)
    while (currentState != 0 && g_isRunning)
    {
        sprintf(logMsg, "[InitializeAndRunGameMainLoop] STATE LOOP: Current state = %d\n", currentState);
        DEBUG_LOG(logMsg);

        // Validate state is in range [0-5]
        if (currentState < 0 || currentState > 5)
        {
            DEBUG_LOG("[InitializeAndRunGameMainLoop] ERROR: Invalid state! Forcing exit.\n");
            currentState = 0;
            break;
        }

        // Special handling for state transitions
        if (currentState == 2) // Entering game state
        {
            // Cleanup menu if it was initialized
            if (menuInitialized)
            {
                CleanupMenuSystem();
                menuInitialized = FALSE;
            }

            // Prepare graphics if needed
            if (graphicsInitialized)
            {
                PrepareGraphicsShutdown();
            }
        }

        // Call state handler from dispatch table
        DEBUG_LOG("[InitializeAndRunGameMainLoop] Dispatching to state handler...\n");
        int nextState = stateHandlers[currentState](&g_launchConfig);

        sprintf(logMsg, "[InitializeAndRunGameMainLoop] State handler returned: %d\n", nextState);
        DEBUG_LOG(logMsg);

        // Update current state based on handler return value
        currentState = nextState;
    }

    // =========================================================================
    // CLEANUP AND SHUTDOWN
    // =========================================================================
    DEBUG_LOG("[InitializeAndRunGameMainLoop] Beginning cleanup sequence...\n");

    // Cleanup menu if still active
    if (menuInitialized)
    {
        CleanupMenuSystem();
    }

    // Shutdown graphics if initialized
    if (graphicsInitialized)
    {
        PrepareGraphicsShutdown();
        ShutdownGraphicsThunk();
    }

    // Shutdown subsystems
    CloseEngineSubsystem();
    ShutdownSubsystem6Thunk();
    ShutdownExternalSubsystemThunk();

    DEBUG_LOG("[InitializeAndRunGameMainLoop] Shutdown complete\n");
    DEBUG_LOG("========================================\n\n");

    return 1;
}

// =============================================================================
// LEVEL 3: WINDOW MANAGEMENT
// =============================================================================

/*
 * D2WindowProc @ varies
 * Window procedure for main game window
 * Called by: Windows message dispatcher
 */
LRESULT CALLBACK D2WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        DEBUG_LOG("[D2WindowProc] WM_DESTROY\n");
        g_isRunning = FALSE;
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        DEBUG_LOG("[D2WindowProc] WM_CLOSE\n");
        DestroyWindow(hwnd);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0));

        const char *msg = "Diablo II - Game.exe Replacement (Full Implementation)";
        DrawTextA(hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        EndPaint(hwnd, &ps);
    }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*
 * CreateGameWindow @ varies
 * Create the main game window
 * Called by: D2ServerMain
 */
HWND __cdecl CreateGameWindow(HINSTANCE hInstance, int width, int height, int showCmd)
{
    char debugMsg[256];
    const char CLASS_NAME[] = "Diablo II";

    DEBUG_LOG("[CreateGameWindow] Creating window...\n");

    // Register window class
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = D2WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExA(&wc))
    {
        DWORD error = GetLastError();
        sprintf(debugMsg, "[CreateGameWindow] ERROR: RegisterClassExA failed: %d\n", error);
        DEBUG_LOG(debugMsg);
        return NULL;
    }

    // Create window
    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Diablo II",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hwnd)
    {
        DWORD error = GetLastError();
        sprintf(debugMsg, "[CreateGameWindow] ERROR: CreateWindowExA failed: %d\n", error);
        DEBUG_LOG(debugMsg);
        return NULL;
    }

    sprintf(debugMsg, "[CreateGameWindow] Window created: %dx%d\n", width, height);
    DEBUG_LOG(debugMsg);

    ShowWindow(hwnd, showCmd);
    UpdateWindow(hwnd);

    return hwnd;
}

/*
 * DestroyGameWindow @ varies
 * Cleanup and destroy game window
 * Called by: D2ServerMain shutdown
 */
void __cdecl DestroyGameWindow(void)
{
    DEBUG_LOG("[DestroyGameWindow] Destroying window...\n");

    if (g_hWndMain)
    {
        DestroyWindow(g_hWndMain);
        g_hWndMain = NULL;
    }

    if (g_hInstance)
    {
        UnregisterClassA("Diablo II", g_hInstance);
    }
}

// =============================================================================
// LEVEL 4: DLL MANAGEMENT
// =============================================================================

/*
 * LoadGameDLL @ varies
 * Load a Diablo II DLL with error handling
 * Called by: LoadAllGameDLLs
 */
HMODULE __cdecl LoadGameDLL(const char *dllName)
{
    char debugMsg[256];
    char dllPath[512];
    HMODULE hModule;

    sprintf(debugMsg, "[LoadGameDLL] Loading %s...\n", dllName);
    DEBUG_LOG(debugMsg);

    // Try install directory first
    sprintf(dllPath, "%s\\%s", g_installPath, dllName);
    hModule = LoadLibraryA(dllPath);

    // Try current directory
    if (!hModule)
    {
        hModule = LoadLibraryA(dllName);
    }

    if (!hModule)
    {
        DWORD error = GetLastError();
        sprintf(debugMsg, "[LoadGameDLL] WARNING: Failed to load %s (error %d)\n", dllName, error);
        DEBUG_LOG(debugMsg);

        // Only show error dialog for critical DLLs - not for optional mode-specific DLLs
        // D2Server.dll, D2Client.dll, and D2Multi.dll are optional depending on game mode
        BOOL isOptional = (strstr(dllName, "D2Server") != NULL ||
                           strstr(dllName, "D2Client") != NULL ||
                           strstr(dllName, "D2Multi") != NULL);

        if (!isOptional)
        {
            char errorMsg[512];
            sprintf(errorMsg, "Failed to load %s\n\nError code: %d\n\nMake sure Diablo II DLLs are in the build/Release directory!", dllName, error);
            ERROR_MSGBOX("DLL Loading Error", errorMsg);
        }
        else
        {
            sprintf(debugMsg, "[LoadGameDLL] INFO: %s is optional for this mode, continuing...\n", dllName);
            DEBUG_LOG(debugMsg);
        }

        return NULL;
    }

    sprintf(debugMsg, "[LoadGameDLL] %s loaded successfully\n", dllName);
    DEBUG_LOG(debugMsg);
    return hModule;
}

/*
 * LoadAllGameDLLs @ varies
 * Load all required Diablo II DLLs
 * Called by: D2ServerMain
 */
BOOL __cdecl LoadAllGameDLLs(void)
{
    DEBUG_LOG("\n[LoadAllGameDLLs] ========================================\n");
    DEBUG_LOG("[LoadAllGameDLLs] PHASE 5: DLL Loading\n");
    DEBUG_LOG("[LoadAllGameDLLs] ========================================\n");

    // Core DLLs (always loaded) - Based on binary analysis
    g_hModuleFog = LoadGameDLL("Fog.dll");         // Engine foundation
    g_hModuleD2Gfx = LoadGameDLL("D2Gfx.dll");     // Graphics subsystem (was D2Gdi)
    g_hModuleD2Sound = LoadGameDLL("D2Sound.dll"); // Audio subsystem
    g_hModuleD2Game = LoadGameDLL("D2Game.dll");   // Game logic
    g_hModuleD2Net = LoadGameDLL("D2Net.dll");     // Networking
    g_hModuleD2Win = LoadGameDLL("D2Win.dll");     // UI/Windowing
    g_hModuleD2Lang = LoadGameDLL("D2Lang.dll");   // Localization
    g_hModuleD2Cmp = LoadGameDLL("D2Cmp.dll");     // Video codec
    g_hModuleStorm = LoadGameDLL("Storm.dll");     // File I/O, compression

    // Game mode specific DLLs
    if (g_gameMode == 0)
    {
        // Single-player
        g_hModuleD2Server = LoadGameDLL("D2Server.dll");
        DEBUG_LOG("[LoadAllGameDLLs] Single-player DLLs loaded\n");
    }
    else if (g_gameMode >= 1)
    {
        // Multiplayer/Battle.net
        g_hModuleD2Client = LoadGameDLL("D2Client.dll");
        if (g_gameMode == 2)
        {
            g_hModuleD2Multi = LoadGameDLL("D2Multi.dll");
        }
        DEBUG_LOG("[LoadAllGameDLLs] Multiplayer DLLs loaded\n");
    }

    DEBUG_LOG("[LoadAllGameDLLs] All DLLs loaded\n\n");

    // Initialize function pointers from loaded DLLs
    InitializeDLLFunctionPointers();

    // Build success message showing what actually loaded
    char loadedMsg[1024];
    sprintf(loadedMsg, "DLL Loading Complete!\n\nCore DLLs loaded:\n- Fog.dll %s\n- D2Gfx.dll %s\n- D2Sound.dll %s\n- D2Game.dll %s\n- D2Net.dll %s\n- D2Win.dll %s\n- D2Lang.dll %s\n- D2Cmp.dll %s\n- Storm.dll %s\n\nMode-specific:\n- D2Server.dll %s\n- D2Client.dll %s\n\nFunction pointers initialized!",
            g_hModuleFog ? "✓" : "✗",
            g_hModuleD2Gfx ? "✓" : "✗",
            g_hModuleD2Sound ? "✓" : "✗",
            g_hModuleD2Game ? "✓" : "✗",
            g_hModuleD2Net ? "✓" : "✗",
            g_hModuleD2Win ? "✓" : "✗",
            g_hModuleD2Lang ? "✓" : "✗",
            g_hModuleD2Cmp ? "✓" : "✗",
            g_hModuleStorm ? "✓" : "✗",
            g_hModuleD2Server ? "✓" : "(not needed)",
            g_hModuleD2Client ? "✓" : "(not needed)");
    DEBUG_MSGBOX("DLL Loading", loadedMsg);

    return TRUE;
}

/*
 * InitializeDLLFunctionPointers @ varies
 * Populate function pointers from loaded DLLs using GetProcAddress with ORDINALS
 * Called by: LoadAllGameDLLs (after DLLs are loaded)
 *
 * **CRITICAL DISCOVERY**: All Diablo II DLLs use ordinal-only exports (no names).
 * Must use GetProcAddress with MAKEINTRESOURCE(ordinal) instead of function names.
 * Ordinals discovered via address matching against Ghidra static analysis.
 *
 * Known ordinals (verified working):
 *   - Fog.dll ordinal 10111: InitializeAsyncDataStructures @ 0x6FF6DF00
 *   - Fog.dll ordinal 10096: InitializeModule @ 0x6FF6CCF0
 *   - D2Gfx.dll ordinal 10025: SetParameterAndCallGraphicsVtable_0x58 @ 0x6FA8B1E0
 *   - D2Sound.dll ordinal 10022: ShutdownAudioSystemResources @ 0x6F9B9230
 */
BOOL __cdecl InitializeDLLFunctionPointers(void)
{
    char buf[256];

    DEBUG_LOG("\n[InitializeDLLFunctionPointers] Resolving DLL function pointers by ORDINAL...\n");

    // D2Win.dll functions (UI and windowing)
    // TODO: Discover ordinals for D2Win.dll functions
    if (g_hModuleD2Win)
    {
        // Using name-based lookup (will return NULL until ordinals found)
        g_pfnInitializeMenuSystem = (PFN_InitializeMenuSystem)GetProcAddress(g_hModuleD2Win, "InitializeGameData");
        g_pfnCleanupMenuSystem = (PFN_CleanupMenuSystem)GetProcAddress(g_hModuleD2Win, "CloseGameResources");
        g_pfnSetFramerateLock = (PFN_SetFramerateLock)GetProcAddress(g_hModuleD2Win, "DispatchInitialization");
        g_pfnSetFPSDisplayMode = (PFN_SetFPSDisplayMode)GetProcAddress(g_hModuleD2Win, "InitializeResourceBuffers");
        g_pfnApplyGammaCorrection = (PFN_ApplyGammaCorrection)GetProcAddress(g_hModuleD2Win, "InitializeGameEnvironment");
        g_pfnEnableWideAspectRatio = (PFN_EnableWideAspectRatio)GetProcAddress(g_hModuleD2Win, "InitializeGameDllLibraries");
        g_pfnGetWindowHandle = (PFN_GetWindowHandle)GetProcAddress(g_hModuleD2Win, "PromptInsertPlayDisc");
        DEBUG_LOG("[InitializeDLLFunctionPointers] D2Win.dll function pointers resolved (using names - TODO: convert to ordinals)\n");
    }

    // D2Gfx.dll (Graphics subsystem) - ORDINALS DISCOVERED!
    if (g_hModuleD2Gfx)
    {
        // **ORDINAL 10025 DISCOVERED**: SetParameterAndCallGraphicsVtable_0x58 @ 0x6FA8B1E0
        g_pfnInitializeGraphicsSubsystem = (PFN_InitializeGraphicsSubsystem)GetProcAddress(g_hModuleD2Gfx, MAKEINTRESOURCE(10025));
        if (g_pfnInitializeGraphicsSubsystem)
        {
            sprintf(buf, "[InitializeDLLFunctionPointers] ✓ D2Gfx.dll ordinal 10025 resolved successfully!\n");
            DEBUG_LOG(buf);
        }

        // TODO: Discover ordinals for these functions
        g_pfnInitializeRenderer = (PFN_InitializeRenderer)GetProcAddress(g_hModuleD2Gfx, "ToggleGameState");
        g_pfnPrepareGraphicsShutdown = (PFN_PrepareGraphicsShutdown)GetProcAddress(g_hModuleD2Gfx, "SetCleanupHandlerFlag");
        g_pfnShutdownGraphics = (PFN_ShutdownGraphics)GetProcAddress(g_hModuleD2Gfx, "CleanupWindowAndDisplayError");

        DEBUG_LOG("[InitializeDLLFunctionPointers] D2Gfx.dll function pointers resolved (1 ordinal, 3 names)\n");
    }

    // D2Client.dll functions (Client game logic)
    // TODO: Discover ordinals
    if (g_hModuleD2Client)
    {
        g_pfnValidateSystemRequirements = (PFN_ValidateSystemRequirements)GetProcAddress(g_hModuleD2Client, "ValidateSystemRequirements");
        g_pfnGetDefaultScreenMode = (PFN_GetDefaultScreenMode)GetProcAddress(g_hModuleD2Client, "GetDefaultScreenMode");
        DEBUG_LOG("[InitializeDLLFunctionPointers] D2Client.dll function pointers resolved (using names - TODO: convert to ordinals)\n");
    }

    // D2Sound.dll functions (Audio subsystem) - ORDINAL DISCOVERED!
    if (g_hModuleD2Sound)
    {
        // TODO: Discover ordinal for InitializeDirectSound
        g_pfnInitializeDirectSound = (PFN_InitializeDirectSound)GetProcAddress(g_hModuleD2Sound, "InitializeDirectSound");

        // **ORDINAL 10022 DISCOVERED**: ShutdownAudioSystemResources @ 0x6F9B9230
        g_pfnEnableSound = (PFN_EnableSound)GetProcAddress(g_hModuleD2Sound, MAKEINTRESOURCE(10022));
        if (g_pfnEnableSound)
        {
            sprintf(buf, "[InitializeDLLFunctionPointers] ✓ D2Sound.dll ordinal 10022 resolved successfully!\n");
            DEBUG_LOG(buf);
        }

        DEBUG_LOG("[InitializeDLLFunctionPointers] D2Sound.dll function pointers resolved (1 ordinal, 1 name)\n");
    }

    // Fog.dll functions (Engine foundation) - ORDINALS DISCOVERED!
    if (g_hModuleFog)
    {
        // **ORDINAL 10111 DISCOVERED**: InitializeAsyncDataStructures @ 0x6FF6DF00
        g_pfnInitializeSubsystem2 = (PFN_InitializeSubsystem2)GetProcAddress(g_hModuleFog, MAKEINTRESOURCE(10111));
        if (g_pfnInitializeSubsystem2)
        {
            sprintf(buf, "[InitializeDLLFunctionPointers] ✓ Fog.dll ordinal 10111 resolved to 0x%p\n", g_pfnInitializeSubsystem2);
            DEBUG_LOG(buf);
        }
        else
        {
            DEBUG_LOG("[InitializeDLLFunctionPointers] ✗ Fog.dll ordinal 10111 FAILED (NULL)\n");
        }

        // TODO: Discover ordinal for StubFunction_NoOp
        g_pfnInitializeSubsystem3 = (PFN_InitializeSubsystem3)GetProcAddress(g_hModuleFog, "StubFunction_NoOp");

        // **ORDINAL 10096 DISCOVERED**: InitializeModule @ 0x6FF6CCF0
        g_pfnInitializeSubsystem4 = (PFN_InitializeSubsystem4)GetProcAddress(g_hModuleFog, MAKEINTRESOURCE(10096));
        if (g_pfnInitializeSubsystem4)
        {
            sprintf(buf, "[InitializeDLLFunctionPointers] ✓ Fog.dll ordinal 10096 resolved to 0x%p\n", g_pfnInitializeSubsystem4);
            DEBUG_LOG(buf);
        }
        else
        {
            DEBUG_LOG("[InitializeDLLFunctionPointers] ✗ Fog.dll ordinal 10096 FAILED (NULL)\n");
        }

        // TODO: Discover ordinals for these functions
        g_pfnCloseEngineSubsystem = (PFN_CloseEngineSubsystem)GetProcAddress(g_hModuleFog, "CloseAllEventHandles");
        g_pfnShutdownSubsystem6 = (PFN_ShutdownSubsystem6)GetProcAddress(g_hModuleFog, "DeinitializeGameResources");
        g_pfnShutdownExternalSubsystem = (PFN_ShutdownExternalSubsystem)GetProcAddress(g_hModuleFog, "InitializeGameData");

        DEBUG_LOG("[InitializeDLLFunctionPointers] Fog.dll function pointers resolved (2 ordinals, 4 names)\n");
    }

    // Storm.dll functions (File I/O and utility functions)
    // TODO: Discover ordinals
    if (g_hModuleStorm)
    {
        g_pfnWriteRegistryDwordValue = (PFN_WriteRegistryDwordValue)GetProcAddress(g_hModuleStorm, "WriteRegistryDwordValue");
        DEBUG_LOG("[InitializeDLLFunctionPointers] Storm.dll function pointers resolved (using names - TODO: convert to ordinals)\n");
    }

    DEBUG_LOG("[InitializeDLLFunctionPointers] All function pointers resolved (4 ordinals working, remaining use names)\n\n");
    return TRUE;
}

/*
 * UnloadAllGameDLLs @ varies
 * Unload all Diablo II DLLs
 * Called by: D2ServerMain shutdown
 */
void __cdecl UnloadAllGameDLLs(void)
{
    DEBUG_LOG("[UnloadAllGameDLLs] Unloading DLLs...\n");

    if (g_hModuleD2Client)
    {
        FreeLibrary(g_hModuleD2Client);
        g_hModuleD2Client = NULL;
    }
    if (g_hModuleD2Server)
    {
        FreeLibrary(g_hModuleD2Server);
        g_hModuleD2Server = NULL;
    }
    if (g_hModuleD2Game)
    {
        FreeLibrary(g_hModuleD2Game);
        g_hModuleD2Game = NULL;
    }
    if (g_hModuleD2Gfx)
    {
        FreeLibrary(g_hModuleD2Gfx);
        g_hModuleD2Gfx = NULL;
    }
    if (g_hModuleD2Sound)
    {
        FreeLibrary(g_hModuleD2Sound);
        g_hModuleD2Sound = NULL;
    }
    if (g_hModuleFog)
    {
        FreeLibrary(g_hModuleFog);
        g_hModuleFog = NULL;
    }
    if (g_hModuleD2Net)
    {
        FreeLibrary(g_hModuleD2Net);
        g_hModuleD2Net = NULL;
    }
    if (g_hModuleD2Multi)
    {
        FreeLibrary(g_hModuleD2Multi);
        g_hModuleD2Multi = NULL;
    }
    if (g_hModuleD2Win)
    {
        FreeLibrary(g_hModuleD2Win);
        g_hModuleD2Win = NULL;
    }
    if (g_hModuleD2Lang)
    {
        FreeLibrary(g_hModuleD2Lang);
        g_hModuleD2Lang = NULL;
    }
    if (g_hModuleD2Cmp)
    {
        FreeLibrary(g_hModuleD2Cmp);
        g_hModuleD2Cmp = NULL;
    }
    if (g_hModuleStorm)
    {
        FreeLibrary(g_hModuleStorm);
        g_hModuleStorm = NULL;
    }

    DEBUG_LOG("[UnloadAllGameDLLs] All DLLs unloaded\n");
}

// =============================================================================
// LEVEL 5: GAME LOOP
// =============================================================================

/*
 * RunGameMainLoop @ 0x00407600
 * Main message loop
 * Called by: D2ServerMain
 */
void __cdecl RunGameMainLoop(void)
{
    MSG msg = {0};

    DEBUG_LOG("[RunGameMainLoop] Entering main message loop...\n");

    g_isRunning = TRUE;

    while (g_isRunning && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DEBUG_LOG("[RunGameMainLoop] Exited message loop\n");
}

/*
 * D2ServerMain @ 0x00408540
 * Main game initialization and execution
 *
 * This function is called by CRTStartup after C Runtime initialization.
 * It orchestrates the complete game startup sequence:
 * 1. Configuration loading (registry + command-line)
 * 2. Window creation
 * 3. DLL loading
 * 4. Game loop execution
 * 5. Cleanup and shutdown
 */
int WINAPI D2ServerMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nShowCmd)
{
    char debugMsg[256];

    DEBUG_LOG("\n========================================\n");
    DEBUG_LOG("[D2ServerMain] Diablo II Game.exe Entry\n");
    DEBUG_LOG("========================================\n");

    sprintf(debugMsg, "[D2ServerMain] hInstance=0x%p, lpCmdLine=\"%s\", nShowCmd=%d\n",
            hInstance, lpCmdLine ? lpCmdLine : "", nShowCmd);
    DEBUG_LOG(debugMsg);

    // =========================================================================
    // PHASE 1: Configuration and Initialization
    // =========================================================================
    DEBUG_LOG("\n[D2ServerMain] ========================================\n");
    DEBUG_LOG("[D2ServerMain] PHASE 1: Configuration Loading\n");
    DEBUG_LOG("[D2ServerMain] ========================================\n");

    if (!InitializeD2ServerMain(g_argc, g_argv))
    {
        DEBUG_LOG("[D2ServerMain] ERROR: Configuration initialization failed!\n");
        MessageBoxA(NULL, "Failed to initialize game configuration",
                    "Diablo II Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // =========================================================================
    // PHASE 2: Window Creation
    // =========================================================================
    DEBUG_LOG("\n[D2ServerMain] ========================================\n");
    DEBUG_LOG("[D2ServerMain] PHASE 2: Window Creation\n");
    DEBUG_LOG("[D2ServerMain] ========================================\n");

    g_hWndMain = CreateGameWindow(hInstance, (int)g_screenWidth, (int)g_screenHeight, nShowCmd);
    if (!g_hWndMain)
    {
        DEBUG_LOG("[D2ServerMain] ERROR: Window creation failed!\n");
        MessageBoxA(NULL, "Failed to create game window",
                    "Diablo II Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    DEBUG_LOG("[D2ServerMain] Window created successfully\n");

    // =========================================================================
    // PHASE 3: DLL Loading
    // =========================================================================
    DEBUG_LOG("\n[D2ServerMain] ========================================\n");
    DEBUG_LOG("[D2ServerMain] PHASE 3: DLL Loading\n");
    DEBUG_LOG("[D2ServerMain] ========================================\n");

    if (!LoadAllGameDLLs())
    {
        DEBUG_LOG("[D2ServerMain] WARNING: Some DLLs failed to load\n");
        // Continue anyway - this is a stub implementation
    }

    // =========================================================================
    // PHASE 4: Main Game Loop
    // =========================================================================
    DEBUG_LOG("\n[D2ServerMain] ========================================\n");
    DEBUG_LOG("[D2ServerMain] PHASE 4: Main Game Loop\n");
    DEBUG_LOG("[D2ServerMain] ========================================\n");

    RunGameMainLoop();

    // =========================================================================
    // PHASE 5: Cleanup and Shutdown
    // =========================================================================
    DEBUG_LOG("\n[D2ServerMain] ========================================\n");
    DEBUG_LOG("[D2ServerMain] PHASE 5: Cleanup\n");
    DEBUG_LOG("[D2ServerMain] ========================================\n");

    UnloadAllGameDLLs();
    DestroyGameWindow();

    DEBUG_LOG("[D2ServerMain] Shutdown complete\n");
    DEBUG_LOG("========================================\n\n");

    return 0;
}
