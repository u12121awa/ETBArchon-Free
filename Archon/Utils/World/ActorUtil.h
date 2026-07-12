#pragma once
#include "../Utils.h"
#include "../../Core/Instance/Instances.hpp"

struct ActorInfo {
    AActor* Actor;
    FVector Location;
    FVector2D ScreenLocation;
    std::string Name;
    std::string ClassName;
    std::string DisplayName;
    bool CanVisible;
    int Index;
    float Distance; 
        
    ActorInfo() : Actor(nullptr), Location(0,0,0), ScreenLocation(0,0), CanVisible(true), Index(-1), Distance(0.0f) {}

    bool operator==(const ActorInfo& other) const
    {
        return Actor == other.Actor;
    }
};
    
namespace ActorUtil
{
    template<typename T>
    std::vector<ActorInfo> GetActors() {
        UClass* ActorClass = T::StaticClass();
         
        if (!ActorClass) return std::vector<ActorInfo>();
        
        auto GWorld = Instances::GetWorld();
        if (!GWorld) return std::vector<ActorInfo>();
        
        auto* GPStatics = Instances::GetGPStatics();
        if (!GPStatics) return std::vector<ActorInfo>();
        
        auto* Engine = Instances::GetEngine();
        if (!Engine) return std::vector<ActorInfo>();
        
        auto PlayerController = Instances::GetPlayerController();
        if (!PlayerController) return std::vector<ActorInfo>();
        
        FVector PlayerLocation = PlayerController->K2_GetActorLocation();
        if (PlayerLocation.IsZero()) return std::vector<ActorInfo>();
        
        std::vector<ActorInfo> ActorInfos;

        TArray<AActor*> actors;
        GPStatics->GetAllActorsOfClass(GWorld, ActorClass, &actors);
    
        for (int32 i = 0; i < actors.Num(); i++)
        {
            AActor* actor = actors[i];
        
            if (!actor || !actor->Class) continue;
            
            ActorInfo info;
            info.Actor = actor;
            info.Name = actor->GetName();
            info.ClassName = actor->Class->GetName();
            info.Index = i;
            info.Location = actor->K2_GetActorLocation();
            info.DisplayName = info.ClassName;

            if (ActorClass == ACharacter::StaticClass() && actor->IsA(ABPCharacter_Demo_C::StaticClass())) continue;
            
            if (actor->IsA(ABP_DroppedItem_C::StaticClass()))
            {
                auto droppedItem = static_cast<ABP_DroppedItem_C*>(actor);
                if (droppedItem->ItemMesh)
                {
                    info.Location = droppedItem->ItemMesh->K2_GetComponentLocation();
                } else
                {
                    info.Location = actor->K2_GetActorLocation();
                }
                
                size_t prefixPos = info.ClassName.find("BP_DroppedItem_");
                if (prefixPos != std::string::npos) {
                    size_t startPos = prefixPos + 15;
                    size_t endPos = info.ClassName.find("_C", startPos);
                    if (endPos != std::string::npos) {
                        info.DisplayName = info.ClassName.substr(startPos, endPos - startPos);
                    } else {
                        info.DisplayName = info.ClassName.substr(startPos);
                    }
                } else
                {
                    info.DisplayName = info.ClassName;
                }
            }
            else if (actor->IsA(ABPCharacter_Demo_C::StaticClass()))
            {
                auto player = static_cast<ABPCharacter_Demo_C*>(actor);
                auto StateBP = static_cast<AMP_PS_C*>(player->PlayerState);
                if (player && StateBP)
                {
                    info.DisplayName = Utils::ReadWideStringFromAddress(StateBP->PlayerNamePrivate, 128);
                } else
                {
                    info.DisplayName = info.ClassName;
                }
            }
            else if (actor->IsA(ACharacter::StaticClass()))
            {
                auto entity = static_cast<ACharacter*>(actor);
                info.DisplayName = Utils::CleanClassName(info.ClassName);
            }
            
            
            if (info.Location.IsZero()) continue;
            
            info.CanVisible = PlayerController->ProjectWorldLocationToScreen(
                    info.Location,
                    &info.ScreenLocation,
                    true);
            
            info.Distance = PlayerLocation.GetDistanceTo(info.Location);
            

            ActorInfos.push_back(info);
        }
        return ActorInfos;
    }
}