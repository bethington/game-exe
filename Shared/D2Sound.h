#pragma once
// #pragma comment(lib,"D2Sound") // Disabled for OpenD2 build
#include "D2Shared.hpp"
#define D2_API extern "C" __declspec(dllexport)

D2_API void __fastcall D2SOUND_10000(DWORD);
D2_API void __fastcall D2SOUND_10001(void);