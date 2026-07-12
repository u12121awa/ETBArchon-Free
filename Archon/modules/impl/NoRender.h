#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"
#include "../../Utils/World/WorldUtils.h"
#include "../../Utils/World/FTickCallbackSystem.h"

class NoRender : public Module
{
public:
    BooleanSetting* splashScreen = New<>(BooleanSetting(xorstr_("SplashScreen"), true));
    BooleanSetting* wallMaterial = New<>(BooleanSetting(xorstr_("WallMaterial"), true));

    UMaterialInstanceDynamic* WallMaterial;
    AActor* SunnyActor;
    AActor* RainActor;

    NoRender() : Module(xorstr_("NoRender"))
    {
        WallMaterial = nullptr;
        SunnyActor = nullptr;
        RainActor = nullptr;
    }

    void OnShutdownModule() override
    {
        OnDisable();
    }

    void OnDisable() override
    {
        if (SunnyActor)
        {
            SunnyActor->K2_DestroyActor();
            SunnyActor = nullptr;
        }
        if (RainActor)
        {
            RainActor->K2_DestroyActor();
            RainActor = nullptr;
        }
    }

    void OnWorldChange() override
    {
        SunnyActor = nullptr;
        RainActor = nullptr;
        WallMaterial = nullptr;
        if (Instances::GetLevelData()->LevelName == xorstr_("TransitionLevel")) return;
        OnUseMaterial();
    }

    void OnFunctionEvent(FunctionEvent* event) override
    {
        if (wallMaterial && event->functionName == xorstr_(
            "Function BP_Scanner.BP_Scanner_C.BndEvt__BP_Scanner_LIDARComponent_K2Node_ComponentBoundEvent_0_ScannerTraceSignature__DelegateSignature"))
        {
            event->cancel = true;
        }
        
    }

    void OnUpdate() override
    {
        OnRunSplashScreen();
    }


  

    void OnRunSplashScreen()
    {
        if (!splashScreen->value) return;

        static ULONGLONG SplashScreenEntryTime = 0;
        static bool bHasSkippedSplash = false;

        auto CurLevelData = Instances::GetLevelData();

        auto CurLevel = CurLevelData->CurLevel;

        bool bIsSplashMap = CurLevelData->LevelName == xorstr_("SplashScreenMap");

        if (bIsSplashMap)
        {
            if (SplashScreenEntryTime == 0)
            {
                SplashScreenEntryTime = GetTickCount64();
                bHasSkippedSplash = false;
            }
            else if (!bHasSkippedSplash)
            {
                ULONGLONG CurrentTick = GetTickCount64();

                if (CurrentTick - SplashScreenEntryTime >= 1000)
                {
                    auto* GS = Instances::GetGPStatics();
                    FName LevelName = Instances::GetStringLib()->Conv_StringToName(xorstr_(L"MainMenuMap")); 
                        
                    UObject* WorldContext = Instances::GetWorld(); 
                        
                    GS->OpenLevel(WorldContext, LevelName, true, xorstr_(L""));
                    Log::Info(xorstr_("[NoRender] EZ Run SplashScreen"));
                    bHasSkippedSplash = true;
                }
            }
        }
        else
        {
            SplashScreenEntryTime = 0;
            bHasSkippedSplash = false;
        }
    }

    void OnUseMaterial()
    {
        if (!wallMaterial->value) return;
        auto World = Instances::GetWorld();
        if (!World || !World->PersistentLevel) return;

        auto CurLevelData = Instances::GetLevelData();


        bool bIsLightsOut = CurLevelData->LevelName == xorstr_("LightsOut");
        if (bIsLightsOut)
        {
            if (!WallMaterial || !Utils::ShouldUsePointer64(WallMaterial))
            {
                UMaterialInterface* material = Instances::FindObject<UMaterialInterface>(xorstr_("M_Master_Walls"));

                if (material)
                {
                    FName name = Instances::GetStringLib()->Conv_StringToName(xorstr_(L"WallHackMat"));
                    WallMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(
                        World, material, name, EMIDCreationFlags::None);

                    if (WallMaterial)
                    {
                        FName colorParam = Instances::GetStringLib()->Conv_StringToName(xorstr_(L"Color"));
                        WallMaterial->SetVectorParameterValue(colorParam, FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                }
            }

            TArray<ULevel*> AllLevels = World->Levels;
            if (!AllLevels || AllLevels.Num() <= 0) return;

            for (int i = 0; i < AllLevels.Num(); i++)
            {
                ULevel* CurrentLevel = AllLevels[i];
                if (!CurrentLevel || !Utils::ShouldUsePointer64(CurrentLevel)) continue;

                TArray<AActor*> LevelActors = CurrentLevel->Actors;
                if (!LevelActors || LevelActors.Num() <= 0) continue;

                for (int k = 0; k < LevelActors.Num(); k++)
                {
                    AActor* Actor = LevelActors[k];
                    if (!Actor || !Utils::ShouldUsePointer64(Actor)) continue;

                    if (Actor->GetFullName().find(xorstr_("SM_Wall")) != std::string::npos)
                    {
                        TArray<UActorComponent*> Components = Actor->K2_GetComponentsByClass(
                            UStaticMeshComponent::StaticClass());

                        for (int j = 0; j < Components.Num(); j++)
                        {
                            UStaticMeshComponent* staticMeshComponent = static_cast<UStaticMeshComponent*>(
                                Components[j]);

                            if (!staticMeshComponent || !Utils::ShouldUsePointer64(staticMeshComponent)) continue;

                            if (!WallMaterial) break;

                            if (staticMeshComponent->GetMaterial(0)->GetFullName() ==
                                xorstr_("MaterialInstanceConstant MI_UnlitBlack.MI_UnlitBlack"))
                            {
                                staticMeshComponent->SetMaterial(0, WallMaterial);
                            }
                        }
                    }
                }
            }

            Log::Info(xorstr_("[NoRender] Wall Material replaced successfully"));
        }
    }
};
