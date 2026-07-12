#pragma once
#include "../Module.h"

class SoundFX : public Module
{
public:
    SoundFX() : Module(xorstr_("SoundFX"))
    {
    }

    NumberSetting* volume = New<>(NumberSetting(xorstr_("Volume"), 0.5f, 0.1f, 1.0f));
    ModeSetting* enabledMode = New<>(ModeSetting(xorstr_("EnabledMode"), 1, std::vector<std::string> { xorstr_("None"), xorstr_("Inertia"), xorstr_("USB")}));
    ModeSetting* disabledMode = New<>(ModeSetting(xorstr_("DisabledMode"), 1, std::vector<std::string> { xorstr_("None"), xorstr_("Inertia"), xorstr_("USB")}));
};
