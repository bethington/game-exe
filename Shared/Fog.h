#pragma once
// #pragma comment(lib,"Fog") // Disabled for OpenD2 build
#include "D2Shared.hpp"

#define D2_API extern "C" __declspec(dllexport)

D2_API void __fastcall FOG_10019(void *, DWORD, void *, DWORD);
D2_API void __fastcall FOG_10021(void *);
D2_API void __fastcall FOG_10053(void);
D2_API void __fastcall FOG_10089(DWORD, DWORD);
D2_API void __fastcall FOG_10090(void);
D2_API void __fastcall FOG_10101(DWORD, DWORD);
// D2_API int __cdecl FOG_10117() <-- gets value of -direct
D2_API void __cdecl FOG_10143(void *);
D2_API void __fastcall FOG_10198(DWORD, void *, double, int, int, int);
D2_API void __fastcall FOG_10218(void);
D2_API DWORD __fastcall FOG_10227(void);