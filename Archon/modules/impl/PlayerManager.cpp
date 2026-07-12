#include "pch.h"
#include "PlayerManager.h"

bool isSprint = true;

void PlayerManager::OnUpdate()
{
    auto PlayerCharacter = Instances::GetPlayerCharacter();
    if (!PlayerCharacter) return;

    PlayerCharacter->ShouldUseStamina = !infiniteStamina->value;

    if (autoSprint->value)
    {
        isSprint = true;
        PlayerCharacter->StartSprint();
        PlayerCharacter->IsSprinting = true;
    } else
    {
        if (isSprint)
        {
            PlayerCharacter->StopSprint(true);
            PlayerCharacter->IsSprinting = false;
            isSprint = false;
        }
    }
    
    if (infiniteSanity->value) {
        AMP_PS_C* playerState = static_cast<AMP_PS_C*>(PlayerCharacter->PlayerState);
        if (playerState)
        {
            static ULONGLONG next_update_tick = 0;
            ULONGLONG current_tick = GetTickCount64();

            if (current_tick >= next_update_tick)
            {
                next_update_tick = current_tick + 1000;
                playerState->SRV_AddSanity(100.0f);
                playerState->AddSanity(100.0f);
            }
        }
    }
}

void PlayerManager::OnShutdownModule()
{
   if (IsEnabled()) OnDisable();
}

void PlayerManager::OnFunctionEvent(FunctionEvent* event)
{
    if (noFear->value && (
        event->functionName == xorstr_("Function BP_BalanceZone.BP_BalanceZone_C.ReceiveActorBeginOverlap") ||
        event->functionName == xorstr_("Function BPCharacter_Demo.BPCharacter_Demo_C.FearTimeline__UpdateFunc") ||
        event->functionName.starts_with(xorstr_("Function BPCharacter_Demo.BPCharacter_Demo_C.BalanceTimeline_")))
        )
    {
        event->cancel = true;
    }
}



void PlayerManager::OnDisable()
{
    auto PlayerCharacter = Instances::GetPlayerCharacter();
    if (!PlayerCharacter) return;

    PlayerCharacter->ShouldUseStamina = true;
    
    if (isSprint)
    {
        PlayerCharacter->StopSprint(true);
        PlayerCharacter->IsSprinting = false;
        isSprint = false;
    }
}

