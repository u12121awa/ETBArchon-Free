#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../Module.h"
#include "../../Archon.h"
#include "../../Core/Config.h"

class UnLoad : public Module
{
public:
    UnLoad() : Module(xorstr_("UnLoad"))
    {
        
    }

    void OnEnable() override
    {
        this->Disable();
        Archon::GetArchonGUI()->SetOpen(false);
        Config::Settings::Unload = true;
    }
};
