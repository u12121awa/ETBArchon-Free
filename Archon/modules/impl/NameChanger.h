#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cwctype>
#include <random>

#include "../Module.h"
#include "../../Archon.h"
#include "../../Utils/Utils.h"
#include "../../Utils/World/TimerHandler.h"

class NameChanger : public Module
{
public:
    ModeSetting* mode = New<>(ModeSetting(xorstr_("Mode"), 0, std::vector<std::string>{xorstr_("Normal"), xorstr_("Anim"), xorstr_("Random"), xorstr_("ListRandom")}));
    BooleanSetting* keepSource = New<>(BooleanSetting(xorstr_("KeepSource"), true, [this] { return mode->index < 2; }));
    StringSetting* name = New<>(StringSetting(xorstr_("Name"), xorstr_("NameChanger"), [this] { return mode->index < 2 && !keepSource->value; }));
    NumberSetting* animUpdateTime = New<>(NumberSetting(xorstr_("AnimTime"), 300.0f, 0.0f, 1000.0f, xorstr_("%.0f"), [this] { return mode->index == 1; }));
    NumberSetting* length = New<>(NumberSetting(xorstr_("Length"), 8.0f, 1.0f, 20.0f, xorstr_("%.0f"), [this] { return mode->index == 2; }));
    BooleanSetting* numbers = New<>(BooleanSetting(xorstr_("Numbers"), true, [this] { return mode->index == 2; }));
    BooleanSetting* upperCase = New<>(BooleanSetting(xorstr_("UpperCase"), false, [this] { return mode->index == 2; }));
    BooleanSetting* lowerCase = New<>(BooleanSetting(xorstr_("LowerCase"), false, [this] { return mode->index == 2 && !upperCase->value; }));
    NumberSetting* delay = New<>(NumberSetting(xorstr_("Delay"), 1000.0f, 0.0f, 5000.0f, xorstr_("%.0f"), [this] { return mode->index != 1; }));
    BooleanSetting* autoReset = New<>(BooleanSetting(xorstr_("AutoReset"), true));

    std::string SourceName;
    TimerHandler timer;

    NameChanger() : Module(xorstr_("NameChanger"))
    {
        SourceName = "";
    }

    void OnDisable() override
    {
        if (autoReset->value) OnResetName();
    }

    void OnShutdownModule() override
    {
        if (IsEnabled()) OnResetName();
    }

    void OnResetName()
    {
        FTickCallbackSystem::RegisterCallback([this]
        {
            auto* PC = Instances::GetPlayerController();
            if (PC && !SourceName.empty())
            {
                std::wstring wideSource = Utils::Utf8ToWstring(SourceName);
                FString finalName(wideSource.c_str());
                PC->ServerChangeName(finalName);
            }
        });
    }

    void OnUpdate() override
    {
        auto* PC = Instances::GetPlayerController();

        if (SourceName.empty())
        {
            SourceName = Archon::GetWorldManager()->GetPlayerName();
            return;
        }

        if (!PC) return;
        
        std::wstring TargetWideName = GetTargetNameW(mode->index, keepSource->value ? SourceName : name->text);

        if (mode->index == 1 || timer.passedMs((int)delay->value))
        {
            if (!TargetWideName.empty())
            {
                FString MessageToName(TargetWideName.c_str());
                PC->ServerChangeName(MessageToName);
            }
            else
            {
                FString Empty = FString(L" ");
                PC->ServerChangeName(Empty);
            }
            
            timer.reset();
        }
    }
    
    std::wstring GetTargetNameW(int mode, std::string inputUtf8Name)
    {
        std::string finalUtf8String = "";

        switch (mode)
        {
        case 0: // Normal
            finalUtf8String = inputUtf8Name;
            break;
            
        case 1: // Anim
            finalUtf8String = GetAnimatedTitleUtf8(inputUtf8Name, (int)animUpdateTime->value);
            break;

        case 2: // Random
            finalUtf8String = GenerateRandomAlphabetic((int)length->value, numbers->value);
            if (upperCase->value) {
                std::transform(finalUtf8String.begin(), finalUtf8String.end(), finalUtf8String.begin(), ::toupper);
            } else if (lowerCase->value) {
                std::transform(finalUtf8String.begin(), finalUtf8String.end(), finalUtf8String.begin(), ::tolower);
            }
            break;
            
        default: 
            return L"";
        }

        if (!finalUtf8String.empty())
        {
            return Utils::Utf8ToWstring(finalUtf8String);
        }

        return L"";
    }
    
    size_t GetUtf8Length(const std::string& str) {
        size_t len = 0;
        for (char c : str) {
            if ((c & 0xC0) != 0x80) len++;
        }
        return len;
    }

    std::string Utf8Substr(const std::string& str, size_t count) {
        size_t bytes = 0;
        size_t chars = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            if (chars >= count) break;
            bytes++; 
            if (i + 1 < str.size() && (str[i + 1] & 0xC0) != 0x80) {
                chars++;
            }
        }
        if (chars < count) bytes = str.size();
        return str.substr(0, bytes);
    }

    std::string GetAnimatedTitleUtf8(std::string name, int updateTimeMs)
    {
        static std::string lastTitle = "";
        static int charLength = 0;
        static int breakTimer = 0;
        static bool back = false;
        static auto lastTime = std::chrono::steady_clock::now();

        if (name != lastTitle)
        {
            lastTitle = name;
            charLength = 0;
            breakTimer = 0;
            back = false;
            lastTime = std::chrono::steady_clock::now();
            return name;
        }

        int fullCharCount = (int)GetUtf8Length(name);
        
        int maxHidden = fullCharCount - 1;
        if (maxHidden < 0) maxHidden = 0;

        auto currentTime = std::chrono::steady_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();

        if (elapsed < updateTimeMs)
        {
            int currentCount = fullCharCount - charLength;
            if (currentCount < 1) currentCount = 1;
            if (currentCount > fullCharCount) currentCount = fullCharCount;
            return Utf8Substr(name, currentCount);
        }

        lastTime = currentTime;
        
        if (charLength == maxHidden) 
        {
            if (breakTimer < 1) 
            { 
                breakTimer++; 
                return Utf8Substr(name, 1); 
            }
        }
        
        if (charLength == 0) 
        {
            if (breakTimer < 1) { breakTimer++; return name; }
        }

        breakTimer = 0;
        
        if (charLength == maxHidden) back = true;

        if (back) charLength--;
        else charLength++;

        if (charLength <= 0) { charLength = 0; back = false; }
        
        if (charLength >= maxHidden) charLength = maxHidden;
        
        return Utf8Substr(name, fullCharCount - charLength);
    }

    std::string GenerateRandomAlphabetic(int len, bool includeNumbers)
    {
        static const std::string charset = xorstr_("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        int maxIndex = includeNumbers ? (int)charset.size() - 1 : 51;
        
        std::uniform_int_distribution<> dis(0, maxIndex);

        std::string result;
        result.resize(len);

        for (int i = 0; i < len; ++i) 
        {
            result[i] = charset[dis(gen)];
        }
    
        return result;
    }
};