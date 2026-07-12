#include "pch.h"

#include "Instances.hpp"

#include "../../Archon.h"
#include "../../modules/impl/Flight.h"
#include "../../modules/impl/FreeCam.h"
#include "../../modules/impl/NoRender.h"
#include "../../Utils/Logger.h"
#include "../Managers/ModuleManager.h"

namespace Instances
{
    bool bInitialized = false;
    
    namespace
    {
        UWorld* LastGWorld = nullptr;
        UWorld* GWorld = nullptr;
        UEngine* Engine = nullptr;
        ULevel* CurLevel = nullptr;
        APawn* GLocalPawn = nullptr;
        ULocalPlayer* GLocalPlayer = nullptr;
        UGameplayStatics* GPStatics = nullptr;
        UKismetMathLibrary* MathLib = nullptr;
        UKismetTextLibrary* TextLib = nullptr;
        UKismetStringLibrary* StringLib = nullptr;
        UKismetSystemLibrary* SystemLib = nullptr;
        APlayerController* GPlayerController = nullptr;
        ABPCharacter_Demo_C* GPlayerCharacter = nullptr;
        ABPCharacter_Demo_C* LastGPlayerCharacter = nullptr;
        FVector PlayerLocation;

        LevelData* CurLevelData = new LevelData();

        std::map<std::string, UClass*> GStaticClasses;
        std::map<std::string, UFunction*> GStaticFunctions;
    }


    void MapObjects()
    {
        GStaticClasses.clear();
        GStaticFunctions.clear();

        for (int i = 0; i < UObject::GObjects->Num(); ++i)
        {
            UObject* obj = UObject::GObjects->GetByIndex(i);
            if (!obj) continue;

            if (obj->IsA(UClass::StaticClass()))
            {
                GStaticClasses.emplace(obj->GetFullName(), static_cast<UClass*>(obj));
            }
            else if (obj->IsA(UFunction::StaticClass()))
            {
                GStaticFunctions.emplace(obj->GetFullName(), static_cast<UFunction*>(obj));
            }
        }
        Log::Success(
            xorstr_("Mapped ") + std::to_string(GStaticClasses.size()) + xorstr_(" classes and ") + std::to_string(
                GStaticFunctions.size()) + xorstr_(" functions."));
    }

    void Initialize()
    {
        if (!bInitialized)
        {
            VMProtectBeginUltra("Initializing Instances");
            Engine = UEngine::GetEngine();
            MathLib = static_cast<UKismetMathLibrary*>(UObject::FindClassFast(xorstr_("KismetMathLibrary"))->DefaultObject);
            TextLib = static_cast<UKismetTextLibrary*>(UObject::FindClassFast(xorstr_("KismetTextLibrary"))->DefaultObject);
            GPStatics = static_cast<UGameplayStatics*>(UObject::FindClassFast(xorstr_("GameplayStatics"))->DefaultObject);
            StringLib = static_cast<UKismetStringLibrary*>(UObject::FindClassFast(xorstr_("KismetStringLibrary"))->DefaultObject);
            SystemLib = static_cast<UKismetSystemLibrary*>(UObject::FindClassFast(xorstr_("KismetSystemLibrary"))->DefaultObject);

            MapObjects();
            Update();
            Log::Success(xorstr_("Instances Initialized successfully."));

            bInitialized = true;
            VMProtectEnd();
        }
    }
    
    bool IsValidSteamID(uint64_t val) {
        return val > 76561190000000000ULL;
    }

    std::string GetSteamID(ABPCharacter_Demo_C* Target)
    {
        if (!Target || !Target->PlayerState) return "";
        
        uintptr_t BaseAddr = (uintptr_t)&Target->PlayerState->UniqueId;
        
        uintptr_t* PtrLocation = (uintptr_t*)(BaseAddr + 0x08);
        if (IsBadReadPtr(PtrLocation, 8)) return "";
    
        uintptr_t DeepPtr = *PtrLocation;
        
        if (!DeepPtr || DeepPtr < 0x10000 || IsBadReadPtr((void*)DeepPtr, 8)) return "";
        
        uint64_t* IDPtr = (uint64_t*)(DeepPtr + 0x18);
        
        if (IsBadReadPtr(IDPtr, 8)) return "";

        uint64_t SteamID = *IDPtr;
        
        if (IsValidSteamID(SteamID)) {
            return std::to_string(SteamID);
        }

        return "";
    }
    
    
    std::string GetHostSteamID(UWorld* World)
    {
        if (!World || !World->NetDriver || !World->NetDriver->ServerConnection) return "";
    
        uintptr_t ConnectionAddr = (uintptr_t)World->NetDriver->ServerConnection;
        
        uintptr_t StartAddr = ConnectionAddr + 0xA0; 
        uintptr_t EndAddr = StartAddr + 0x200;
    
        for (uintptr_t ptr = StartAddr; ptr < EndAddr; ptr += 8) 
        {
            struct FStringFake {
                uintptr_t Data;
                int32_t Count;
                int32_t Max;
            };
        
            FStringFake* FakeStr = (FStringFake*)ptr;
            
            if (FakeStr->Count <= 5 || FakeStr->Count > 200 || FakeStr->Count > FakeStr->Max) {
                continue; 
            }
            
            if (IsBadReadPtr((void*)FakeStr->Data, 8)) {
                continue;
            }

            FString* RealStr = (FString*)ptr;
            std::string str = RealStr->ToString(); 

            if (str.find(xorstr_("steam.")) != std::string::npos)
            {
                size_t found = str.find(xorstr_("steam."));
                std::string id = str.substr(found + 6);
                size_t colon = id.find(xorstr_(":"));
                if (colon != std::string::npos) id = id.substr(0, colon);
                return id;
            }
        }

        return "";
    }
    
