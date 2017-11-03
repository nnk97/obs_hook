#pragma once

#include <memory>
#include <thread>
#include <chrono>

#include <Windows.h>

#include "Direct3DIncludes.hpp"

#ifdef Sleep
	#undef Sleep
#endif

#define Sleep(dur) std::this_thread::sleep_for(std::chrono::milliseconds(dur))
