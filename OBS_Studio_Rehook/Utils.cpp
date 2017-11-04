#include "Utils.hpp"

namespace Utils
{
	// No logging in release mode :^)
#ifdef _DEBUG
	void LogSystem_Initialize()
	{
		AllocConsole();

		// redirect 
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		char nBuffer[128];
		sprintf(nBuffer, "%s - Build from %s @ %s", "OBS_Example", __DATE__, __TIME__);
		SetConsoleTitleA(nBuffer);
	}

	void LogSystem_Cleanup()
	{
		FreeConsole();
	}

	std::string __forceinline GetTimeString()
	{
		//Time related variables
		time_t current_time;
		struct tm *time_info;
		static char timeString[10];

		//Get current time
		time(&current_time);
		time_info = localtime(&current_time);

		//Get current time as string
		strftime(timeString, sizeof(timeString), "%I:%M %p", time_info);
		return timeString;
	}

	void Log(char* fmt, ...)
	{
		va_list va_alist;
		char logBuf[256] = { 0 };

		//Do sprintf with the parameters
		va_start(va_alist, fmt);
		_vsnprintf(logBuf + strlen(logBuf), sizeof(logBuf) - strlen(logBuf), fmt, va_alist);
		va_end(va_alist);

		//Output to console
		if (logBuf[0] != '\0')
		{

			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN | FOREGROUND_INTENSITY));
			printf("[%s]", GetTimeString().c_str());
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN));
			printf(": %s\n", logBuf);
		}
	}
#endif

	std::uintptr_t FindSignature(const char* szModule, const char* szSignature)
	{
		auto module = GetModuleHandleA(szModule);

		static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; ++current) {
				if (*current == '?') {
					++current;
					if (*current == '?')
						++current;
					bytes.push_back(-1);
				}
				else {
					bytes.push_back(strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

		auto dosHeader = (PIMAGE_DOS_HEADER)module;
		auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

		auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		auto patternBytes = pattern_to_byte(szSignature);
		auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

		auto s = patternBytes.size();
		auto d = patternBytes.data();

		for (auto i = 0ul; i < sizeOfImage - s; ++i) {
			bool found = true;
			for (auto j = 0ul; j < s; ++j) {
				if (scanBytes[i + j] != d[j] && d[j] != -1) {
					found = false;
					break;
				}
			}
			if (found) {
				return (std::uintptr_t)&scanBytes[i];
			}
		}
		return (std::uintptr_t)nullptr;
	}
}
