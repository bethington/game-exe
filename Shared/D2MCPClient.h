#pragma once
// #pragma comment(lib,"D2Client") // Disabled for OpenD2 build
#include "D2Shared.hpp"
#define D2_API extern "C" __declspec(dllimport)

D2_API void __fastcall D2MCPCLIENT_10001(void);