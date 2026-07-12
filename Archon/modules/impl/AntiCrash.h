#pragma once
#include "../Module.h"

class AntiCrash : public Module
{
public:
    BooleanSetting* nullFunction = New<>(BooleanSetting(xorstr_("NullFunction"), false));
    BooleanSetting* dismembered = New<>(BooleanSetting(xorstr_("Dismembered"), true));
    BooleanSetting* ragdoll = New<>(BooleanSetting(xorstr_("Ragdoll"), true));
    BooleanSetting* rowBoat = New<>(BooleanSetting(xorstr_("RowBoat"), true));
 
    AntiCrash() : Module(xorstr_("AntiCrash"))
    {
    }
    
    void OnFunctionEvent(FunctionEvent* event) override;

    
};