    void Update(bool GameThread)
    {
        if (!bInitialized) return;
        
        GWorld = UWorld::GetWorld();
        if (!GWorld)
        {
            CurLevel = nullptr;
            CurLevelData->CurLevel = nullptr;
            CurLevelData->LevelName = "";
            CurLevelData->HostSteamID = "";
            GLocalPlayer = nullptr;
            GPlayerController = nullptr;
            GLocalPawn = nullptr;
            GPlayerCharacter = nullptr;
            return;
        }

        if (GameThread)
        {
            if (LastGWorld != GWorld)
            {
                CurLevel = GWorld->PersistentLevel;
                LastGWorld = GWorld;
                
                CurLevelData->CurLevel = CurLevel;
                CurLevelData->LevelName = GPStatics->GetCurrentLevelName(GWorld, true).ToString();

                Log::Info(std::string(xorstr_("Current Level: ") + CurLevelData->LevelName).c_str());

                if (GWorld->NetDriver && GWorld->NetDriver->ServerConnection)
                {
                    CurLevelData->HostSteamID = GetHostSteamID(GWorld);
                    if (CurLevelData->HostSteamID == xorstr_("76561198368991730"))
                    {
                        auto* GS = GetGPStatics();
                        FName LevelName = GetStringLib()->Conv_StringToName(xorstr_(L"MainMenuMap"));
                        UObject* WorldContext = GetWorld();
                        GS->OpenLevel(WorldContext, LevelName, true, xorstr_(L""));
                    }
                    else Log::Info(std::string(xorstr_("Host SteamID: ") + CurLevelData->HostSteamID).c_str(), RGBA(220, 0, 0, 255));
                }
                else
                {
                    CurLevelData->HostSteamID = "";
                }
                
                ModuleManager::Get().OnWorldChange();
            }
        }

        UGameInstance* gameInstance = GWorld->OwningGameInstance;
        if (gameInstance && gameInstance->LocalPlayers.IsValidIndex(0))
        {
            GLocalPlayer = gameInstance->LocalPlayers[0];
        }
        else
        {
            GLocalPlayer = nullptr;
            PlayerLocation = {0, 0, 0};
        }

        if (GLocalPlayer)
        {
            GPlayerController = GLocalPlayer->PlayerController;
        }
        else
        {
            GPlayerController = nullptr;
            if (GameThread) PlayerLocation = {0, 0, 0};
        }


        if (GPlayerController)
        {
            GLocalPawn = GPlayerController->K2_GetPawn();
        }
        else
        {
            GLocalPawn = nullptr;
            if (GameThread) PlayerLocation = {0, 0, 0};
        }


        if (GLocalPawn)
        {
            if (GLocalPawn && GLocalPawn->IsA(ABPCharacter_Demo_C::StaticClass()))
            {
                GPlayerCharacter = static_cast<ABPCharacter_Demo_C*>(GLocalPawn);
                if (GameThread) PlayerLocation = GPlayerCharacter->K2_GetActorLocation();
            }
            else
            {
                GPlayerCharacter = nullptr;
                if (GameThread) PlayerLocation = {0, 0, 0};
            }
        }
        else
        {
            GPlayerCharacter = nullptr;
        }

        if (GameThread)
        {
            if (LastGPlayerCharacter != GPlayerCharacter)
            {
                LastGPlayerCharacter = GPlayerCharacter;
                ModuleManager::Get().OnRespawnChange();
                Log::Info(xorstr_("Player Respawn"));
            }
        }
    }

    UGameplayStatics* GetGPStatics()
    {
        return GPStatics;
    }

    UKismetSystemLibrary* GetSystemLib()
    {
        return SystemLib;
    }

    UEngine* GetEngine()
    {
        return Engine;
    }

    UKismetTextLibrary* GetTextLib()
    {
        return TextLib;
    }

    UKismetStringLibrary* GetStringLib()
    {
        return StringLib;
    }

    UWorld* GetWorld()
    {
        return GWorld;
    }

    LevelData* GetLevelData()
    {
        return CurLevelData;
    }

    APlayerController* GetPlayerController()
    {
        return GPlayerController;
    }

    ULocalPlayer* GetLocalPlayer()
    {
        return GLocalPlayer;
    }

    APawn* GetLocalPawn()
    {
        return GLocalPawn;
    }

    ABPCharacter_Demo_C* GetPlayerCharacter()
    {
        return GPlayerCharacter;
    }

    FVector GetPlayerLocation()
    {
        auto freecam = ModuleManager::Get().GetModule<FreeCam>();

        if (freecam && freecam->IsEnabled() && freecam->locationSync->value && !freecam->GetVirtualLocation().IsZero())
        {
            return freecam->GetVirtualLocation();
        }
        
        auto flight = ModuleManager::Get().GetModule<Flight>();
        
        return flight && flight->IsEnabled() && flight->locationSync->value && !flight->GetVirtualLocation().IsZero() ? flight->GetVirtualLocation() : PlayerLocation;
    }

    UClass* FindStaticClass(const std::string& className)
    {
        if (GStaticClasses.count(className))
        {
            return GStaticClasses[className];
        }
        return nullptr;
    }

    UFunction* FindStaticFunction(const std::string& functionName)
    {
        if (GStaticFunctions.count(functionName))
        {
            return GStaticFunctions[functionName];
        }
        return nullptr;
    }
}
