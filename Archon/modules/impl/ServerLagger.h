#pragma once
#include "../Module.h"
#include "../../Core/Managers/DynamicIslandManager.h"

class ServerLagger : public Module
{
public:
    ModeSetting* mode = New<>(ModeSetting(xorstr_("CrashMode"), 0, std::vector<std::string>{xorstr_("Ragdoll"), xorstr_("Dismembered")}));
    BooleanSetting* autoDisabled = New<>(BooleanSetting(xorstr_("AutoDisabled"), true));
    NumberSetting* delay = New<>(NumberSetting(xorstr_("SpawnDelay"), 500, 0, 5000, xorstr_("%.0f"), [this] { return !autoDisabled->value; }));

    TimerHandler timer;
    bool tip = false;

    ServerLagger() : Module(xorstr_("ServerLagger"))
    {
    }

    void OnEnable() override
    {
        tip = false;
    }

    void OnUpdate() override
    {
        if (Instances::GetLevelData()->HostSteamID.empty())
        {
            if (!tip) {
                NotificationManager::AddNotification(NotificationType::ERR, xorstr_("ServerLagger"), xorstr_("IsHost!"), 3000);
                tip = true;
            }
            timer.reset();
            return;
        }

        auto PC = Instances::GetPlayerCharacter();

        if (!PC)
        {
            if (!tip)
            {
                NotificationManager::AddNotification(NotificationType::ERR, xorstr_("ServerLagger"), xorstr_("Demo NotFound!"), 3000);
                tip = true;
            }
            timer.reset();
            return;
        }
        VMProtectBeginMutation("ServerLagger::OnUpdate");
        if (timer.passedMs((int)delay->value) || autoDisabled->value)
        {
            switch (mode->index)
            {
            case 0:
                {
                    UClass* ragdoll = ABP_Ragdoll_C::StaticClass();
                    if (!ragdoll) break;
                    PC->SpawnEquipItem_SERVER(ragdoll);
                    break;
                }
            case 1:
                {
                    UClass* dismembered = ABP_Dismembered_C::StaticClass();

                    if (!dismembered) break;
                    PC->SpawnEquipItem_SERVER(dismembered);
                    break;
                }
            }
            
            timer.reset();
            if (autoDisabled->value)
            {
                this->Disable();
            }
        }
        VMProtectEnd();
    }
};
