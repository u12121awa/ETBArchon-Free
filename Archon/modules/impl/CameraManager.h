#pragma once

#include <vector>
#include <string>

#include "../Module.h"

class CameraManager : public Module
{
public:
    ModeSetting* viewMode = New<>(ModeSetting(xorstr_("ViewMode"), 0, std::vector<std::string> {xorstr_("First"),  xorstr_("Second"), xorstr_("Third") }));
    BindSetting* toggleBind = New<>(BindSetting(xorstr_("ToggleBind"), Bind(-1)));
    BooleanSetting* antiClip = New<>(BooleanSetting(xorstr_("AntiClip"), true)); 
    BooleanSetting* onlyFTToggle = New<>(BooleanSetting(xorstr_("NoSecondToggle"), true));
    
    BooleanGroupSetting* cameraFix = New<>(BooleanGroupSetting(xorstr_("CameraFix"), true));
    BooleanSetting* ignoreRoll = New<>(BooleanSetting(xorstr_("IgnoreRoll"), true), cameraFix);
    NumberSetting* eyeHeight = New<>(NumberSetting(xorstr_("EyeHeight"), 64.f, 0.f, 100.f, xorstr_("%.1f")), cameraFix);
    NumberSetting* crEyeHeight = New<>(NumberSetting(xorstr_("CrEyeHeight"), 50.f, 0.f, 100.f, xorstr_("%.1f")), cameraFix);

    NumberSetting* distance = New<>(NumberSetting(xorstr_("Distance"), 100.f, 50.f, 300.f, xorstr_("%.0f")));
    NumberSetting* offsetRight = New<>(NumberSetting(xorstr_("OffsetRight"), 0.f, -100.f, 100.f, xorstr_("%.0f")));
    NumberSetting* offsetUp = New<>(NumberSetting(xorstr_("OffsetUp"), 0.f, -100.f, 100.f, xorstr_("%.0f")));
    
    BooleanGroupSetting* armsFix = New<>(BooleanGroupSetting(xorstr_("ArmsFix"), true));
    NumberSetting* offsetX = New<>(NumberSetting(xorstr_("ArmsOffsetX"), 7.5f, -100.f, 100.f, xorstr_("%.1f")), armsFix);
    NumberSetting* offsetY = New<>(NumberSetting(xorstr_("ArmsOffsetY"), 2.0f, -100.f, 100.f, xorstr_("%.1f")), armsFix);
    NumberSetting* offsetZ = New<>(NumberSetting(xorstr_("ArmsOffsetZ"), -75.0f, -100.f, 100.f, xorstr_("%.1f")), armsFix);

    BooleanSetting* cameraInputFix = New<>(BooleanSetting(xorstr_("CameraInputFix"), true));
    NumberSetting* scaleYaw = New<>(NumberSetting(xorstr_("ScaleYaw"), 0.85f, 0.1f, 5.f, xorstr_("%.1f"), [this]{ return cameraInputFix->value; }));
    NumberSetting* scalePitch = New<>(NumberSetting(xorstr_("ScalePitch"), 0.85f, 0.1f, 5.f, xorstr_("%.1f"), [this]{ return cameraInputFix->value; }));
  
    BooleanSetting* aspectRatio = New<>(BooleanSetting(xorstr_("AspectRatio"), false));
    NumberSetting* ratio = New<>(NumberSetting(xorstr_("Ratio"), 1.3333f, 0.1f, 8.f, xorstr_("%.1f"), [this]{ return aspectRatio->value; }));
    
    
    CameraManager() : Module(xorstr_("CameraManager"))
    {
    }

    void OnEnable() override
    {
        SetVisibility(viewMode->GetCurrent() == xorstr_("First"));
        auto PC = Instances::GetPlayerController();
        if (PC && PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->ViewPitchMin = -89.9f;
            PC->PlayerCameraManager->ViewPitchMax = 89.9f;
        }
    }

    void OnDisable() override
    {
        SetVisibility(true);
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!PC || !Pawn) return;
        
        Pawn->Arms->RelativeLocation = {3.0f, 0.0f, -67.5f};
        Pawn->Arms->RelativeRotation = {0.0f, -90.000183f, 0.0f};
    }

    void OnInitializeModule() override
    {
        if (IsEnabled()) OnEnable();
    }

    void OnShutdownModule() override
    {
        OnDisable();
    }

    void OnRespawnChange() override
    {
        OnEnable();
    }

    float GetEyeHeight()
    {
        auto Character = Instances::GetPlayerCharacter(); 
        if (!Character) return 64.0f;
        
        float Eye = IsEnabled() && cameraFix->value ? eyeHeight->value : 64.0f;
        float CrouchEye = IsEnabled() && cameraFix->value ? crEyeHeight->value : 50.0f;
        
        auto MoveComp = Character->CharacterMovement;
        
        bool Crouched = Character->bIsCrouched == 1;
        
        if (!MoveComp) return Crouched ? CrouchEye : Eye;
        
        return Crouched && MoveComp->MovementMode != EMovementMode::MOVE_Falling ? CrouchEye : Eye;
    }

    void OnKeyEvent(KeyEvent* event) override
    {
        if (event->type == Pressed)
        {
            if (event->mouse == toggleBind->bind.mouse && event->key == toggleBind->bind.key)
            {
                std::string mode = viewMode->GetCurrent();

                if (mode == xorstr_("First"))
                {
                    viewMode->index = 2;
                } else if (mode == xorstr_("Second"))
                {
                    viewMode->index = 0;
                } else if (mode == xorstr_("Third"))
                {
                    if (onlyFTToggle->value) viewMode->index = 0;
                    else viewMode->index = 1;
                }
            }
        }
    }
    void OnUpdateViewPoint(FMinimalViewInfo* viewInfo);
    void OnFunctionEvent(FunctionEvent* event) override;
    
    
private:
    void SetVisibility(bool bEnable)
    {
        auto Character = Instances::GetPlayerCharacter(); 
        if (!Character || !Character->Mesh) return;
        
        if (Character->Mesh && Character->Mesh->bOwnerNoSee != (bEnable ? 1 : 0)) Character->Mesh->SetOwnerNoSee(bEnable);
            
        if (Character->Arms && Character->Arms->IsVisible() != bEnable) Character->Arms->SetVisibility(bEnable, false);

        Character->ShowBody = false;
    }
    
};