#pragma once
#include "../Module.h"
#include "../../Archon.h"
#include "../../Utils/World/TimerHandler.h"

class Flight : public Module
{
public:
    NumberSetting* hSpeed = New<>(NumberSetting(xorstr_("Horizontal"), 50.0f, 1.0f, 100.0f, xorstr_("%.1f")));
    NumberSetting* vSpeed = New<>(NumberSetting(xorstr_("Vertical"), 40.0f, 1.0f, 100.0f, xorstr_("%.1f")));
    BooleanSetting* noCollision = New<>(BooleanSetting(xorstr_("NoCollision"), true));
    BooleanSetting* locationSync = New<>(BooleanSetting(xorstr_("LocationSync"), false));
    
    FVector VirtualLocation = FVector(0, 0, 0);
    bool bIsInitialized = false;
    TimerHandler timer;
    
    Flight() : Module(xorstr_("Flight"))
    {
    }
    
    void OnEnable() override
    {
        bIsInitialized = false;
    }

    void OnShutdownModule() override
    {
        if (IsEnabled()) OnDisable();
    }

    void OnRespawnChange() override
    {
        bIsInitialized = false;
    }
    
    void OnDisable() override
    {
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
      
        if (!PC || !Pawn) return;
        
        auto MovementComponent = Pawn->CharacterMovement;
        auto CapsuleComponent = Pawn->CapsuleComponent;
        
        if (!MovementComponent || !CapsuleComponent) return;
        if (noCollision->value)
            CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
    }
    
    
    void OnUpdate() override;

    FVector GetVirtualLocation()
    {
        if (!bIsInitialized) return FVector(0, 0, 0);
        
        return VirtualLocation;
    }
};
