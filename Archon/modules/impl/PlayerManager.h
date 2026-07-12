#pragma once
#include "../Module.h"

class PlayerManager : public Module
{
public:
    BooleanSetting* infiniteSanity = New<>(BooleanSetting(xorstr_("Infinite Sanity"), true));
    BooleanSetting* infiniteStamina = New<>(BooleanSetting(xorstr_("Infinite Stamina"), true));
    BooleanSetting* autoSprint = New<>(BooleanSetting(xorstr_("AutoSprint"), true));
    BooleanSetting* noFear = New<>(BooleanSetting(xorstr_("NoFear"), true));

    void OnUpdate() override;
    void OnDisable() override;
    void OnShutdownModule() override;
    void OnFunctionEvent(FunctionEvent* event) override;
    
    PlayerManager() : Module(xorstr_("PlayerManager"))
    {
    }
};
