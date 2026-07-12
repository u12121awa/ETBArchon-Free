#include "pch.h"
#include "GameRenderHook.h"

#include "../Archon.h"
#include "../Core/Memory.h"
#include "../Core/Instance/Instances.hpp"
#include "../Core/Managers/ActorsFindManager.h"
#include "../Core/Managers/ModuleManager.h"
#include "../Core/Module/features.h"
#include "../Utils/World/EventInterval.h"
#include "../Utils/World/FTickCallbackSystem.h"


constexpr int32 PostRenderIdx = 0x00000064;

PostRender_t origin_renderer;

bool GameRenderHook::Initialize()
{
	VMProtectBeginUltra("Initializing GameRenderHook");
	UWorld* world = UWorld::GetWorld();
	if (world && world->OwningGameInstance)
	{
		auto GameInstance = world->OwningGameInstance;
		auto LocalPlayer = GameInstance->LocalPlayers[0];
		auto ViewportClient = LocalPlayer->ViewportClient;
		
		auto vTable = *(void***)(ViewportClient);
		origin_renderer = (PostRender_t)Memory::ChangePointer((uintptr_t)vTable, PostRenderIdx, (uintptr_t)MainRender);

		if (!origin_renderer)
		{
			Log::Error(xorstr_("OriginRenderer created failed!"));
			VMProtectEnd();
			return false;
		}
		
		Log::Success(xorstr_("OriginRenderer created successfully!"));
		
		VMProtectEnd();
		return true;
	}
	
	VMProtectEnd();
	return false;
}


void GameRenderHook::MainRender(UObject* object, UCanvas* Canvas)
{
	thread_local bool s_InMainRender = false;

	if (s_InMainRender)
	{
		if (origin_renderer) return origin_renderer(object, Canvas);
		return;
	}

	s_InMainRender = true;

	if (Canvas)
	{
		Archon::OnUpdate();
	}

	s_InMainRender = false;

	if (origin_renderer)
	{
		return origin_renderer(object, Canvas);
	}
}


void GameRenderHook::Shutdown()
{
	UWorld* world = UWorld::GetWorld();
	if (world && world->OwningGameInstance)
	{
		auto GameInstance = world->OwningGameInstance;
		auto LocalPlayer = GameInstance->LocalPlayers[0];
		auto ViewportClient = LocalPlayer->ViewportClient;
		
		auto vTable = *(void***)(ViewportClient);

		Memory::ChangePointer((uintptr_t)vTable, PostRenderIdx, (uintptr_t)origin_renderer);
	}
}


