#pragma once
#include <Windows.h>
#include "../Module.h"
#include "../../Utils/World/TimerHandler.h"

class LongJump : public Module
{
public:
    BooleanSetting* airJump = New<>(BooleanSetting(xorstr_("Air Jump"), false)); 
    NumberSetting* jumpMotion = New<>(NumberSetting(xorstr_("JumpMotion"), 2.0f, 0.1f, 50.0f));
    NumberSetting* motionTime = New<>(NumberSetting(xorstr_("MotionTime"), 500.0f, 1.0f, 2000.0f, xorstr_("%.0f")));

    LongJump() : Module(xorstr_("LongJump"))
    {
    }
    
    TimerHandler timer;
    bool jumping = false;

    void OnEnable() override
    {
        jumping = false;
    }

    void OnRespawnChange() override
    {
        jumping = false;
    }
    
    void OnUpdate() override;
};