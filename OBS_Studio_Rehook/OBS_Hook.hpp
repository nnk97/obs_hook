#pragma once

#include "CommonIncludes.hpp"
#include "Direct3DIncludes.hpp"
#include "Utils.hpp"

namespace OBS_Rehook
{
	extern void SetupHook();
	extern uintptr_t obs_hook_address, backup_address;
}