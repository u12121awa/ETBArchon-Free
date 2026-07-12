#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"
#include "../../Utils/World/TimerHandler.h"

class ItemSpawner : public Module
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

    ModeSetting* item = New<>(ModeSetting(xorstr_("Item"), 0, [this]{std::vector<std::string> temp;for (const auto& i : GetItemDatabase()) temp.push_back(Utils::WstringToUtf8(i.name_cn));return temp;}()));
    
    ModeSetting* spawnMode = New<>(ModeSetting(xorstr_("SpawnMode"), 0, std::vector<std::string>{xorstr_("Normal"), xorstr_("Drop"), xorstr_("InvAdd"),xorstr_("Equip")}));
    NumberSetting* spawnCount = New<>(NumberSetting(xorstr_("SpawnCount"), 1, 1, 100, xorstr_("%.0f"), [this] { return spawnMode->index != 3; }));
    BooleanSetting* autoDisabled = New<>(BooleanSetting(xorstr_("AutoDisabled"), true));
    NumberSetting* delay = New<>(NumberSetting(xorstr_("SpawnDelay"), 500, 0, 5000, xorstr_("%.0f"), [this] { return !autoDisabled->value; }));

    ItemSpawner() : Module(xorstr_("ItemSpawner"))
    {
    }

    TimerHandler timer;

    template <typename T>
    void SpawnItemByType(ABPCharacter_Demo_C* PlayerCharacter, int count)
    {
        UWorld* World = Instances::GetWorld();

        if (!PlayerCharacter) return;

        UClass* ItemClass = T::StaticClass();

        if (!ItemClass) return;


        T* DefaultItem = static_cast<T*>(ItemClass->DefaultObject);

        if (!DefaultItem)
        {
            return;
        }

        FName ItemID = DefaultItem->ID;

        switch (spawnMode->index)
        {
        case 0:
            {
                UClass* DroppedItemClass = GetItemDropped<T>();
                if (!DroppedItemClass) return;

                for (int i = 0; i < count; ++i)
                {
                    FTransform transform = PlayerCharacter->GetTransform();

                    AActor* NewActor = UGameplayStatics::BeginSpawningActorFromClass(
                        World, DroppedItemClass, transform, false, nullptr);

                    if (NewActor)
                    {
                        UGameplayStatics::FinishSpawningActor(NewActor, transform);
                    }
                }
                break;
            }

        case 1:
            {
                for (int i = 0; i < count; ++i)
                {
                    PlayerCharacter->DropItem_SERVER(ItemID);
                }
                break;
            }

        case 2:
            {
                for (int i = 0; i < count; ++i)
                {
                    auto playerState = PlayerCharacter->PlayerState;
                    if (!playerState) return;

                    auto Inventory = static_cast<AMP_PS_C*>(playerState)->Items_Rep;
                    bool Full = true;

                    for (size_t i = 0; i < Inventory.Num(); i++)
                    {
                        if (Inventory[i].ComparisonIndex == 0)
                        {
                            Full = false;
                            Inventory[i] = ItemID;
                            break;
                        }
                    }
                    if (Full) Inventory[Inventory.Num() - 1] = ItemID;
                }
                break;
            }

        case 3:
            {
                PlayerCharacter->SpawnEquipItem_SERVER(ItemClass);
            }
        }
    }

    void ItemSpawn(int selectedItemIndex, int spawnCount)
    {
        ABPCharacter_Demo_C* PlayerCharacter = Instances::GetPlayerCharacter();


        if (!PlayerCharacter)
        {
            return;
        }
        switch (selectedItemIndex)
        {
        case 0: SpawnItemByType<ABP_Rope_C>(PlayerCharacter, spawnCount);
            break;

        case 1: SpawnItemByType<ABP_Item_AlmondWater_C>(PlayerCharacter, spawnCount);
            break;

        case 2: SpawnItemByType<ABP_Item_AlmondBottle_C>(PlayerCharacter, spawnCount);
            break;

        case 3: SpawnItemByType<ABP_Item_BugSpray_C>(PlayerCharacter, spawnCount);
            break;

        case 4: SpawnItemByType<ABP_Item_Crowbar_C>(PlayerCharacter, spawnCount);
            break;

        case 5: SpawnItemByType<ABP_EnergyBar_C>(PlayerCharacter, spawnCount);
            break;

        case 6: SpawnItemByType<ABP_Item_Flashlight_C>(PlayerCharacter, spawnCount);
            break;

        case 7: SpawnItemByType<ABP_MothJelly_C>(PlayerCharacter, spawnCount);
            break;

        case 8: SpawnItemByType<ABP_Juice_C>(PlayerCharacter, spawnCount);
            break;

        case 9: SpawnItemByType<ABP_Scanner_C>(PlayerCharacter, spawnCount);
            break;

        case 10: SpawnItemByType<ABP_Liquid_Pain_C>(PlayerCharacter, spawnCount);
            break;

        case 11: SpawnItemByType<ABP_Thermometer_C>(PlayerCharacter, spawnCount);
            break;

        case 12: SpawnItemByType<ABP_Item_Chainsaw_C>(PlayerCharacter, spawnCount);
            break;

        case 13: SpawnItemByType<ABP_Diving_Helmet_C>(PlayerCharacter, spawnCount);
            break;

        case 14: SpawnItemByType<ABP_FlareGun_C>(PlayerCharacter, spawnCount);
            break;

        case 15: SpawnItemByType<ABP_WalkieTalkie_C>(PlayerCharacter, spawnCount);
            break;

        case 16: SpawnItemByType<ABP_Item_Firework_C>(PlayerCharacter, spawnCount);
            break;

        case 17: SpawnItemByType<ABP_Item_Glowstick_C>(PlayerCharacter, spawnCount);
            break;

        case 18: SpawnItemByType<ABP_Item_Glowstick_Blue_C>(PlayerCharacter, spawnCount);
            break;

        case 19: SpawnItemByType<ABP_Item_Glowstick_Red_C>(PlayerCharacter, spawnCount);
            break;

        case 20: SpawnItemByType<ABP_Item_Glowstick_Yellow_C>(PlayerCharacter, spawnCount);
            break;

        case 21: SpawnItemByType<ABP_Item_Ticket_C>(PlayerCharacter, spawnCount);
            break;

        case 22: SpawnItemByType<ABP_Item_Camera_C>(PlayerCharacter, spawnCount);
            break;

        case 23: SpawnItemByType<ABP_Item_Toy_C>(PlayerCharacter, spawnCount);
            break;

        case 24: SpawnItemByType<ABP_Item_Knife_C>(PlayerCharacter, spawnCount);
            break;
        }
    }

    void OnUpdate() override
    {
        if (timer.passedMs((int)delay->value) || autoDisabled->value)
        {
            ItemSpawn(item->index, (int)spawnCount->value);
            timer.reset();
            if (autoDisabled->value)
            {
                this->Disable();
            }
        }
    }
};
