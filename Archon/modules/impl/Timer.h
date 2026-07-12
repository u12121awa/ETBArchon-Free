#pragma once

#include "../Module.h"
#include "../../Archon.h"
#include "../../Utils/InputUtils.h"

class Timer : public Module
{
public:
    Timer() : Module(xorstr_("Timer"))
    {
    }

    NumberSetting* timer = New<>(NumberSetting(xorstr_("Timer"), 1.0f, 0.1f, 20.0f, xorstr_("%.1f")));
    ModeSetting* mode = New<>(ModeSetting(xorstr_("Mode"), 0, std::vector<std::string>{ xorstr_("Normal"), xorstr_("Air"), xorstr_("NoAir") }));
    BooleanSetting* onlySpace = New<>(BooleanSetting(xorstr_("OnlySpace"), false));
    BooleanSetting* onlyNoSpace = New<>(BooleanSetting(xorstr_("OnlyNoSpace"), false, [this] { return !onlySpace->value; }));
   
    void OnInitializeModule() override
    {
        if (IsEnabled()) OnEnable();
    }

    bool ShouldUseTimer()
    {
        bool space = InputUtils::IsKeyDown(VK_SPACE, true);

        if (onlySpace->value) return space;
        if (onlyNoSpace->value) return !space;
        
        return true;
    }

    void OnUpdate() override
    {
        auto Pawn = Instances::GetPlayerCharacter();

        switch (mode->index)
        {
        case 0:
            {
                if (ShouldUseTimer())
                {
                    Archon::GetWorldManager()->SetSpeed(timer->value);
                }
                else
                {
                    Archon::GetWorldManager()->Reset();
                }
                break;
            }

        case 1:
            {
                if (!Pawn) break; 
                auto MoveComp = Pawn->CharacterMovement;
                if (!MoveComp) break;

                if (ShouldUseTimer() && MoveComp->MovementMode != EMovementMode::MOVE_Walking && MoveComp->MovementMode != EMovementMode::MOVE_NavWalking)
                {
                    Archon::GetWorldManager()->SetSpeed(timer->value);
                }
                else
                {
                    Archon::GetWorldManager()->Reset();
                }
                break;
            }
        case 2:
            {
                if (!Pawn) break; 
                auto MoveComp = Pawn->CharacterMovement;
                if (!MoveComp) break;

                if (ShouldUseTimer() && MoveComp->MovementMode == EMovementMode::MOVE_Walking || MoveComp->MovementMode == EMovementMode::MOVE_NavWalking)
                {
                    Archon::GetWorldManager()->SetSpeed(timer->value);
                }
                else
                {
                    Archon::GetWorldManager()->Reset();
                }
                break;
            }
        }
    }

    void OnDisable() override
    {
        if (auto WorldManager = Archon::GetWorldManager())
        {
            WorldManager->Reset();
        }
    }
};
