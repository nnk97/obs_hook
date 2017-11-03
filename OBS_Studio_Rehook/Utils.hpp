#pragma once

#include <string>
#include <fstream>
#include <vector>

#include <time.h>
#include <stdio.h>
#include <Windows.h>

namespace Utils
{
	// Logging system
#ifdef _DEBUG
	void LogSystem_Initialize();
	void LogSystem_Cleanup();
	void Log(char* fmt, ...);
#else
	void __forceinline LogSystem_Initialize() { };
	void __forceinline LogSystem_Cleanup() { };
	void __forceinline Log(char* fmt, ...) { };
#endif

	// Pattern scanning
	std::uintptr_t FindSignatureFn(const char* szModule, const char* szSignature);
}
