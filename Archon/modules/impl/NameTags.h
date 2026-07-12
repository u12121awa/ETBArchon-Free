#pragma once
#include "../Module.h"
#include "../../Utils/World/ActorUtil.h"
#include <string>

#include "../../Archon.h"
#include "../../Core/Managers/FontManager.h"
#include "../../imgui/imgui.h"
#include "../../Utils/Auth/Auth.h"
#include "../../Utils/Render/AnimateUtil.h"
#include "../../Utils/Render/Shader2DUtil.h"

#define IM_COL32_A_MASK     0xFF000000
#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24

class NameTags : public Module
{
public:
    BooleanSetting* monster = New<>(BooleanSetting(xorstr_("Monster"), true));
    BooleanSetting* self = New<>(BooleanSetting(xorstr_("Self"), false));
    NumberSetting* scale = New<>(NumberSetting(xorstr_("Scale"), 20.0f, 1.0f, 50.f, "%.1f"));
    NumberSetting* playerOffsetY = New<>(NumberSetting(xorstr_("PlayerOffsetY"), 5.0f, 0.0f, 20.f, "%.1f"));
    NumberSetting* monsterOffsetY = New<>(NumberSetting(xorstr_("MonsterOffsetY"), 5.0f, 0.0f, 20.f, "%.1f"));
    
    BooleanSetting* ping = New<>(BooleanSetting(xorstr_("Ping"), true));
    BooleanSetting* steamID = New<>(BooleanSetting(xorstr_("SteamID"), false));
    BooleanSetting* distance = New<>(BooleanSetting(xorstr_("Distance"), true));
    BooleanSetting* sanity = New<>(BooleanSetting(xorstr_("Sanity"), true));
    NumberSetting* paddingX = New<>(NumberSetting(xorstr_("PaddingX"), 6.0f, 1.0f, 15.f, xorstr_("%.1f")));
    NumberSetting* paddingY = New<>(NumberSetting(xorstr_("PaddingY"), 2.0f, 1.0f, 15.f, xorstr_("%.1f")));

    ModeSetting* fontMode = New<>(ModeSetting(xorstr_("FontMode"), 0, std::vector<std::string>{xorstr_("Custom"), xorstr_("Normal")}));
    BooleanSetting* fontShadow = New<>(BooleanSetting(xorstr_("FontShadow"), false));
    ColorSetting* playerNameColor = New<>(ColorSetting(xorstr_("PlayerNameColor"), RGBA(255, 255, 255, 255).ToU32()));
    ColorSetting* ownerNameColor = New<>(ColorSetting(xorstr_("OwnerNameColor"), RGBA(255, 0, 0, 247).ToU32()));
    ColorSetting* steamIDColor = New<>(ColorSetting(xorstr_("SteamIDColor"), RGBA(ImVec4(1.0f, 0.84f, 0.0f, 1.0f)).ToU32(), [this] { return steamID->value; }));
    
    ColorSetting* monsterNameColor = New<>(ColorSetting(xorstr_("MonsterNameColor"), RGBA(255, 255, 0, 255).ToU32()));
 
    NumberSetting* rounding = New<>(NumberSetting(xorstr_("Rounding"), 6.0f, 0.0f, 10.0f, xorstr_("%.1f")));
    BooleanSetting* blurBackGround = New<>(BooleanSetting(xorstr_("BlurBackground"), true));
    NumberSetting* blurOpacity = New<>(NumberSetting(xorstr_("BlurOpacity"), 0.55f, 0.1f, 1.f, xorstr_("%.1f"), [this] { return blurBackGround->value; }));
    NumberSetting* blurStrength = New<>(NumberSetting(xorstr_("BlurStrength"), 20.0f, 1.0f, 50.f, xorstr_("%.1f"), [this] { return blurBackGround->value; }));
    ColorSetting* fillColor = New<>(ColorSetting(xorstr_("FillColor"), RGBA(47, 47, 47, 150).ToU32(), [this] { return !blurBackGround->value; }));
    ColorSetting* blurColor = New<>(ColorSetting(xorstr_("BlurColor"), RGBA(47, 47, 47, 255).ToU32(), [this] { return blurBackGround->value; }));

