#include "pch.h"
#include "RotationUtil.h"

#include "../../Archon.h"

FVector RotationUtil::GetEyePos()
{
    FVector PlayerLocation = Instances::GetPlayerLocation();
    PlayerLocation.Z += GetEyeHeight();
    return PlayerLocation;
}

FVector RotationUtil::GetCameraPos()
{
    auto PC = Instances::GetPlayerController();
    if (!PC) return {0, 0, 0};
    return PC->PlayerCameraManager->GetCameraLocation();
}

float RotationUtil::GetEyeHeight()
{
    auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();
    
    return cameraManager ? cameraManager->GetEyeHeight() : 64.0f;
}


RotationManager* RotationUtil::GetRotationManager()
{
    return Archon::GetRotationManager();
}

void RotationUtil::SetRotation(FRotator NewTarget, float TimeMs, int Priority)
{
    Archon::GetRotationManager()->SetRotation(NewTarget, TimeMs, Priority);
}

void RotationUtil::StopRotation()
{
    Archon::GetRotationManager()->StopRotation();
}

