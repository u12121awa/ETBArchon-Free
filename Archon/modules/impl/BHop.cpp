#include "pch.h"
#include "BHop.h"

#include "FreeCam.h"
#include "../../Utils/InputUtils.h"

void BHop::OnUpdate()
{
    auto freecam = ModuleManager::Get().GetModule<FreeCam>();
    if (freecam && freecam->IsEnabled()) return;
        
    auto PC = Instances::GetPlayerCharacter();
    if (!PC || !PC->Mesh) return;

    auto MoveComp = PC->CharacterMovement;
    if (!MoveComp) return;
        
    if (bIsAutoCrouching && crouch->value)
    {
        auto Now = std::chrono::steady_clock::now();
        double ElapsedMs = std::chrono::duration<double, std::milli>(Now - CrouchStartTime).count();
            
        if (ElapsedMs >= crouchTime->value) 
        {
            PC->UnCrouch(true);
            bIsAutoCrouching = false;
        }
    }
        
    if (MoveComp->MovementMode == EMovementMode::MOVE_Walking || 
        MoveComp->MovementMode == EMovementMode::MOVE_NavWalking)
    {
        CurrentCrouchCount = 0; 
            
        if (InputUtils::IsKeyDown(VK_SPACE, true))
        {
            PC->Jump();
            PC->bPressedJump = true;
        }
    }
    else if (MoveComp->MovementMode == EMovementMode::MOVE_Falling)
    {
        float VelocityZ = MoveComp->Velocity.Z;
            
        bool bCanCrouchAmount = (maxCrouches->value <= -1.0f) || (CurrentCrouchCount < (int)maxCrouches->value);
        bool bCanCrouchVel = (crouchVelocity->value <= -1.0f) || (VelocityZ <= crouchVelocity->value && VelocityZ > -1000.f);
        
        if (crouch->value && 
            !bIsAutoCrouching && 
            bCanCrouchAmount &&
            bCanCrouchVel) 
        {
            PC->Crouch(true);
                
            bIsAutoCrouching = true;
            CurrentCrouchCount++; 
            CrouchStartTime = std::chrono::steady_clock::now();
        }
            
        PC->bPressedJump = false;
    }
}
