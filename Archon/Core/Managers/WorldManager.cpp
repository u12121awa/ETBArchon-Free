#include "pch.h"
#include "WorldManager.h"

#include "../../Utils/Utils.h"
#include "../Instance/Instances.hpp"

void GWorldManager::OnFunctionEvent(FunctionEvent* event)
{
 
}


void GWorldManager::OnUpdate()
{
    VMProtectBeginMutation("GWorldManager::OnUpdate");
    
    UWorld* World = Instances::GetWorld();
    AWorldSettings* Settings = World->K2_GetWorldSettings();
    if (!World || !Settings)
    {
        VMProtectEnd();
        return;
    }
    
    if (Settings->TimeDilation != TICK_TIMER)
    {
        Settings->TimeDilation = TICK_TIMER;
    }

    auto* Pawn = Instances::GetPlayerCharacter();
    auto* PC = Instances::GetPlayerController();
   
    
    if (Pawn)
    {
        if (Pawn->IsPushing)
        {
            Pawn->IsPushing = false;
            Pawn->StopPushingActor_SERVER(nullptr);
        }
        if (Pawn->IsClimbing || Pawn->IsClimbingLadder)
        {
            Pawn->IsClimbing = false;
            Pawn->IsClimbingLadder = false;
        }

        if (Pawn->IsHoldingRope)
        {
            Pawn->IsHoldingRope = false;
        }
    }
    

    if (PC)
    {
        auto playerState = PC->PlayerState;

        if (playerState)
        {
            Ping = playerState->Ping * 4;
            PlayerName = Utils::ReadWideStringFromAddress(playerState->PlayerNamePrivate, 128);
            VMProtectEnd();
            return;
        }
    }
    
    Ping = 0;
    if (PlayerName.empty()) PlayerName = xorstr_("User");
    VMProtectEnd();
}

void GWorldManager::SetSpeed(float NewSpeed)
{
    TICK_TIMER = NewSpeed;
    OnUpdate();
}

void GWorldManager::Reset()
{
    TICK_TIMER = 1.0f;

    UWorld* World = Instances::GetWorld(); 
    if (!World) return;
    
    AWorldSettings* Settings = World->K2_GetWorldSettings();
    if (!Settings) return;
    
    if (Settings->TimeDilation != TICK_TIMER)
    {
        Settings->TimeDilation = TICK_TIMER;
    }
}
