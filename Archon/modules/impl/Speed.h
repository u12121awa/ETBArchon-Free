#pragma once
#include "../Module.h"
#include "../../Utils/InputUtils.h"

class Speed : public Module
{
public:
    ModeSetting* speedMode = New<>(ModeSetting(xorstr_("Mode"), 0, std::vector<std::string> {xorstr_("Normal"), xorstr_("Space")}));
    BooleanSetting* speedBoost = New<>(BooleanSetting(xorstr_("SpeedBoost"), false));
    
    NumberSetting* walkSpeed = New<>(NumberSetting(xorstr_("WalkSpeed"), 275.0f, -1.0f, 2000.f, xorstr_("%.1f")));
    NumberSetting* sprintSpeed = New<>(NumberSetting(xorstr_("SprintSpeed"), 550.0f, -1.0f, 2000.f, xorstr_("%.1f")));
    NumberSetting* crouchSpeed = New<>(NumberSetting(xorstr_("CrouchSpeed"), 600.0f, -1.0f, 2000.f, xorstr_("%.1f")));

    NumberSetting* maxWalkSpeed = New<>(NumberSetting(xorstr_("MaxWalkSpeed"), 275.0f, -1.0f, 2000.f, xorstr_("%.1f"), [this]{ return !speedBoost->value; }));
    NumberSetting* maxCustomSpeed = New<>(NumberSetting(xorstr_("MaxCustomSpeed"), 600.0f, -1.0f, 2000.f, xorstr_("%.1f")));
    NumberSetting* maxCrouchSpeed = New<>(NumberSetting(xorstr_("MaxCrouchSpeed"), 100.0f, -1.0f, 2000.f, xorstr_("%.1f")));

    NumberSetting* maxSwimSpeed = New<>(NumberSetting(xorstr_("MaxSwimSpeed"), 75.0f, -1.0f, 2000.f, xorstr_("%.1f")));

    NumberSetting* maxAcceleration = New<>(NumberSetting(xorstr_("MaxAcceleration"), 2048.0f, 1.0f, 4096.0f, xorstr_("%.0f")));

    
    bool bIsWalkSpeed = false;
    bool bIsSprintSpeed = false;
    bool bIsCrouchSpeed = false;
    
    bool bIsMaxWalkSpeed = false;
    bool bIsMaxCustomSpeed = false;
    bool bIsMaxCrouchSpeed = false;
    bool bIsMaxSwimSpeed = false;

    bool bIsMaxAcceleration = false;
    
    float NormalWalkSpeed = 300.0f;
    float NormalSprintSpeed = 600.0f;
    float NormalCrouchSpeed = 600.0f;

    float NormalMaxWalkSpeed = 275.0f;
    float NormalMaxCustomSpeed = 600.0f;
    float NormalMaxCrouchSpeed = 100.0f;
    float NormalMaxSwimSpeed = 75.0f;

  
    Speed() : Module(xorstr_("Speed"))
    {
    }

    void OnShutdownModule() override
    {
        OnDisable();
    }

    void OnDisable() override
    {
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!PC || !Pawn || !Pawn->Mesh) return;
        
