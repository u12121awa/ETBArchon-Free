#pragma once
#include "../Module.h"
#include <chrono>

class BHop : public Module
{
public:
    BooleanSetting* crouch = New<>(BooleanSetting(xorstr_("Auto Crouch"), true));
    NumberSetting* maxCrouches = New<>(NumberSetting(xorstr_("Max Crouches"), 1.0f, -1.0f, 10.0f, xorstr_("%.0f"), [this]{ return crouch->value; }));
    NumberSetting* crouchVelocity = New<>(NumberSetting(xorstr_("Trigger Velocity"), 10.0f, -1.0f, 50.0f, xorstr_("%.1f"), [this]{ return crouch->value; }));
    NumberSetting* crouchTime = New<>(NumberSetting(xorstr_("Duration (ms)"), 200.0f, 50.0f, 500.0f, xorstr_("%.0f"), [this]{ return crouch->value; }));
    
    bool bIsAutoCrouching = false;
    int CurrentCrouchCount = 0; 
    std::chrono::steady_clock::time_point CrouchStartTime;
    
    BHop() : Module(xorstr_("BHop"))
    {
    }

    void OnUpdate() override;
};
