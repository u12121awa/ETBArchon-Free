#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"
#include "../../Utils/World/TimerHandler.h"

class AutoConnect : public Module
{
public:
    StringSetting* steamID = New<>(StringSetting(xorstr_("SteamID"), xorstr_("")));
    NumberSetting* delay = New<>(NumberSetting(xorstr_("ConnectDelay"), 1500, 0, 5000, xorstr_("%.0f")));
    BooleanSetting* smart = New<>(BooleanSetting(xorstr_("Smart"), true));

    TimerHandler timer;
    bool joining = false;
    
    AutoConnect() : Module(xorstr_("AutoConnect"))
    {
    }

    void OnEnable() override
    {
        joining = false;
    }

    void OnWorldChange() override
    {
        joining = false;
    }

    void OnUpdate() override
    {
        auto World = Instances::GetWorld();
        auto PC = Instances::GetPlayerController();

        if (!World || !PC) return;
        
        std::string currentUtf8 = std::string(xorstr_("open steam.") + steamID->text);
        std::wstring wideStr = Utils::Utf8ToWstring(currentUtf8);
        
        FString MessageToCommand(wideStr.c_str());

        if (!ShouldConnect())
        {
            timer.reset();
            return;
        }

        if (timer.passedMs((int) delay->value))
        {
            Instances::GetSystemLib()->ExecuteConsoleCommand(World, MessageToCommand, PC);
            Log::Info(xorstr_("Connected!"));
            joining = true;
            timer.reset();
        }
    }

    bool ShouldConnect()
    {
        auto World = Instances::GetWorld();
        if (smart->value && joining) return false;
        return World && (!World->NetDriver || !World->NetDriver->ServerConnection);
    }
};
