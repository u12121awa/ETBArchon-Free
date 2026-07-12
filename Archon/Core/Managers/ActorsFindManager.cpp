#include "pch.h"
#include "ActorsFindManager.h"

#include "../Memory.h"
#include "../../Utils/World/FTickCallbackSystem.h"

namespace
{
    bool Running;
    std::vector<ActorInfo> ItemInfos;
    CRITICAL_SECTION Item_CriticalSection;
    std::vector<ActorInfo> PlayerInfos;
    CRITICAL_SECTION Player_CriticalSection;
    std::vector<ActorInfo> EntityInfos;
    CRITICAL_SECTION Entity_CriticalSection;
    std::vector<ActorInfo> AllActorInfos;
    CRITICAL_SECTION AllActor_CriticalSection;
}

namespace ActorsFind
{

    DataCollector::DataCollector() 
    {
        Running = false;
        InitializeCriticalSection(&Item_CriticalSection);
        InitializeCriticalSection(&Player_CriticalSection);
        InitializeCriticalSection(&Entity_CriticalSection);
        InitializeCriticalSection(&AllActor_CriticalSection);
        ItemInfos.reserve(1000);
        PlayerInfos.reserve(1000);
        EntityInfos.reserve(1000);
        AllActorInfos.reserve(1000);
    }
    
    DataCollector::~DataCollector() {
        StopCollection();
        DeleteCriticalSection(&Item_CriticalSection);
        DeleteCriticalSection(&Player_CriticalSection);
        DeleteCriticalSection(&Entity_CriticalSection);
        DeleteCriticalSection(&AllActor_CriticalSection);
    }
    
    void DataCollector::StartCollection() {
        if (Running) return;
        Running = true;
    }
    
    void DataCollector::StopCollection() {
        Running = false;
    }
    
    
    void ActorsFind::onUpdate()
    {
        if (!Running)
        {
            ItemInfos.clear();
            PlayerInfos.clear();
            EntityInfos.clear();
            AllActorInfos.clear();
            return;
        }
        
        EnterCriticalSection(&Item_CriticalSection);
        ItemInfos.clear();
        if (Config::Settings::ItemESP) ItemInfos = ActorUtil::GetActors<ABP_DroppedItem_C>();
        LeaveCriticalSection(&Item_CriticalSection);

        EnterCriticalSection(&Player_CriticalSection);
        PlayerInfos.clear();
        if (Config::Settings::PlayerESP) PlayerInfos = ActorUtil::GetActors<ABPCharacter_Demo_C>();
        LeaveCriticalSection(&Player_CriticalSection);

        EnterCriticalSection(&Entity_CriticalSection);
        EntityInfos.clear();
        if (Config::Settings::EntityESP) EntityInfos = ActorUtil::GetActors<ACharacter>();
        LeaveCriticalSection(&Entity_CriticalSection);
        
        EnterCriticalSection(&AllActor_CriticalSection);
        AllActorInfos.clear();
        if (Config::Settings::ActorESP) AllActorInfos = ActorUtil::GetActors<AActor>();
        LeaveCriticalSection(&AllActor_CriticalSection);
    }

    std::vector<ActorInfo> ActorsFind::GetItemInfos()
    {
        EnterCriticalSection(&Item_CriticalSection);
        auto items = ItemInfos;
        LeaveCriticalSection(&Item_CriticalSection);
        return items;
    }

    std::vector<ActorInfo> ActorsFind::GetPlayerInfos()
    {
        EnterCriticalSection(&Player_CriticalSection);
        auto players = PlayerInfos;
        LeaveCriticalSection(&Player_CriticalSection);
        return players;
    }

    std::vector<ActorInfo> ActorsFind::GetEntityInfos()
    {
        EnterCriticalSection(&Entity_CriticalSection);
        auto entities = EntityInfos;
        LeaveCriticalSection(&Entity_CriticalSection);
        return entities;
    }

    std::vector<ActorInfo> ActorsFind::GetAllActorInfos()
    {
        EnterCriticalSection(&AllActor_CriticalSection);
        auto allActors = AllActorInfos;
        LeaveCriticalSection(&AllActor_CriticalSection);
        return allActors;
    }
}



