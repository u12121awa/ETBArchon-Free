#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"

using namespace Render2DUtil;

class ESP : public Module
{
public:
    BooleanSetting* player = New<>(BooleanSetting(xorstr_("Player"), true));
    BooleanSetting* monster = New<>(BooleanSetting(xorstr_("Monster"), true));

    BooleanSetting* sanity = New<>(BooleanSetting(xorstr_("Sanity"), true, [this] { return player->value; }));

    ModeSetting* playerColorMode = New<>(ModeSetting(xorstr_("PlayerColorMode"), 1, std::vector<std::string>{xorstr_("Custom"), xorstr_("Sky")},[this] { return player->value; }));
    ColorSetting* playerColor = New<>(ColorSetting(xorstr_("PlayerColor"), RGBA(0, 255, 0, 255).ToU32(), [this]{return player->value && playerColorMode->index == 0;}));
    ModeSetting* monsterColorMode = New<>(ModeSetting(xorstr_("MonsterColorMode"), 1, std::vector<std::string>{xorstr_("Custom"), xorstr_("Sky")},[this] { return monster->value; }));
    ColorSetting* monsterColor = New<>(ColorSetting(xorstr_("MonsterColor"), RGBA(0, 255, 0, 255).ToU32(), [this]{return monster->value && monsterColorMode->index == 0;}));

    BooleanSetting* playerCity = New<>(BooleanSetting(xorstr_("PlayerCityBox"), true, [this] { return player->value; }));
    NumberSetting* playerCitySize = New<>(NumberSetting(xorstr_("PlayerCitySize"), 0.1f, 0.05f, 0.6f, xorstr_("%.2f"),[this] { return playerCity->value && player->value; }));
    NumberSetting* playerThickness = New<>(NumberSetting(xorstr_("PlayerThickness"), 2.0f, 1.0f, 3.0f, xorstr_("%.0f"),[this] { return player->value; }));
   
    BooleanSetting* monsterCity = New<>(BooleanSetting(xorstr_("MonsterCityBox"), false, [this] { return monster->value; }));
    NumberSetting* monsterCitySize = New<>(NumberSetting(xorstr_("MonsterCitySize"), 0.1f, 0.05f, 0.6f, xorstr_("%.2f"),[this] { return monsterCity->value && monster->value; }));
    NumberSetting* monsterThickness = New<>(NumberSetting(xorstr_("MonsterThickness"), 2.0f, 1.0f, 3.0f, xorstr_("%.0f"),[this] { return monster->value; }));
   
    enum EntityType
    {
        Monster,
        Player,
    };

    struct EntityInfo
    {
        ABPCharacter_Demo_C* Player;
        ACharacter* Monster;
        EntityType EntityType;
        FVector Location;
        FVector2D ScreenLocation;
        FVector4 RenderPosition;
        float Sanity;
        float MaxSanity;
        float Distance;
        bool CanVisible;
    };

    bool Running;
    CRITICAL_SECTION Entity_CriticalSection;
    std::vector<EntityInfo> EntityInfos;

    ESP() : Module("ESP")
    {
        Running = true;
        InitializeCriticalSection(&Entity_CriticalSection);
        EntityInfos.reserve(1000);
    }

    void OnShutdownModule() override
    {
        Running = false;
        DeleteCriticalSection(&Entity_CriticalSection);
    }

    void OnUpdate() override
    {
        if (!Running)
        {
            EntityInfos.clear();
            return;
        }
        EnterCriticalSection(&Entity_CriticalSection);
        EntityInfos.clear();
        EntityInfos = _GetEntityInfos();
        LeaveCriticalSection(&Entity_CriticalSection);
    }

