#pragma once

#include "../Config.h"
#include "../../Utils/World/ActorUtil.h"

namespace ActorsFind
{
    std::vector<ActorInfo> GetItemInfos();
    std::vector<ActorInfo> GetPlayerInfos();
    std::vector<ActorInfo> GetEntityInfos();
    std::vector<ActorInfo> GetAllActorInfos();
    void onUpdate();
    
    class DataCollector
    {
    public:
        DataCollector();
        ~DataCollector();
        
        void StartCollection();
        void StopCollection();
    };
}
