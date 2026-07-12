#pragma once

#include <windows.h>

#include "../../Core/Instance/Instances.hpp"


enum class ESpawnActorNameMode : uint8_t
{
    Required_Fatal,
    Required_ErrorAndReturnNull,
    Required_ReturnNull,
    Requested
};

struct FActorSpawnParameters
{
    FActorSpawnParameters()
        : Name({ 0, 0 }), Template(nullptr), Owner(nullptr), Instigator(nullptr), OverrideLevel(nullptr),
        SpawnCollisionHandlingOverride(ESpawnActorCollisionHandlingMethod::Undefined),
        BitFields(0),
        NameMode(ESpawnActorNameMode::Required_Fatal),
        Padding(0),
        ObjectFlags(EObjectFlags::Transactional)
    {
    }

    FName Name;
    AActor* Template;
    AActor* Owner;
    APawn* Instigator;
    ULevel* OverrideLevel;
    ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;
    
    uint8_t BitFields; 
    
    ESpawnActorNameMode NameMode;
    uint8_t Padding;
    EObjectFlags ObjectFlags;
    
    void SetNoFail(bool enable) {
        if (enable) BitFields |= (1 << 1);
        else BitFields &= ~(1 << 1);
    }
};

namespace WorldUtils
{
    constexpr uintptr_t OFFSET_SpawnActor = 0x2B822E0;
    constexpr uintptr_t OFFSET_StaticLoadObject = 0x1353A10;
    constexpr uintptr_t OFFSET_AddNetworkActor = 0x2EFC950;
    constexpr uintptr_t OFFSET_RemoveNetworkActor = 0x2F11820;
    constexpr uintptr_t OFFSET_PostSpawnInitialize = 0x2838110;
    
    typedef AActor* (*SpawnActor_t)(UWorld* World, UClass* Class, const FTransform* UserTransform, const FActorSpawnParameters& SpawnParameters);
    typedef UObject* (*StaticLoadObject_t)(UClass* Class, UObject* InOuter, const wchar_t* Name, const wchar_t* Filename, uint32_t LoadFlags, void* Sandbox,bool bAllowObjectReconciliation, const void* InstancingContext);

    typedef void (*PostSpawnInitialize_t)(AActor* ThisActor, const FTransform* SpawnTransform, AActor* InOwner, APawn* InInstigator, bool bRemoteOwned, bool bNoFail, bool bDeferConstruction);

    typedef void (*AddNetworkActor_t)(UWorld* World, AActor* Actor);
    typedef void (*RemoveNetworkActor_t)(UWorld* World, AActor* Actor);
    
    inline uintptr_t GetBaseAddress() {
        return (uintptr_t)GetModuleHandleA(nullptr);
    }

    __declspec(noinline) inline void PostSpawnInitialize(
        AActor* Actor, 
        const FTransform& Transform, 
        AActor* InOwner = nullptr, 
        APawn* InInstigator = nullptr, 
        bool bRemoteOwned = false, 
        bool bNoFail = false, 
        bool bDeferConstruction = false
    )
    {
        if (!Actor) return;
        
        static auto fnPostSpawnInitialize = (PostSpawnInitialize_t)(GetBaseAddress() + OFFSET_PostSpawnInitialize);
        
        fnPostSpawnInitialize(Actor, &Transform, InOwner, InInstigator, bRemoteOwned, bNoFail, bDeferConstruction);
    }
    
    __declspec(noinline) inline AActor* SpawnActor(UWorld* World, UClass* Class, FTransform const* Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters())
    {
        if (!World || !Class) return nullptr;
        
        static auto fnSpawnActor = (SpawnActor_t)(GetBaseAddress() + OFFSET_SpawnActor);
        
        
        return fnSpawnActor(World, Class, Transform, SpawnParameters);
    }

    __declspec(noinline) inline AActor* SpawnActor(UClass* Class, FTransform Transform, bool bForceSpawn = true)
    {
        auto World = UWorld::GetWorld();
        if (!World || !Class) return nullptr;
        
        FActorSpawnParameters Params;
        if (bForceSpawn) {
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Params.SetNoFail(true);
        }
        
        if (World->OwningGameInstance && World->OwningGameInstance->LocalPlayers[0]) {
            Params.Owner = (AActor*)World->OwningGameInstance->LocalPlayers[0]->PlayerController;
        }
        
        return SpawnActor(World, Class, &Transform, Params);
    }
    
    __declspec(noinline) inline void AddNetworkActor(UWorld* World, AActor* Actor)
    {
        if (!World || !Actor) return;

        static auto fnAddNetworkActor = (AddNetworkActor_t)(GetBaseAddress() + OFFSET_AddNetworkActor);

        fnAddNetworkActor(World, Actor);
    }

    __declspec(noinline) inline void RemoveNetworkActor(UWorld* World, AActor* Actor)
    {
        if (!World || !Actor) return;
        
        static auto fnRemoveNetworkActor = (RemoveNetworkActor_t)(GetBaseAddress() + OFFSET_RemoveNetworkActor);
        
        fnRemoveNetworkActor(World, Actor);
    }

    template <typename T>
    T* LoadObject(const std::wstring& Path)
    {
        uintptr_t base = GetBaseAddress();
        auto fnStaticLoadObject = (StaticLoadObject_t)(base + OFFSET_StaticLoadObject);
        
        auto TargetClass = UClass::StaticClass(); 
        
        UObject* Obj = fnStaticLoadObject(
            TargetClass,    // Class
            nullptr,        // InOuter
            Path.c_str(),   // Name
            nullptr,        // Filename
            0,              // LoadFlags (LOAD_one)
            nullptr,        // Sandbox
            true,           // bAllowObjectReconciliation
            nullptr         // InstancingContext
        );

        return static_cast<T*>(Obj);
    }

    float GetPing();
}