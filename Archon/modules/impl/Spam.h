#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"
#include "../../Utils/World/TimerHandler.h"

class Spam : public Module
{
public:
    StringSetting* spamString = New<>(StringSetting(xorstr_("Spam"), xorstr_("ez")));
    NumberSetting* delay = New<>(NumberSetting(xorstr_("SpamDelay"), 1500, 0, 5000, xorstr_("%.0f")));

    TimerHandler timer;

    Spam() : Module(xorstr_("Spam"))
    {
    }

    void OnUpdate() override
    {
        if (timer.passedMs((int)delay->value))
        {
            auto GameChatInstance = Instances::GetInstanceOf<UWB_Chat_C>();

            std::string currentUtf8 = spamString->text;

            if (!currentUtf8.empty())
            {
                if (GameChatInstance)
                {
                    std::wstring wideStr = Utils::Utf8ToWstring(currentUtf8);
                    
                    FString MessageToSend(wideStr.c_str());
                    
                    GameChatInstance->Send_Global_Message(MessageToSend);
                }
            }
            timer.reset();
        }
    }
};
