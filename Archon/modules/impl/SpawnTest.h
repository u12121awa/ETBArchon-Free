#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"
#include "../../Utils/World/WorldUtils.h"
#include "../../Utils/World/FTickCallbackSystem.h"

class SpawnTest : public Module
{
public:
    struct GameItemInfo
    {
        const char* id;
        const char* name_en;
        const wchar_t* name_cn;
    };

    const std::vector<GameItemInfo>& GetItemDatabase()
    {
        static std::vector<GameItemInfo> items;

        if (items.empty())
        {
            items = {
                {xorstr_("BP_DroppedItem_Rope_C"), xorstr_("Rope"), xorstr_(L"绳索")},
                {xorstr_("BP_DroppedItem_AlmondWater_C"), xorstr_("Almond Water"), xorstr_(L"杏仁水")},
                {xorstr_("BP_DroppedItem_AlmondBottle_C"),xorstr_("Almond Bottle"), xorstr_(L"杏仁瓶")},
                {xorstr_("BP_DroppedItem_BugSpray_C"), xorstr_("Bug Spray"), xorstr_(L"杀虫剂")},
                {xorstr_("BP_DroppedItem_Crowbar_C"), xorstr_("Crowbar"), xorstr_(L"撬棍")},
                {xorstr_("BP_DroppedItem_EnergyBar_C"), xorstr_("Energy Bar"), xorstr_(L"巧克力/能量棒")},
                {xorstr_("BP_DroppedItem_Flashlight_C"), xorstr_("Flashlight"), xorstr_(L"手电筒")},
                {xorstr_("BP_DroppedItem_Jelly_C"), xorstr_("Moth Jelly"), xorstr_(L"飞蛾果冻")},
                {xorstr_("BP_DroppedItem_Juice_C"), xorstr_("Juice"), xorstr_(L"果汁")},
                {xorstr_("BP_DroppedItem_LiDAR_C"), xorstr_("LiDAR Scanner"), xorstr_(L"激光雷达")},
                {xorstr_("BP_DroppedItem_LiquidPain_C"), xorstr_("Liquid Pain"), xorstr_(L"液态痛苦")},
                {xorstr_("BP_DroppedItem_Thermometer_C"), xorstr_("Thermometer"), xorstr_(L"飞蛾探测仪")},
                {xorstr_("BP_DroppedItem_Chainsaw_C"), xorstr_("Chainsaw"), xorstr_(L"电锯")},
                {xorstr_("BP_DroppedItem_DivingHelmet_C"),xorstr_("Diving Helmet"), xorstr_(L"潜水头盔")},
                {xorstr_("BP_DroppedItem_FlareGun_C"), xorstr_("Flare Gun"), xorstr_(L"信号枪")},
                {xorstr_("BP_DroppedItem_WalkieTalkie_C"),xorstr_("Walkie Talkie"), xorstr_(L"对讲机")},
                {xorstr_("BP_DroppedItem_Firework_C"), xorstr_("Firework"), xorstr_(L"鞭炮")},
                {xorstr_("BP_DroppedItem_Glowstick_C"), xorstr_("Glowstick (G)"), xorstr_(L"荧光棒 (绿)")},
                {xorstr_("BP_DroppedItem_Glowstick_Blue_C"),xorstr_("Glowstick (B)"),xorstr_(L"荧光棒 (蓝)")},
                {xorstr_("BP_DroppedItem_Glowstick_Red_C"), xorstr_("Glowstick (R)"),xorstr_(L"荧光棒 (红)")},
                {xorstr_("BP_DroppedItem_Glowstick_Yellow_C"),xorstr_("Glowstick (Y)"),xorstr_(L"荧光棒 (黄)")},
                {xorstr_("BP_DroppedItem_Ticket_C"), xorstr_("Ticket"), xorstr_(L"票")},
                {xorstr_("BP_DroppedItem_Camera_C"), xorstr_("Video Camera"), xorstr_(L"摄像机")},
                {xorstr_("BP_DroppedItem_Toy_C"), xorstr_("Toy"), xorstr_(L"玩具")},
                {xorstr_("BP_DroppedItem_Knife_C"), xorstr_("Knife"), xorstr_(L"刀")}
            };
        }
        return items;
    }

    template <typename T>
    UClass* GetItemDropped()
    {
        return nullptr;
    }