    void OnRender2D(DX12DrawContext context) override
    {
        auto PC = Instances::GetPlayerCharacter();
        auto Controller = Instances::GetPlayerController();
        if (!Controller || !Running) return;

        std::vector<EntityInfo> entityInfos = GetEntityInfos();
        ImDrawList* dl = context.BackGroundDrawList;
        

        for (const auto& entityInfo : entityInfos)
        {
            if (!entityInfo.CanVisible) continue;

            if (entityInfo.EntityType == Player)
            {
                if (entityInfo.Player == PC) continue;
                RGBA c0 = GetColor(0);
                RGBA c90 = GetColor(90);
                RGBA c180 = GetColor(180);
                RGBA c270 = GetColor(270);
                
                
                if (playerColorMode->index == 0)
                {
                    c0 = c90 = c180 = c270 = RGBA(playerColor->GetU32());
                }
                float t = playerThickness->value;
                
                float x1 = entityInfo.RenderPosition.X; // Left
                float y1 = entityInfo.RenderPosition.Y; // Top
                float x2 = entityInfo.RenderPosition.Z; // Right
                float y2 = entityInfo.RenderPosition.W; // Bottom

                if (playerCity->value)
                {
                    float height = y2 - y1;
                    float boxSize = height * playerCitySize->value;

                    if (boxSize > (x2 - x1) / 2) boxSize = (x2 - x1) / 2;

                    RectMultiColor(dl, x1, y1, x1 + boxSize, y1 + t, c0, c180, c180, c0);
                    RectMultiColor(dl, x1, y1, x1 + t, y1 + boxSize, c270, c0, c0, c270);

                    RectMultiColor(dl, x2 - boxSize, y1, x2, y1 + t, c0, c180, c180, c0);
                    RectMultiColor(dl, x2 - t, y1, x2, y1 + boxSize, c270, c0, c0, c270);

                    RectMultiColor(dl, x1, y2 - t, x1 + boxSize, y2, c180, c90, c90, c180);
                    RectMultiColor(dl, x1, y2 - boxSize, x1 + t, y2, c180, c270, c270, c90);

                    RectMultiColor(dl, x2 - boxSize, y2 - t, x2, y2, c90, c90, c90, c180);
                    RectMultiColor(dl, x2 - t, y2 - boxSize, x2, y2, c90, c270, c270, c90);
                }
                else
                {
                    RectMultiColor(dl, x1 - t, y1, x1, y2, c270, c0, c0, c270);
                    RectMultiColor(dl, x1, y2 - t, x2, y2, c0, c180, c180, c0);
                    RectMultiColor(dl, x1 - t, y1, x2, y1 + t, c180, c90, c90, c180);
                    RectMultiColor(dl, x2 - t, y1, x2, y2, c90, c270, c270, c90);
                }

                if (sanity->value)
                {
                    float healthFactor = entityInfo.Sanity / entityInfo.MaxSanity;
                    if (healthFactor > 1.0f) healthFactor = 1.0f;
                    if (healthFactor < 0.0f) healthFactor = 0.0f;

                    float hpX1 = x1 - 5.0f * 2.0f;
                    float hpX2 = x1 - 3.0f * 2.0f;

                    float height = y2 - y1;
                    float fillHeight = height * healthFactor;

                    RGBA black = RGBA(0, 0, 0, 255);
                    RectMultiColor(dl, hpX1, y1, hpX2, y2, black, black, black, black);

                    float fillTop = y2 - fillHeight;

                    RGBA colTop, colBottom;

                    if (playerColorMode->index == 0)
                    {
                        RGBA healthC = RGBA(GetSanityColor(entityInfo.Sanity));

                        colTop = healthC;
                        colBottom = healthC;
                    }
                    else
                    {
                        colTop = c90;
                        colBottom = c270;
                    }

                    RectMultiColor(dl, hpX1, fillTop, hpX2, y2, colTop, colTop, colBottom, colBottom);
                }
            }
            else 
            {
                RGBA c0 = GetColor(0);
                RGBA c90 = GetColor(90);
                RGBA c180 = GetColor(180);
                RGBA c270 = GetColor(270);

                if (monsterColorMode->index == 0)
                {
                    c0 = c90 = c180 = c270 = RGBA(playerColor->GetU32());
                }
                float t = monsterThickness->value;
                
                float x1 = entityInfo.RenderPosition.X; // Left
                float y1 = entityInfo.RenderPosition.Y; // Top
                float x2 = entityInfo.RenderPosition.Z; // Right
                float y2 = entityInfo.RenderPosition.W; // Bottom

                if (monsterCity->value)
                {
                    float height = y2 - y1;
                    float boxSize = height * monsterCitySize->value;

                    if (boxSize > (x2 - x1) / 2) boxSize = (x2 - x1) / 2;

                    RectMultiColor(dl, x1, y1, x1 + boxSize, y1 + t, c0, c180, c180, c0);
                    RectMultiColor(dl, x1, y1, x1 + t, y1 + boxSize, c270, c0, c0, c270);

                    RectMultiColor(dl, x2 - boxSize, y1, x2, y1 + t, c0, c180, c180, c0);
                    RectMultiColor(dl, x2 - t, y1, x2, y1 + boxSize, c270, c0, c0, c270);

                    RectMultiColor(dl, x1, y2 - t, x1 + boxSize, y2, c180, c90, c90, c180);
                    RectMultiColor(dl, x1, y2 - boxSize, x1 + t, y2, c180, c270, c270, c90);

                    RectMultiColor(dl, x2 - boxSize, y2 - t, x2, y2, c90, c90, c90, c180);
                    RectMultiColor(dl, x2 - t, y2 - boxSize, x2, y2, c90, c270, c270, c90);
                }
                else
                {
                    RectMultiColor(dl, x1 - t, y1, x1, y2, c270, c0, c0, c270);
                    RectMultiColor(dl, x1, y2 - t, x2, y2, c0, c180, c180, c0);
                    RectMultiColor(dl, x1 - t, y1, x2, y1 + t, c180, c90, c90, c180);
                    RectMultiColor(dl, x2 - t, y1, x2, y2, c90, c270, c270, c90);
                }
            }
        }
    }
    
