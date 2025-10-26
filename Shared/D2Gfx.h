#pragma once
// #pragma comment(lib,"D2Gfx") // Disabled for OpenD2 build
#include "D2Shared.hpp"
#define D2_API extern "C" __declspec(dllexport)

D2_API void __stdcall D2GFX_10001(void);
D2_API void __stdcall D2GFX_10011(DWORD);
D2_API void __stdcall D2GFX_10015(void);
D2_API void __stdcall D2GFX_10018(DWORD);
D2_API void __stdcall D2GFX_10020(void);
