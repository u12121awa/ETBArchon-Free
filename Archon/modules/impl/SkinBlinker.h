#pragma once
#include "../Module.h"
#include "../../Utils/World/TimerHandler.h"
#include <vector>
#include <cstdlib>
#include <ctime>

class SkinBlinker : public Module
{
public:
    
    NumberSetting* delay = New<>(NumberSetting(xorstr_("Delay (ms)"), 1000, 1, 5000, xorstr_("%.0f")));
    ModeSetting* mode = New<>(ModeSetting(xorstr_("Mode"), 0, std::vector<std::string>{ xorstr_("Normal"), xorstr_("PingPong"), xorstr_("Random") }));

    std::vector<UCostume*> Costumes;
    TimerHandler timer;
    int currentIndex = 0;
    
    bool isMovingForward = true;

    SkinBlinker() : Module(xorstr_("SkinBlinker"))
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void OnUpdate() override
    {
        auto PC = Instances::GetPlayerCharacter(); 
        if (!PC) return;

        if (Costumes.empty())
        {
            Costumes = Instances::GetAllInstancesOf<UCostume>();
        }
        if (Costumes.empty()) return;
        
        if (timer.passedMs((long long)delay->value))
        {
            UpdateIndex();
            
            if (currentIndex >= 0 && currentIndex < Costumes.size())
            {
                UCostume* selectedSkin = Costumes[currentIndex];
                if (selectedSkin) {
                    ApplySkin(PC, selectedSkin);
                }
            }
            
            timer.reset();
        }
    }
    
    void UpdateIndex()
    {
        int size = (int)Costumes.size();
        if (size <= 1) return; 
        
        switch (mode->index)
        {
        case 0:
            currentIndex++;
            if (currentIndex >= size) {
                currentIndex = 0;
            }
            break;

        case 1: 
            if (isMovingForward) {
                currentIndex++;
                if (currentIndex >= size) {
                    currentIndex = size - 2; 
                    isMovingForward = false;
                }
            } else {
                currentIndex--;
                if (currentIndex < 0) {
                    currentIndex = 1; 
                    isMovingForward = true;
                }
            }
            if (currentIndex < 0) currentIndex = 0;
            if (currentIndex >= size) currentIndex = size - 1;
            break;

        case 2:
            currentIndex = std::rand() % size;
            break;
        }
    }

    void ApplySkin(ABPCharacter_Demo_C* PC, UCostume* Skin)
    {
        if (!PC || !Skin) return;
        
        UFancyPlayerCostumeComponent* cc = static_cast<UFancyPlayerCostumeComponent*>(PC->GetComponentByClass(UFancyPlayerCostumeComponent::StaticClass()));
        if (!cc) return;
        
        cc->AssignCostumeRPC(Skin);
    }

    void OnRespawnChange() override
    {
        Costumes = Instances::GetAllInstancesOf<UCostume>();
        currentIndex = 0;
        isMovingForward = true;
    }
};