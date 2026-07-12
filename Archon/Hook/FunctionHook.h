#pragma once
#include "../Utils/World/WorldUtils.h"

enum EStereoscopicPass
{
    eSSP_FULL,
    eSSP_LEFT_EYE,
    eSSP_RIGHT_EYE,
    eSSP_LEFT_EYE_SIDE,
    eSSP_RIGHT_EYE_SIDE,
};
typedef void (*ProcessEvent_t)(UObject*, UFunction*, void*);
typedef void (*OGetViewPoint)(ULocalPlayer*, FMinimalViewInfo*, EStereoscopicPass);
typedef void (*GetPlayerViewPoint)(APlayerController*, FVector*, FRotator*);
typedef bool (__fastcall* GetProjectionData_t)(__int64 a1, __int64 a2, int a3, __int64 a4);

namespace FunctionHook
{
    bool Initialize();
    void hkProcessEvent(UObject* pThis, UFunction* Function, void* Parms);
    void hkGetPlayerViewPoint(APlayerController* this_PC, FVector* Location, FRotator* Rotation);
    void hkGetViewPoint(ULocalPlayer* thisLocalPlayer, FMinimalViewInfo* OutViewInfo, EStereoscopicPass StereoPass);
    bool __fastcall hkGetProjectionData(__int64 a1, __int64 a2, int a3, __int64 a4);
};

