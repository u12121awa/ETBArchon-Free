#pragma once
#include <mutex>
#include <unordered_set>

#include "../Module.h"

class Test : public Module
{
public:
    BooleanSetting* isRecordingSpam = New<>(BooleanSetting(xorstr_("RecordingSpam"), true));
    
    std::unordered_set<std::string> BlockedFunctions;
    std::mutex SpamMutex;
    UMapEditorHandlerComponent* Component;
    AGameModeBase* GameMode;
    
    Test() : Module(xorstr_("Test"))
    {
        GameMode = nullptr;
        Component = nullptr;
    }

    void OnWorldChange() override
    {
        GameMode = nullptr;
        Component = nullptr;
    }

    void OnRespawnChange() override
    {
        
    }
    void OnEnable() override
    {
        std::lock_guard lock(SpamMutex);
        BlockedFunctions.clear();
    }
    /*
    void OnUpdate() override
    {
        auto* World = Instances::GetWorld();
        auto* GPStatics = Instances::GetGPStatics();
        if (!World) return;
        
        if (!GameMode)
        {
            GameMode = GPStatics->GetGameMode(World);
        }
        
        if (GameMode)
        {
            TArray<UActorComponent*> Components = GameMode->K2_GetComponentsByClass(UMapEditorHandlerComponent::StaticClass());

            if (Components.Num() > 0) {
                for (int32 i = 0; i < Components.Num(); i++)
                {
                    UActorComponent* component = Components[i];
                    Component = static_cast<UMapEditorHandlerComponent*>(component);
                    break;
                }
            } else
            {
                FTransform T = {};
                T.Rotation = {0.0f, 0.0f, 0.0f};
                T.Translation = {0.0f, 0.0f, 0.0f};
                T.Scale3D = {1.0f, 1.0f, 1.0f};

                UActorComponent* NewComp = GameMode->AddComponentByClass(UMapEditorHandlerComponent::StaticClass(), false, T, true);

                if (NewComp)
                {
                    GameMode->FinishAddComponent(NewComp, false, T);
                    Component = static_cast<UMapEditorHandlerComponent*>(NewComp);
                }
            }
        }

        if (!GameMode || !Component) return;

        TArray<AActor*> actors;
        
        GPStatics->GetAllActorsOfClass(World, ABPCharacter_Demo_C::StaticClass(), &actors);

        for (int32 i = 0; i < actors.Num(); i++)
        {
            AActor* actor = actors[i];
            FTransform transform = actor->GetTransform();
            
            transform.Scale3D = {0.5f, 0.5f, 0.5f};
            Component->Server_ReplicateTransform(actor, transform);
          
        }
    }
    */
    

    void OnFunctionEvent(FunctionEvent* event) override
    {
        std::string fullName = event->functionName;
        
        std::lock_guard lock(SpamMutex);
        
        if (isRecordingSpam->value)
        {
            BlockedFunctions.insert(fullName);
        }
        else
        {
            if (BlockedFunctions.count(fullName) == 0)
            {
              
                    Log::Info(fullName);
                    
                    BlockedFunctions.insert(fullName); 
                
            }
        }
    }
};