    ImU32 GetSanityColor(float health)
    {
        if (health > 75.0f)
        {
            return IM_COL32(0, 255, 0, 255);
        }

        if (health > 35.0f)
        {
            return IM_COL32(255, 255, 0, 255);
        }

        return IM_COL32(255, 50, 50, 255);
    }
    
    std::vector<EntityInfo> GetEntityInfos()
    {
        EnterCriticalSection(&Entity_CriticalSection);
        auto players = EntityInfos;
        LeaveCriticalSection(&Entity_CriticalSection);
        return players;
    }


    std::vector<EntityInfo> _GetEntityInfos()
    {
        auto GWorld = Instances::GetWorld();
        if (!GWorld) return std::vector<EntityInfo>();

        auto* GPStatics = Instances::GetGPStatics();
        if (!GPStatics) return std::vector<EntityInfo>();

        auto* Engine = Instances::GetEngine();
        if (!Engine) return std::vector<EntityInfo>();

        auto PlayerController = Instances::GetPlayerController();
        if (!PlayerController) return std::vector<EntityInfo>();

        FVector PlayerLocation = Instances::GetPlayerLocation();
        if (PlayerLocation.IsZero()) return std::vector<EntityInfo>();

        std::vector<EntityInfo> EntityInfos;

        TArray<AActor*> actors;

        if (player->value)
        {
            GPStatics->GetAllActorsOfClass(GWorld, ABPCharacter_Demo_C::StaticClass(), &actors);

            for (int32 i = 0; i < actors.Num(); i++)
            {
                AActor* actor = actors[i];

                if (!actor || !actor->Class) continue;

                auto player = static_cast<ABPCharacter_Demo_C*>(actor);

                if (!player) continue;

                FVector worldPos = player->K2_GetActorLocation();

                EntityInfo info;
                info.EntityType = Player;
                info.Player = player;
                info.Location = worldPos;
                info.Sanity = player->PlayerState ? static_cast<AMP_PS_C*>(player->PlayerState)->Sanity : 0;
                info.Sanity = player->PlayerState ? static_cast<AMP_PS_C*>(player->PlayerState)->MaxSanity : 0;

                info.Distance = PlayerLocation.GetDistanceTo(worldPos);
                FVector2D screenPos;
                PlayerController->ProjectWorldLocationToScreen(worldPos, &screenPos, true);
                info.ScreenLocation = screenPos;
                FVector4 BoxRect;
                if (Utils::GetActor2DBox(player, PlayerController, BoxRect))
                {
                    info.RenderPosition = BoxRect;
                    info.CanVisible = true;
                }
                else
                {
                    info.CanVisible = false;
                }

                EntityInfos.push_back(info);
            }
        }

        if (!monster->value) return EntityInfos;

        GPStatics->GetAllActorsOfClass(GWorld, ACharacter::StaticClass(), &actors);

        for (int32 i = 0; i < actors.Num(); i++)
        {
            AActor* actor = actors[i];

            if (!actor || !actor->Class || actor->IsA(ABPCharacter_Demo_C::StaticClass())) continue;

            auto monster = static_cast<ACharacter*>(actor);

            if (!monster) continue;

            FVector worldPos = monster->K2_GetActorLocation();
            
            EntityInfo info;
            info.EntityType = Monster;
            info.Monster = monster;
            info.Location = worldPos;
            info.Distance = PlayerLocation.GetDistanceTo(worldPos);
            FVector2D screenPos;
            PlayerController->ProjectWorldLocationToScreen(worldPos, &screenPos, true);
            info.ScreenLocation = screenPos;
            FVector4 BoxRect;
            if (Utils::GetActor2DBox(monster, PlayerController, BoxRect))
            {
                info.RenderPosition = BoxRect;
                info.CanVisible = true;
            }
            else
            {
                info.CanVisible = false;
            }

            EntityInfos.push_back(info);
        }

        return EntityInfos;
    }
};
