#pragma once
#include <Windows.h>
#include <cstdint>

namespace Config {
    namespace Settings {
        inline ABPCharacter_Demo_C* LastKnownPawn;

        inline bool ItemESP = false;
        inline bool PlayerESP = false;
        inline bool EntityESP = false;
        inline bool ActorESP = false;

        inline int SelectedItemIndex = 1;
        inline int SpawnCount = 1;
        inline int MaxPlayers = 16;
        inline bool SpeedHackEnabled;
        inline float SpeedValue;
        inline float OriginalWalkSpeed;
        inline float OriginalSprintSpeed;
        inline bool OriginalSpeedSaved;
        inline bool LastSpeedHackState;

        inline bool UnlockPlayersEnabled;

        inline bool InfiniteStaminaEnabled;
        inline bool LastStaminaState;
        inline bool InfiniteSanityEnabled;
        
        inline bool FlyHackEnabled;
        inline float FlySpeed;
        inline bool WasFlying;

        inline bool Unload;
    }
}
