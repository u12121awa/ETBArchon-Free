#include "pch.h"
#include "LongJump.h"

#include "FreeCam.h"
#include "../../Core/Managers/ModuleManager.h"
#include "../../Utils/InputUtils.h"

void LongJump::OnUpdate()
{
    auto freecam = ModuleManager::Get().GetModule<FreeCam>();
    if (freecam && freecam->IsEnabled()) return;
        
    auto Pawn = Instances::GetPlayerCharacter();
    if (!Pawn) return;
        
    auto MoveComp = Pawn->CharacterMovement;
    if (!MoveComp) return;
        
    if (InputUtils::IsKeyDown(VK_SPACE, true))
    {
        if (!jumping)
        {
            bool bCanActivate = false;

            if (airJump->value)
            {
                bCanActivate = true;
            }
            else
            {
                if (MoveComp->MovementMode == EMovementMode::MOVE_Walking || 
                    MoveComp->MovementMode == EMovementMode::MOVE_NavWalking)
                {
                    bCanActivate = true;
                }
            }
                
            if (bCanActivate)
            {
                jumping = true;
                timer.reset();
            }
        }
    }
        
    if (jumping)
    {
        if (timer.passedMs((int) motionTime->value)) 
        {
            jumping = false;
        }
        else
        {
            Pawn->SRV_Launch(jumpMotion->value);
        }
    }
}

