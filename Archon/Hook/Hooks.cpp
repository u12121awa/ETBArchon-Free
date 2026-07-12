#include "pch.h"
#include "Hooks.h"

#include "DX12Hook.h"
#include "FunctionHook.h"
#include "GameRenderHook.h"
#include "../Archon.h"
#include "../Core/Memory.h"
#include "../Core/Managers/ActorsFindManager.h"
#include "../Core/Managers/SoundManager.h"
#include "../Core/Module/features.h"
#include "../Utils/Auth/Auth.h"
#include "../Utils/World/FTickCallbackSystem.h"
#pragma comment(lib, "Winmm.lib")

bool Hooks::onInitSDK()
{
	VMProtectBeginUltra("Initialize SDK");
	
	Log::Info(xorstr_("Injected! Initializing SDK..."));
	
	const char* addr = Auth::IsAuth ? xorstr_("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1") : xorstr_("13 78 91");
	
	uintptr_t GObjectsAddress = Memory::FindPattern(addr); //or Offsets::GObjects 
	if (!GObjectsAddress) {
		Log::Error(xorstr_("Failed to find GObjects pattern!"));
		VMProtectEnd();
		return false;
	}
	GObjectsAddress = GObjectsAddress + *reinterpret_cast<int32_t*>(GObjectsAddress + 3) + 7;
	UObject::GObjects.InitManually(reinterpret_cast<void*>(GObjectsAddress));

	if (!UObject::GObjects.GetTypedPtr()) {
		Log::Error(xorstr_("GObjects pointer is null after manual initialization!"));
		VMProtectEnd();
		return false;
	}
	
	Log::Success(xorstr_("SDK Initialized! Found ") + std::to_string(UObject::GObjects->Num()) + xorstr_(" objects."));
	VMProtectEnd();
	return true;
}

bool Hooks::Initialize()
{
	VMProtectBeginUltra("Initialize Hooks");

	if (!Auth::PerformAuth())
	{
		MessageBoxA(NULL, xorstr_("客户端不可用!"), xorstr_("Error"), MB_OK | MB_ICONERROR);
		VMProtectEnd();
		return false;
	}
	
	
	if (!onInitSDK())
	{
		VMProtectEnd();
		return false;
	}

	Instances::Initialize();
	
	if (MH_Initialize() != MH_OK){
		Log::Error(xorstr_("Failed to initialize MinHook!"));
		VMProtectEnd();
		return false;
	}
	
	FunctionHook::Initialize();
	GameRenderHook::Initialize();
	
	if (!DX12Hook::Initialize())
	{
		VMProtectEnd();
		return false;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		Log::Error(xorstr_("EnableHook failed!"));
		VMProtectEnd();
		return false;
	}
	
	Log::Success(xorstr_("Hooks initialized successfully!"));
	
	VMProtectEnd();
	return true;
}

void Hooks::Shutdown()
{
	VMProtectBeginVirtualization("Shutdown All");
	
	ModuleManager::Get().Shutdown();
	Archon::GetMCChatSystem()->ResetChat();
	Archon::GetWorldManager()->Reset();
	Archon::GetRotationManager()->OnStop();
	Archon::GetConfigManager()->Save(xorstr_("Default"));
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	GameRenderHook::Shutdown();
	DX12Hook::Shutdown();
	SoundManager::Get().Shutdown();
	
	VMProtectEnd();
}



