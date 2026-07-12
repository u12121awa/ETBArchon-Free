#include "pch.h"
#include "FunctionHook.h"

#include <algorithm>
#include <psapi.h>
#include "../Archon.h"
#include "../Core/Memory.h"
#include "../Core/Instance/Instances.hpp"
#include "../Core/Module/features.h"
#include "../modules/impl/AntiAim.h"
#include "../Utils/World/FTickCallbackSystem.h"
#include "../Utils/Minhook/MinHook.h"
#include "../Utils/World/EventInterval.h"
#include "../Utils/World/Events.h"
#include "DX12Hook.h"
#include "../Core/Managers/ModuleManager.h"
#include "../Core/Managers/RotationManager.h"

static ProcessEvent_t oProcessEvent = nullptr;
static OGetViewPoint oGetViewPoint = nullptr;
static GetPlayerViewPoint oGetPlayerViewPoint = nullptr;
static GetProjectionData_t oGetProjectionData = nullptr;

bool FunctionHook::Initialize()
{
    VMProtectBeginUltra("Initializing FunctionHook");

    UWorld* world = UWorld::GetWorld();
    if (world && world->OwningGameInstance)
    {
        uintptr_t** VTable = *(uintptr_t***)world->OwningGameInstance;

        void* pProcessEventAddr = VTable[Offsets::ProcessEventIdx];

        if (MH_CreateHook(pProcessEventAddr, &hkProcessEvent, reinterpret_cast<LPVOID*>(&oProcessEvent)) != MH_OK)
        {
            Log::Error(xorstr_("CreateHook (ProcessEvent) failed!"));
            VMProtectEnd();
            return false;
        }

        uintptr_t GetViewPointAddress = Memory::PatternScan((uintptr_t)GetModuleHandleA(NULL),
                                                            xorstr_(
                                                                "48 8B C4 48 89 58 ? 55 56 57 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 ? 0F 29 78 ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 48 8B FA"));
        if (!GetViewPointAddress)
        {
            Log::Error(xorstr_("Failed to find GetViewPoint pattern!"));
        }
        if (MH_CreateHook(reinterpret_cast<LPVOID>(GetViewPointAddress), hkGetViewPoint,
                          reinterpret_cast<LPVOID*>(&oGetViewPoint)) != MH_STATUS::MH_OK)
        {
            Log::Error(xorstr_("Failed to create GetViewPoint hook!"));
        }

        if (MH_EnableHook(reinterpret_cast<LPVOID>(GetViewPointAddress)) != MH_OK)
        {
            Log::Error(xorstr_("Failed to enable GetViewPoint hook!"));
        }

        uintptr_t GetProjectionDataAddress = Memory::PatternScan((uintptr_t)GetModuleHandleA(NULL),
                                                                 xorstr_("40 55 56 57 41 55 41 56 41 57 48 8D AC 24"));
        if (!GetProjectionDataAddress)
        {
            Log::Error(xorstr_("Failed to find GetProjectionData pattern!"));
        }
        if (MH_CreateHook(reinterpret_cast<LPVOID>(GetProjectionDataAddress), hkGetProjectionData,
                          reinterpret_cast<LPVOID*>(&oGetProjectionData)) != MH_STATUS::MH_OK)
        {
            Log::Error(xorstr_("Failed to createGetProjectionData hook!"));
        }

        if (MH_EnableHook(reinterpret_cast<LPVOID>(GetProjectionDataAddress)) != MH_OK)
        {
            Log::Error(xorstr_("Failed to enable GetProjectionData hook!"));
        }

        /*
        uintptr_t GetPlayerViewPointAddress = Memory::PatternScan((uintptr_t)GetModuleHandleA(NULL), "48 89 5C 24 ? 48 89 74 24 ? 55 41 56 41 57 48 8B EC 48 83 EC ? 48 8B F2");
        if (!GetPlayerViewPointAddress) {
            Log::Error("Failed to find GetPlayerViewPoint pattern!");
        }
        if (MH_CreateHook(reinterpret_cast<LPVOID>(GetPlayerViewPointAddress), hkGetPlayerViewPoint, reinterpret_cast<LPVOID*>(&oGetPlayerViewPoint)) != MH_STATUS::MH_OK)
        {
            Log::Error("Failed to create GetPlayerViewPoint hook!");
        }

        if (MH_EnableHook(reinterpret_cast<LPVOID>(GetPlayerViewPointAddress)) != MH_STATUS::MH_OK)
        {
            Log::Error("Failed to enable GetPlayerViewPoint hook!");
        }
        */
        
        Log::Success(xorstr_("ProcessEvent hook created successfully!"));
        VMProtectEnd();
        return true;
    }

    Log::Error(xorstr_("Failed to get UGameInstance to hook ProcessEvent."));
    return false;
}



void FunctionHook::hkGetPlayerViewPoint(APlayerController* this_Player, FVector* Location, FRotator* Rotation)
{
    if (oGetPlayerViewPoint)
        oGetPlayerViewPoint(this_Player, Location, Rotation);

    // if (Archon::GetRotationManager()) Archon::GetRotationManager()->OnUpdatePlayerViewPoint(this_Player, Location, Rotation);
}

void FunctionHook::hkGetViewPoint(ULocalPlayer* this_Player, FMinimalViewInfo* OutViewInfo,
                                  EStereoscopicPass StereoPass)
{
    Instances::Update(false);
    if (oGetViewPoint) oGetViewPoint(this_Player, OutViewInfo, StereoPass);
    if (Archon::GetRotationManager()) Archon::GetRotationManager()->OnUpdateViewPoint(this_Player, OutViewInfo);
}

bool __fastcall FunctionHook::hkGetProjectionData(__int64 a1, __int64 a2, int a3, __int64 a4)
{
    auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();

    if (!cameraManager || !cameraManager->IsEnabled() || !cameraManager->aspectRatio->value)
        return oGetProjectionData(a1, a2, a3, a4);

    bool Result = oGetProjectionData(a1, a2, a3, a4);

    if (Result && a4 != 0)
    {
        float* Matrix = (float*)(a4 + 0x50);

        float CurrentYScale = Matrix[5];

        float TargetRatio = cameraManager->ratio->value;

        if (CurrentYScale != 0.0f)
        {
            Matrix[0] = CurrentYScale / TargetRatio;
        }
    }

    return Result;
}


void FunctionHook::hkProcessEvent(UObject* Context, UFunction* Function, void* Parms)
{
    bool cancel = true;

    std::string functionName = xorstr_("Unknown");

    if (Context && Function)
    {
        functionName = Function->GetFullName();

        FunctionEvent event = {Context, Function, Parms, functionName, oProcessEvent, false};

        Archon::OnFunctionEvent(&event);

        cancel = event.cancel;
    }

    if (!cancel)
    {
        if (oProcessEvent)
        {
            oProcessEvent(Context, Function, Parms);
        }
    }
}
