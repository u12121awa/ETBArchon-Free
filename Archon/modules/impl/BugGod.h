#pragma once
#include "../Module.h"
#include "../../Utils/World/TimerHandler.h"

class BugGod : public Module
{
public:
    ModeSetting* mode = New<>(ModeSetting(xorstr_("Mode"), 0, std::vector<std::string> { xorstr_("Climbing") }));
    NumberSetting* climbDelay = New<>(NumberSetting(xorstr_("ClimbDelay"), 500.0f, 0.0f, 10000.0f, xorstr_("%.0f"), [this] { return mode->index == 0; }));

    TimerHandler timer2;

    BugGod() : Module(xorstr_("BugGod"))
    {
    }

    void OnUpdate() override
    {
        VMProtectBeginMutation("BugGod::OnUpdate");
        
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();

        if (!PC || !Pawn)
        {
            VMProtectEnd();
            return;
        }
        
        if (mode->index == 0)
        {
            if (timer2.passedMs((int) climbDelay->value))
            {
                Pawn->StartClimbingBot_SERVER();
       
                timer2.reset();
            }
        }
        VMProtectEnd();
    }
};
