#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "../Module.h"
#include "../../Core/Managers/ModuleManager.h"
#include "../../Core/Managers/RotationManager.h" 

class AntiAim : public Module
{
public:
    float currentSpinYaw = 0.0f;
    float currentSpinPitch = 0.0f;
    float sinusStepYaw = 0.0f;
    float sinusStepPitch = 0.0f;
    
    ModeSetting* yawMode = New<>(ModeSetting(xorstr_("YawMode"), 0, std::vector<std::string> {xorstr_("None"), xorstr_("Random"), xorstr_("Spin"), xorstr_("Reverse"), xorstr_("Sinus"), xorstr_("Fixed"), xorstr_("Static")}));
    ModeSetting* pitchMode = New<>(ModeSetting(xorstr_("PitchMode"), 0, std::vector<std::string> {xorstr_("None"), xorstr_("Random"), xorstr_("Spin"), xorstr_("Reverse"), xorstr_("Sinus"), xorstr_("Fixed"), xorstr_("Static")}));
    
    NumberSetting* speed = New<>(NumberSetting(xorstr_("Speed"), 1.f, 0.1f, 45.f, xorstr_("%.1f")));
    NumberSetting* yawDelta = New<>(NumberSetting(xorstr_("YawDelta"), 0.f, -180.f, 180.f, xorstr_("%.0f")));
    NumberSetting* pitchDelta = New<>(NumberSetting(xorstr_("PitchDelta"), 0.f, -89.f, 89.f, xorstr_("%.0f")));
    
    NumberSetting* time = New<>(NumberSetting(xorstr_("Time"), 50.f, 0.f, 500.f, xorstr_("%.0f")));
    
    AntiAim() : Module(xorstr_("AntiAim"))
    {
    }

    void OnEnable() override
    {
        FRotator Real = Archon::GetRotationManager()->GetRealRotation();
        currentSpinYaw = Real.Yaw;
        currentSpinPitch = Real.Pitch;
        sinusStepYaw = 0.0f;
        sinusStepPitch = 0.0f;
        
    }

    void OnInitializeModule() override
    {
        OnEnable();
    }

    void OnDisable() override
    {
        Archon::GetRotationManager()->StopRotation();
    }
    
    void OnUpdate() override
    {
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!PC || !Pawn || !Pawn->Mesh) return;
        
        FRotator Real = Archon::GetRotationManager()->GetRealRotation();
        
        float finalYaw = Real.Yaw;
        float finalPitch = Real.Pitch;
        
        std::string pMode = pitchMode->GetCurrent();
        if (pMode != xorstr_("None"))
        {
            if (pMode == xorstr_("Random"))
            {
                finalPitch = -89.f + static_cast<float>(rand()) / (RAND_MAX / (89.f - (-89.f)));
            }
            else if (pMode == xorstr_("Spin"))
            {
                currentSpinPitch += pitchDelta->value; 
                if (currentSpinPitch > 89.f) currentSpinPitch = -89.f;
                if (currentSpinPitch < -89.f) currentSpinPitch = 89.f;
                finalPitch = currentSpinPitch;
            }
            else if (pMode == xorstr_("Reverse"))
            {
                finalPitch = -Real.Pitch;
            }
            else if (pMode == xorstr_("Sinus"))
            {
                sinusStepPitch += speed->value / 10.0f;
                finalPitch = std::clamp(Real.Pitch + pitchDelta->value * sinf(sinusStepPitch), -89.f, 89.f);
            }
            else if (pMode == xorstr_("Fixed"))
            {
                finalPitch = pitchDelta->value; 
            }
            else if (pMode == xorstr_("Static"))
            {
                finalPitch = std::clamp(Real.Pitch + pitchDelta->value, -89.f, 89.f);
            }
        }
        
        std::string yMode = yawMode->GetCurrent();
        if (yMode != xorstr_("None"))
        {
            if (yMode == xorstr_("Random"))
            {
                finalYaw = -180.f + static_cast<float>(rand()) / (RAND_MAX / (180.f - (-180.f)));
            }
            else if (yMode == xorstr_("Spin"))
            {
                currentSpinYaw += yawDelta->value; 
                finalYaw = NormalizeAngle(currentSpinYaw);
                currentSpinYaw = finalYaw; 
            }
            else if (yMode == xorstr_("Reverse"))
            {
                finalYaw = NormalizeAngle(Real.Yaw + 180.0f);
            }
            else if (yMode == xorstr_("Sinus"))
            {
                sinusStepYaw += speed->value / 10.0f;
                finalYaw = NormalizeAngle(Real.Yaw + yawDelta->value * sinf(sinusStepYaw));
            }
            else if (yMode == xorstr_("Fixed"))
            {
                finalYaw = NormalizeAngle(yawDelta->value);
            }
            else if (yMode == xorstr_("Static"))
            {
                finalYaw = NormalizeAngle(Real.Yaw + yawDelta->value);
            }
        }
        
        if (pMode != xorstr_("None") || yMode != xorstr_("None"))
        {
            finalPitch = std::clamp(finalPitch, -89.f, 89.f);
            
            Archon::GetRotationManager()->SetRotation({finalPitch, finalYaw, Real.Roll}, time->value, 1);
        }
    }

private:
    float NormalizeAngle(float angle)
    {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }
};