#include "CommonIncludes.hpp"

#include "Utils.hpp"
#include "OBS_Hook.hpp"

HMODULE hM;

namespace OBS_Rehook
{
	void Initialize()
	{
		// Initialize
		Utils::LogSystem_Initialize();

		// Wait until OBS actually loads...
		Utils::Log("Looking for OBS module...");
		
		while (!GetModuleHandleA("graphics-hook32.dll"))
			Sleep(100);

		Sleep(1000);

		SetupHook();

		// We can exit this thread now...
		Utils::Log("Direct3D Present is now hooked. (after OBS Studio recording)");
		Utils::Log("This window can be safely closed...");

		//Utils::LogSystem_Cleanup();
		//FreeLibraryAndExitThread(hM, NULL);
	}
}

int __stdcall DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		hM = hModule;
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)OBS_Rehook::Initialize, NULL, NULL, NULL);
	}
	return TRUE;
}
