#include "pch.h"
#include "FreeCam.h"

#include "../../Utils/InputUtils.h"

void FreeCam::OnUpdate()
{
    auto PC = Instances::GetPlayerController();
      
    if (!PC) return;
    
    if (!bIsInitialized)
    {
        VirtualLocation = PC->K2_GetActorLocation();
        VirtualRotation = PC->ControlRotation;
        bIsInitialized = true;
    }

    FVector moveDirection = {0.f, 0.f, 0.f};
    FRotator realRotation = Archon::GetRotationManager()->GetRealRotation();
    FRotator horizontalRotation = FRotator(0.f, realRotation.Yaw, 0.f);

    FVector forward = Archon::GetRotationManager()->GetForwardVector(horizontalRotation);
    FVector right = Archon::GetRotationManager()->GetRightVector(horizontalRotation);

    if (InputUtils::IsKeyDown('W', true)) moveDirection += forward;
    if (InputUtils::IsKeyDown('S', true)) moveDirection -= forward;
    if (InputUtils::IsKeyDown('D', true)) moveDirection += right;
    if (InputUtils::IsKeyDown('A', true)) moveDirection -= right;
    if (InputUtils::IsKeyDown(VK_SPACE, true)) moveDirection.Z += 1.0f;
    if (InputUtils::IsKeyDown(VK_LSHIFT, true)) moveDirection.Z -= 1.0f;
        
    if (!moveDirection.IsZero())
    {
        moveDirection.Normalize();
            
        FVector delta = moveDirection;
        delta.X *= hSpeed->value;
        delta.Y *= hSpeed->value;
        delta.Z *= vSpeed->value;
            
        VirtualLocation += delta;
    }
}

void FreeCam::OnUpdateViewPoint(FMinimalViewInfo* viewInfo)
{
    
    if (IsEnabled())
    {
        auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();
        
        viewInfo->Location = VirtualLocation;
        viewInfo->Location.Z += cameraManager ? cameraManager->GetEyeHeight() : 64.0f;
    }
}

void FreeCam::OnKeyEvent(KeyEvent* event)
{
    std::vector<int> keys = {'W', 'A', 'S', 'D', VK_SPACE, VK_LSHIFT, VK_LCONTROL};

    if (!event->mouse && event->type == Pressed && std::find(keys.begin(), keys.end(), event->key) != keys.end())
    {
        event->cancel = true;
    }
}