    template <>
    UClass* GetItemDropped<ABP_Rope_C>()
    {
        return ABP_DroppedItem_Rope_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_AlmondWater_C>()
    {
        return ABP_DroppedItem_AlmondWater_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_AlmondBottle_C>()
    {
        return ABP_DroppedItem_AlmondBottle_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_BugSpray_C>()
    {
        return ABP_DroppedItem_BugSpray_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Crowbar_C>()
    {
        return ABP_DroppedItem_Crowbar_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_EnergyBar_C>()
    {
        return ABP_DroppedItem_EnergyBar_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Flashlight_C>()
    {
        return ABP_DroppedItem_Flashlight_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_MothJelly_C>()
    {
        return ABP_DroppedItem_Jelly_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Juice_C>()
    {
        return ABP_DroppedItem_Juice_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Scanner_C>()
    {
        return ABP_DroppedItem_LiDAR_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Liquid_Pain_C>()
    {
        return ABP_DroppedItem_LiquidPain_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Thermometer_C>()
    {
        return ABP_DroppedItem_Thermometer_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Chainsaw_C>()
    {
        return ABP_DroppedItem_Chainsaw_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Diving_Helmet_C>()
    {
        return ABP_DroppedItem_DivingHelmet_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_FlareGun_C>()
    {
        return ABP_DroppedItem_FlareGun_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_WalkieTalkie_C>()
    {
        return ABP_DroppedItem_WalkieTalkie_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Firework_C>()
    {
        return ABP_DroppedItem_Firework_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Glowstick_C>()
    {
        return ABP_DroppedItem_Glowstick_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Glowstick_Blue_C>()
    {
        return ABP_DroppedItem_Glowstick_Blue_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Glowstick_Red_C>()
    {
        return ABP_DroppedItem_Glowstick_Red_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Glowstick_Yellow_C>()
    {
        return ABP_DroppedItem_Glowstick_Yellow_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Ticket_C>()
    {
        return ABP_DroppedItem_Ticket_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Camera_C>()
    {
        return ABP_DroppedItem_Camera_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Toy_C>()
    {
        return ABP_DroppedItem_Toy_C::StaticClass();
    }

    template <>
    UClass* GetItemDropped<ABP_Item_Knife_C>()
    {
        return ABP_DroppedItem_Knife_C::StaticClass();
    }

    ModeSetting* item = New<>(ModeSetting(xorstr_("Item"), 0, [this]{std::vector<std::string> temp; for (const auto& i : GetItemDatabase()) temp.push_back(Utils::WstringToUtf8(i.name_cn)); return temp;}()));
    NumberSetting* count = New<>(NumberSetting("Count", 600, 100, 1000));
    NumberSetting* radius = New<>(NumberSetting("Radius", 70, 5, 200));
    NumberSetting* scaleX = New<>(NumberSetting("ScaleX", 1, 0.1, 2));
    NumberSetting* scaleY = New<>(NumberSetting("ScaleY", 1, 0.1, 2));
    NumberSetting* scaleZ = New<>(NumberSetting("ScaleZ", 1, 0.1, 2));
    
   
    SpawnTest() : Module("SpawnTest")
    {
    }

    template <typename T>
    void OnSpawn()
    {
        auto World = Instances::GetWorld();
        if (!World) return;


        APawn* LocalPawn = Instances::GetLocalPawn();
        if (!LocalPawn) return;

        FVector Center = LocalPawn->GetTransform().Translation;
        auto Rot = LocalPawn->GetTransform().Rotation;
        
        UClass* ActorClass = GetItemDropped<T>();
        
        if (!ActorClass) return;

        int Count = (int) count->value;
        float Radius = radius->value;

        for (int i = 0; i < Count; i++)
        {
            float Theta = acosf(1 - 2 * (i + 0.5f) / Count);
            float Phi = sqrtf(Count * M_PI) * Theta;


            float x = Radius * sinf(Theta) * cosf(Phi);
            float y = Radius * sinf(Theta) * sinf(Phi);
            float z = Radius * cosf(Theta);

            FVector SpawnPos = Center + FVector(x, y, z);

            FTransform transform{};
            transform.Rotation = Rot;
            transform.Translation = SpawnPos;
            transform.Scale3D = FVector(scaleX->value, scaleY->value, scaleZ->value);

            AActor* NewActor = UGameplayStatics::BeginSpawningActorFromClass(
                World, ActorClass, transform, false, nullptr);

            if (NewActor)
            {
                UGameplayStatics::FinishSpawningActor(NewActor, transform);
            }
        }
    }

    void ItemSpawn(int selectedItemIndex)
    {
        
        switch (selectedItemIndex)
        {
        case 0: OnSpawn<ABP_Rope_C>();
            break;

        case 1: OnSpawn<ABP_Item_AlmondWater_C>();
            break;

        case 2: OnSpawn<ABP_Item_AlmondBottle_C>();
            break;

        case 3: OnSpawn<ABP_Item_BugSpray_C>();
            break;

        case 4: OnSpawn<ABP_Item_Crowbar_C>();
            break;

        case 5: OnSpawn<ABP_EnergyBar_C>();
            break;

        case 6: OnSpawn<ABP_Item_Flashlight_C>();
            break;

        case 7: OnSpawn<ABP_MothJelly_C>();
            break;

        case 8: OnSpawn<ABP_Juice_C>();
            break;

        case 9: OnSpawn<ABP_Scanner_C>();
            break;

        case 10: OnSpawn<ABP_Liquid_Pain_C>();
            break;

        case 11: OnSpawn<ABP_Thermometer_C>();
            break;

        case 12: OnSpawn<ABP_Item_Chainsaw_C>();
            break;

        case 13: OnSpawn<ABP_Diving_Helmet_C>();
            break;

        case 14: OnSpawn<ABP_FlareGun_C>();
            break;

        case 15: OnSpawn<ABP_WalkieTalkie_C>();
            break;

        case 16: OnSpawn<ABP_Item_Firework_C>();
            break;

        case 17: OnSpawn<ABP_Item_Glowstick_C>();
            break;

        case 18: OnSpawn<ABP_Item_Glowstick_Blue_C>();
            break;

        case 19: OnSpawn<ABP_Item_Glowstick_Red_C>();
            break;

        case 20: OnSpawn<ABP_Item_Glowstick_Yellow_C>();
            break;

        case 21: OnSpawn<ABP_Item_Ticket_C>();
            break;

        case 22: OnSpawn<ABP_Item_Camera_C>();
            break;

        case 23: OnSpawn<ABP_Item_Toy_C>();
            break;

        case 24: OnSpawn<ABP_Item_Knife_C>();
            break;
        }
    }
    
    void OnEnable() override
    {
        this->Disable();
        FTickCallbackSystem::RegisterCallback([this]
        {
            ItemSpawn(item->index);
        });
    }
};