    NumberSetting* glowStrength = New<>(NumberSetting(xorstr_("GlowStrength"), 8.0f, 1.0f, 20.0f, xorstr_("%.2f")));
    NumberSetting* glowAlpha = New<>(NumberSetting(xorstr_("GlowAlpha"), 0.6f, 0.1f, 1.0f, xorstr_("%.2f")));
    ModeSetting* glowColorMode = New<>(ModeSetting(xorstr_("GlowColorMode"), 1, std::vector<std::string>{xorstr_("Custom"), xorstr_("Sky")}));
    ColorSetting* glowColor = New<>(ColorSetting(xorstr_("GlowColor"), RGBA(0, 14, 37, 255).ToU32(),[this] { return glowColorMode->index == 0; }));

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
        AMP_PS_C* PlayerState;
        FVector Location;
        FVector2D ScreenLocation;
        std::string SteamID;
        std::string RenderName;
        UserType UserType;
        bool Owner;
        int Ping;
        float Sanity;
        float Distance;
        bool CanVisible;
    };

    bool Running;
    CRITICAL_SECTION Entity_CriticalSection;
    std::vector<EntityInfo> EntityInfos;

    NameTags() : Module(xorstr_("NameTags"))
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

    struct RenderSegment
    {
        enum Type { Text, Icon } type;

        std::string text;
        ImU32 color;

        float width = 0.0f;
        float height = 0.0f;

        float iconW = 0.0f;
        float iconH = 0.0f;
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
    };


    void OnRender2D(DX12DrawContext context) override
    {
        auto PC = Instances::GetPlayerCharacter();
        auto Controller = Instances::GetPlayerController();
        if (!Controller || !Running) return;

        std::vector<EntityInfo> entityInfos = GetEntityInfos();
        ImDrawList* drawList = context.BackGroundDrawList;
        float finalScale = scale->value;

        ImFont* font = fontMode->index == 0 ? FontManager::Get().GetSFBoldFont() : FontManager::Get().GetMainFont();

        for (const auto& entityInfo : entityInfos)
        {
            if (!entityInfo.CanVisible) continue;
            std::vector<RenderSegment> segments;
            
            FVector2D screenPos = entityInfo.ScreenLocation;
            
            if (entityInfo.EntityType == Player)
            {
                ABPCharacter_Demo_C* player = entityInfo.Player;

                if (!player || (player == PC && !self->value)) continue;
                
                auto StateBP = entityInfo.PlayerState;

                if (StateBP)
                {
                    if (static_cast<int>(entityInfo.UserType) > 0)
                    {
                        auto logoPtr = Archon::GetArchonGUI()->GetLogoTexturePtr();

                        if (logoPtr)
                        {
                            RenderSegment iconSeg;
                            iconSeg.type = RenderSegment::Icon;

                            float baseW = 45.0f;
                            float baseH = 35.0f;

                            iconSeg.iconW = baseW * 0.02f;
                            iconSeg.iconH = baseH * 0.02f;
                            iconSeg.textureHandle = logoPtr->gpuTextureHandle;

                            segments.push_back(iconSeg);
                        }
                    }


                    if (ping->value)
                    {
                        int p = entityInfo.Ping;
                        RenderSegment seg;
                        seg.type = RenderSegment::Text;
                        seg.text = std::to_string(p) + xorstr_("ms ");
                        seg.color = GetPingColor(p);
                        segments.push_back(seg);
                    }

                    std::string steamID = entityInfo.SteamID;
                    UserType userType = entityInfo.UserType;
                    
                    RenderSegment nameSeg;
                    nameSeg.type = RenderSegment::Text;
                    nameSeg.text = entityInfo.RenderName;


                    if (userType == UserType::Admin || userType == UserType::Alpha)
                        nameSeg.color = RGBA(255, 0, 0, 255).ToU32();
                    else if (userType == UserType::Ban)
                        nameSeg.color = RGBA(10, 10, 10, 255).ToU32();
                    else if (entityInfo.Owner)
                        nameSeg.color = ownerNameColor->GetU32();
                    else
                        nameSeg.color = playerNameColor->GetU32();

                    segments.push_back(nameSeg);

                    if (this->steamID->value)
                    {
                        if (steamID.empty()) steamID = xorstr_("-1");

                        RenderSegment idSeg;
                        idSeg.type = RenderSegment::Text;
                        idSeg.text = steamID + " ";
                        idSeg.color = steamIDColor->GetU32();
                        segments.push_back(idSeg);
                    }


                    if (sanity->value)
                    {
                        float sanityVal = entityInfo.Sanity;
                        RenderSegment sanSeg;
                        sanSeg.type = RenderSegment::Text;
                        sanSeg.text = std::format(("{:.1f}"), sanityVal) + " ";
                        sanSeg.color = GetSanityColor(sanityVal);
                        segments.push_back(sanSeg);
                    }
                }
                else
                {
                    RenderSegment seg;
                    seg.type = RenderSegment::Text;
                    seg.text = xorstr_("[数据删除] ");
                    seg.color = IM_COL32_WHITE;
                    segments.push_back(seg);
                }
            } else
            {
                RenderSegment nameSeg;
                nameSeg.type = RenderSegment::Text;
                nameSeg.text = entityInfo.RenderName;
                nameSeg.color = monsterNameColor->GetU32();

                segments.push_back(nameSeg);
            }

            if (distance->value)
            {
                float Distance = entityInfo.Distance;
                float distMeters = Distance / 100.0f;

                RenderSegment distSeg;
                distSeg.type = RenderSegment::Text;
                distSeg.text = std::format(("{:.1f}m"), distMeters);
                distSeg.color = RGBA(255, 255, 255, 255).ToU32();
                segments.push_back(distSeg);
            }
            float iconPadding = 0.3f;

            float totalWidth = 0.0f;
            float totalHeight = 0.0f;

            for (auto& seg : segments)
            {
                if (seg.type == RenderSegment::Text)
                {
                    ImVec2 sz = ImGui::CalcTextSize3(seg.text.c_str(), font, finalScale);
                    seg.width = sz.x;
                    seg.height = sz.y;
                }
                else if (seg.type == RenderSegment::Icon)
                {
                    seg.width = (seg.iconW * finalScale) + (iconPadding * finalScale);
                    seg.height = seg.iconH * finalScale;
                }

                totalWidth += seg.width;
                if (seg.height > totalHeight) totalHeight = seg.height;
            }

            if (totalHeight == 0) totalHeight = 15.0f * finalScale;

            float paddingX = this->paddingX->value;
            float paddingY = this->paddingY->value;

            ImVec2 pos(screenPos.X, screenPos.Y);
            float boxHeight = totalHeight + (paddingY * 2);
            float boxWidth = totalWidth + (paddingX * 2);
            
            ImVec2 rectMin(pos.x - (boxWidth / 2.0f), pos.y - boxHeight);
            ImVec2 rectMax(pos.x + (boxWidth / 2.0f), pos.y);

            float x = rectMin.x;
            float y = rectMin.y;
            float w = rectMax.x - rectMin.x;
            float h = rectMax.y - rectMin.y;

            RGBA4C R4C = RGBA4C(RGBA(glowColor->GetU32()));
            if (glowColorMode->index == 1)
            {
                RGBA c0 = Render2DUtil::GetColor(0);
                RGBA c90 = Render2DUtil::GetColor(90);
                RGBA c180 = Render2DUtil::GetColor(180);
                RGBA c270 = Render2DUtil::GetColor(270);
                R4C = RGBA4C(c270, c0, c180, c90);
            }

            if (blurBackGround->value)
            {
                Shader2DUtil::Get().OnDrawRoundBlur(context, x, y, w, h, rounding->value, RGBA(blurColor->GetU32()),
                                                    blurStrength->value, blurOpacity->value);
            }
            else
            {
                drawList->AddRectFilled(rectMin, rectMax, fillColor->GetU32(), rounding->value);
            }

            Shader2DUtil::Get().OnDrawRoundGlow(context, x, y, w, h, rounding->value, glowStrength->value,
                                                glowAlpha->value, R4C);


            float visualFixY = 0.025f * finalScale;
            float currentDrawX = rectMin.x + paddingX;

            float centerY = rectMin.y + paddingY + (totalHeight / 2.0f);

            for (const auto& seg : segments)
            {
                if (seg.type == RenderSegment::Text)
                {
                    float textY = rectMin.y + paddingY - visualFixY;

                    if (fontShadow->value)
                    {
                        drawList->AddText(font, finalScale, ImVec2(currentDrawX + 1, textY + 1), IM_COL32_BLACK,
                                          seg.text.c_str());
                    }
                    drawList->AddText(font, finalScale, ImVec2(currentDrawX, textY), seg.color, seg.text.c_str());
                }
                else if (seg.type == RenderSegment::Icon)
                {
                    float drawIconW = seg.width - (iconPadding * finalScale);
                    float drawIconH = seg.height;

                    float iconDrawY = centerY - (drawIconH / 2.0f);

                    RGBA4C iconColor = RGBA(28, 41, 85);

                    if (static_cast<int>(entityInfo.UserType) > 3)
                    {
                        RGBA c0 = Render2DUtil::GetColor(0);
                        RGBA c90 = Render2DUtil::GetColor(90);
                        RGBA c180 = Render2DUtil::GetColor(180);
                        RGBA c270 = Render2DUtil::GetColor(270);

                        iconColor = RGBA4C(c270, c0, c180, c90);
                    }

                    Shader2DUtil::Get().OnDrawStencilImage(
                        context,
                        currentDrawX, iconDrawY,
                        drawIconW, drawIconH,
                        iconColor,
                        seg.textureHandle
                    );
                }

                currentDrawX += seg.width;
            }
        }
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
        
        GPStatics->GetAllActorsOfClass(GWorld, ABPCharacter_Demo_C::StaticClass(), &actors);

        for (int32 i = 0; i < actors.Num(); i++)
        {
            AActor* actor = actors[i];

            if (!actor || !actor->Class) continue;

            auto player = static_cast<ABPCharacter_Demo_C*>(actor);

            if (!player) continue;

            FVector worldPos = player->K2_GetActorLocation();
            
            UCapsuleComponent* Cap = player->CapsuleComponent;
            
            float height = Cap ? Cap->GetScaledCapsuleHalfHeight() : 0.0f;
          
            EntityInfo info;
          
            info.EntityType = Player;
            info.Player = player;
            info.PlayerState = player->PlayerState ? static_cast<AMP_PS_C*>(player->PlayerState) : nullptr;
            info.Location = worldPos;
            info.SteamID = info.PlayerState ? Instances::GetSteamID(player) : "";
            info.Owner = Instances::GetLevelData()->HostSteamID == info.SteamID;
            info.UserType = info.SteamID.empty() ? UserType::None : Auth::GetArchonUserType(info.SteamID);
            
            std::string namePrefix = "";
            switch (info.UserType)
            {
            case UserType::Ban: namePrefix = xorstr_("[Ban] ");
                break;
            case UserType::Beta: namePrefix = xorstr_("[Beta] ");
                break;
            case UserType::Alpha: namePrefix = xorstr_("[Alpha] ");
                break;
            case UserType::Admin: namePrefix = xorstr_("[Admin] ");
                break;
            }
            std::string nameStr = info.PlayerState ? Utils::ReadWideStringFromAddress(info.PlayerState->PlayerNamePrivate, 64) : xorstr_("[数据删除]");
            
            info.RenderName = namePrefix + nameStr + " ";
            
            info.Ping = info.PlayerState ? info.PlayerState->Ping * 4 : -1;
            info.Sanity = info.PlayerState ? info.PlayerState->Sanity : 0;
            info.Distance = PlayerLocation.GetDistanceTo(worldPos);
            FVector HeadLocation = { worldPos.X, worldPos.Y, worldPos.Z + height + playerOffsetY->value };
            FVector2D screenPos;
            info.CanVisible = PlayerController->ProjectWorldLocationToScreen(HeadLocation, &screenPos, true);
            info.ScreenLocation = screenPos;

            EntityInfos.push_back(info);
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

            UCapsuleComponent* Cap = monster->CapsuleComponent;
            
            float height = Cap ? Cap->GetScaledCapsuleHalfHeight() : 0.0f;

            EntityInfo info;
            info.EntityType = Monster;
            info.Monster = monster;
            info.RenderName = Utils::CleanClassName(monster->Class->GetName()) + " ";
            info.Location = worldPos;
            info.Distance = PlayerLocation.GetDistanceTo(worldPos);
            FVector2D screenPos;
            FVector HeadLocation = { worldPos.X, worldPos.Y, worldPos.Z + height + monsterOffsetY->value};
            info.CanVisible = PlayerController->ProjectWorldLocationToScreen(HeadLocation, &screenPos, true);
            info.ScreenLocation = screenPos;

            EntityInfos.push_back(info);
        }
        
        return EntityInfos;
    }

    ImU32 GetPingColor(int p)
    {
        if (p >= 150)
        {
            return RGBA(255, 50, 50, 255).ToU32();
        }

        if (p >= 70)
        {
            return RGBA(255, 255, 0, 255).ToU32();
        }

        return RGBA(0, 255, 0, 255).ToU32();
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
};
