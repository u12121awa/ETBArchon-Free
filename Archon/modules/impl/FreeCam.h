#pragma once
#include "../Module.h"
#include "../../Archon.h"

class FreeCam : public Module
{
public:
    NumberSetting* hSpeed = New<>(NumberSetting(xorstr_("Horizontal"), 50.0f, 1.0f, 100.0f, xorstr_("%.1f")));
    NumberSetting* vSpeed = New<>(NumberSetting(xorstr_("Vertical"), 40.0f, 1.0f, 100.0f, xorstr_("%.1f")));
    BooleanSetting* rotationLock = New<>(BooleanSetting(xorstr_("RotationLock"), true));
    BooleanSetting* locationSync = New<>(BooleanSetting(xorstr_("LocationSync"), true));
    BooleanSetting* reSpawnDisabled = New<>(BooleanSetting(xorstr_("ReSpawnDisabled"), true));
  
    FVector VirtualLocation = FVector(0, 0, 0);
    FRotator VirtualRotation = FRotator(0, 0, 0);
   
    bool bIsInitialized = false;
    
    FreeCam() : Module(xorstr_("FreeCam"))
    {
    }
    
    
    void OnEnable() override
    {
        bIsInitialized = false;
    }

    void OnShutdownModule() override
    {
        if (IsEnabled()) Disable();
    }

    void OnRespawnChange() override
    {
        bIsInitialized = false;
        if (reSpawnDisabled->value) Disable();
    }
    
    void OnUpdate() override;
    void OnUpdateViewPoint(FMinimalViewInfo* viewInfo);
    void OnKeyEvent(KeyEvent* event) override;

    FVector GetVirtualLocation()
    {
        if (!bIsInitialized) return FVector(0, 0, 0);
        
        return VirtualLocation;
    }
};