        Pawn->CharacterMovement->MaxAcceleration = 2048.0f;
    }

    void OnUpdate() override
    {
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!PC || !Pawn || !Pawn->Mesh) return;
        
        
        if (speedBoost->value)
        {
            Pawn->SpeedBoost();
            Pawn->SpeedBoostClient();
        }

        auto bWalkSpeed = walkSpeed->value;
        auto bSprintSpeed = sprintSpeed->value;
        auto bCrouchSpeed = crouchSpeed->value;
        auto bMaxWalkSpeed = maxWalkSpeed->value;
        auto bMaxCustomSpeed = maxCustomSpeed->value;
        auto bMaxCrouchSpeed = maxCrouchSpeed->value;
        auto bMaxSwimSpeed = maxSwimSpeed->value;

        auto bMaxAcceleration = maxAcceleration->value;
     
        if (speedMode->GetCurrent() == xorstr_("Normal") || InputUtils::IsKeyDown(VK_SPACE, true))
        {
            if (bWalkSpeed != -1)
            {
                bIsWalkSpeed = true;
                Pawn->SetWalkSpeedServer(bWalkSpeed);
            }
            else
            {
                if (bIsWalkSpeed)
                {
                    Pawn->SetWalkSpeedServer(NormalWalkSpeed);
                    bIsSprintSpeed = false;
                }
            }

            if (bSprintSpeed != -1)
            {
                bIsSprintSpeed = true;
                Pawn->SetSprintSpeedServer(bSprintSpeed);
            }
            else
            {
                if (bIsSprintSpeed)
                {
                    Pawn->SetSprintSpeedServer(NormalSprintSpeed);
                    bIsSprintSpeed = false;
                }
            }
        
            if (bCrouchSpeed != -1)
            {
                bIsCrouchSpeed = true;
                Pawn->SetCrouchWalkSpeedServer(bCrouchSpeed);
            }
            else
            {
                if (bIsCrouchSpeed)
                {
                    Pawn->SetCrouchWalkSpeedServer(NormalCrouchSpeed);
                    bIsCrouchSpeed = false;
                }
            }

            if (speedBoost->value)
            {
                bIsMaxWalkSpeed = false;
            }
            else
            {
                if (bMaxWalkSpeed != -1)
                {
                    bIsMaxWalkSpeed = true;
                    Pawn->CharacterMovement->MaxWalkSpeed = bMaxWalkSpeed;
                }
                else
                {
                    if (bIsMaxWalkSpeed)
                    {
                        Pawn->CharacterMovement->MaxWalkSpeed = NormalMaxWalkSpeed;
                        bIsMaxWalkSpeed = false;
                    }
                }
            }

            if (bMaxCustomSpeed != -1)
            {
                bIsMaxCustomSpeed = true;
                Pawn->CharacterMovement->MaxCustomMovementSpeed = bMaxCustomSpeed;
            }
            else
            {
                if (bIsMaxCustomSpeed)
                {
                    Pawn->CharacterMovement->MaxCustomMovementSpeed = NormalMaxCustomSpeed;
                    bIsMaxCustomSpeed = false;
                }
            }

            if (bMaxCrouchSpeed != -1)
            {
                bIsMaxCrouchSpeed = true;
                Pawn->CharacterMovement->MaxWalkSpeedCrouched = bMaxCrouchSpeed;
            }
            else
            {
                if (bIsMaxCrouchSpeed)
                {
                    Pawn->CharacterMovement->MaxWalkSpeedCrouched = NormalMaxCrouchSpeed;
                    bIsMaxCrouchSpeed = false;
                }
            }

            if (bMaxSwimSpeed != -1)
            {
                bIsMaxSwimSpeed = true;
                Pawn->CharacterMovement->MaxSwimSpeed = bMaxSwimSpeed;
            }
            else
            {
                if (bIsMaxSwimSpeed)
                {
                    Pawn->CharacterMovement->MaxSwimSpeed = NormalMaxSwimSpeed;
                    bIsMaxSwimSpeed = false;
                }
            }

            if (bMaxAcceleration != -1)
            {
                bIsMaxAcceleration = true;
                Pawn->CharacterMovement->MaxAcceleration = bMaxAcceleration;
            }
            else
            {
                if (bIsMaxAcceleration)
                {
                    Pawn->CharacterMovement->MaxAcceleration = 2048;
                    bIsMaxAcceleration = false;
                }
            }
        } else if (speedMode->GetCurrent() == xorstr_("Space"))
        {
            if (bIsWalkSpeed)
            {
                Pawn->SetWalkSpeedServer(NormalWalkSpeed);
                bIsWalkSpeed = false;
            }
            
            if (bIsSprintSpeed)
            {
                Pawn->SetSprintSpeedServer(NormalSprintSpeed);
                bIsSprintSpeed = false;
            }

            if (bIsCrouchSpeed)
            {
                Pawn->SetCrouchWalkSpeedServer(NormalCrouchSpeed);
                bIsCrouchSpeed = false;
            }

            if (bIsMaxWalkSpeed)
            {
                Pawn->CharacterMovement->MaxWalkSpeed = NormalMaxWalkSpeed;
                bIsMaxWalkSpeed = false;
            }

            if (bIsMaxCustomSpeed)
            {
                Pawn->CharacterMovement->MaxCustomMovementSpeed = NormalMaxCustomSpeed;
                bIsMaxCustomSpeed = false;
            }

            if (bIsMaxCrouchSpeed)
            {
                Pawn->CharacterMovement->MaxWalkSpeedCrouched = NormalMaxCrouchSpeed;
                bIsMaxCrouchSpeed = false;
            }

            if (bIsMaxSwimSpeed)
            {
                Pawn->CharacterMovement->MaxSwimSpeed = NormalMaxSwimSpeed;
                bIsMaxSwimSpeed = false;
            }
        }
    }
    
};
