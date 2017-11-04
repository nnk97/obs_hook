#include "OBS_Hook.hpp"

namespace OBS_Rehook
{
	/*
	int __stdcall hkPresent__CBaseDevice(int pDevice, int a2, int a3, int a4, int a5)
	{
		int hr; // eax@4
		int return_value; // ebx@8
		int backbuffer_1; // edi@8
		int backbuffer; // [sp+Ch] [bp-4h]@1

		backbuffer = 0;
		if ( !hooked_reset )
			setup_reset_hooks(pDevice);
		
		if ( !present_recurse )
		{
			hr = get_backbuffer((int *)pDevice, (int)&backbuffer);
			if ( hr < 0 )
				hlog_hr((int)"d3d9_shmem_capture: Failed to get backbuffer", hr);
			if ( !*((_BYTE *)global_hook_info + 0x3A) ) // if (!global_hook_info->capture_overlay) {
				d3d9_capture(pDevice, backbuffer);
		}
		++present_recurse;
		
		unhook((DWORD)&hook_data_Present);
		return_value = hook_data_Present(pDevice, a2, a3, a4, a5);// hr = call(device, src_rect, dst_rect, override_window, dirty_region);	// we draw in there, just after obs captures the images ^^
		rehook((DWORD)&hook_data_Present, 0);																								
		
		backbuffer_1 = backbuffer;
		if ( !--present_recurse )
		{
			if ( *((_BYTE *)global_hook_info + 0x3A) )
				d3d9_capture(pDevice, backbuffer);
			if ( backbuffer_1 )
				(*(void (__stdcall **)(int))(*(_DWORD *)backbuffer_1 + 8))(backbuffer_1);// backbuffer->Release();							// we also hook this to restore correct capture_interface boolean
		}
		return return_value;
	}
	*/

	void ON_PRESENT_CALLBACK(const IDirect3DDevice9* device)
	{

	}

	uintptr_t obs_hook_address = 0, parent_stack = 0, backbuffer_ptr = 0, d3d9_capture_sub = 0, backup_address = 0;
	uintptr_t present_recurse_ptr = 0, global_hook_info_ptr = 0;
	bool org_capture_overlay = false;
	BYTE org_present_recurse = 0;

	struct FakeVTable
	{
	public:
		uintptr_t sub_0 = 0;
		uintptr_t sub_1 = 0;
		uintptr_t sub_2 = 0;
	};
	struct FakeBuffer
	{
	public:
		FakeVTable* vtable = 0;
	};
	FakeBuffer* fake_backbuffer_ptr = nullptr;

	HRESULT __stdcall HookedPresent(uintptr_t device, void* a2, void* a3, HWND a4, void* a5)
	{
		// Call d3d9_capture if it wasn't called already
		if (*reinterpret_cast<bool*>(global_hook_info_ptr + 0x3A))
		{
			using d3d9_capture_prototype = int(__cdecl*)(uintptr_t, uintptr_t);
			reinterpret_cast<d3d9_capture_prototype>(d3d9_capture_sub)(device, *reinterpret_cast<uintptr_t*>(parent_stack - 0x4));
		}

		// Draw our stuff
		ON_PRESENT_CALLBACK(reinterpret_cast<IDirect3DDevice9*>(device));

		// Call original present
		using present_prototype = HRESULT(__stdcall*)(uintptr_t, void*, void*, HWND, void*);
		HRESULT present_result = reinterpret_cast<present_prototype>(backup_address)(device, a2, a3, a4, a5);

		// Backup original backbuffer pointer
		backbuffer_ptr = *reinterpret_cast<uintptr_t*>(parent_stack - 0x4);

		// Put our value on stack instead of original buffer
		*reinterpret_cast<uintptr_t*>(parent_stack - 0x4) = (uintptr_t)fake_backbuffer_ptr;

		// Patch present_recurse so we always hit the vtable call
		org_present_recurse = *reinterpret_cast<BYTE*>(present_recurse_ptr);
		*reinterpret_cast<BYTE*>(present_recurse_ptr) = 1;

		// Patch global_hook_info param so we don't call capture yet, we'll do it later...
		org_capture_overlay = *reinterpret_cast<bool*>(global_hook_info_ptr + 0x3A);
		*reinterpret_cast<bool*>(global_hook_info_ptr + 0x3A) = false;

		return present_result;
	}

	void __declspec(naked) HookedPresetWrapper()
	{
		__asm
		{
			mov parent_stack, ebp
			jmp HookedPresent
		}
	}

	void __stdcall HookedRelease(void* buffer)
	{
		// Restore original values
		*reinterpret_cast<bool*>(global_hook_info_ptr + 0x3A) = org_capture_overlay;
		*reinterpret_cast<uintptr_t*>(parent_stack - 0x4) = backbuffer_ptr;
		*reinterpret_cast<BYTE*>(present_recurse_ptr) = org_present_recurse;

		// Call original backbuffer function
		if (!--*reinterpret_cast<BYTE*>(present_recurse_ptr))
		{
			if (backbuffer_ptr)
			{
				using org_call_prototype = void(__stdcall*)(uintptr_t);
				uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(backbuffer_ptr);
				reinterpret_cast<org_call_prototype>(vtable[2])(backbuffer_ptr);
			}
		}
	}

	void SetupHook()
	{
		fake_backbuffer_ptr = new FakeBuffer();
		fake_backbuffer_ptr->vtable = new FakeVTable();
		fake_backbuffer_ptr->vtable->sub_2 = (uintptr_t)HookedRelease;

		d3d9_capture_sub = Utils::FindSignature("graphics-hook32.dll", "55 8B EC E8 ? ? ? ? 84 C0 74 05 E8 ? ? ? ? 56");

		uintptr_t scan_result = Utils::FindSignature("graphics-hook32.dll", "83 C4 08 FF 05 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 FF 75 18");
		present_recurse_ptr = *reinterpret_cast<uintptr_t*>(scan_result + 0x5);
		global_hook_info_ptr = **reinterpret_cast<uintptr_t**>(scan_result + 0x46);
		obs_hook_address = *reinterpret_cast<uintptr_t*>(scan_result + 0xA);

		backup_address = *reinterpret_cast<uintptr_t*>(obs_hook_address);
		*reinterpret_cast<uintptr_t*>(obs_hook_address) = (uintptr_t)HookedPresetWrapper;
	}
}