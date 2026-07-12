#pragma once
#include "../Module.h"
#include "../../Utils/World/TimerHandler.h"

class ItemCleaner : public Module
{
public:
    ModeSetting* clearMode = New<>(ModeSetting(xorstr_("ClearMode"), 0, std::vector<std::string>{xorstr_("All"), xorstr_("Radius")}));
    ModeSetting* clearType = New<>(ModeSetting(xorstr_("ClearType"), 0, std::vector<std::string>{xorstr_("Normal"), xorstr_("PickUp")}));

    NumberSetting* radius = New<>(NumberSetting(xorstr_("Radius"), 1000.0f, 1.0f, 10000.0f, xorstr_("%.0f"),
                                                [this] { return clearMode->index == 1; }));
    BooleanSetting* autoDisabled = New<>(BooleanSetting(xorstr_("AutoDisabled"), true));
    NumberSetting* delay = New<>(NumberSetting(xorstr_("ClearDelay"), 1500, 0, 5000, xorstr_("%.0f"),
                                               [this] { return !autoDisabled->value; }));

    ItemCleaner() : Module(xorstr_("ItemCleaner"))
    {
    }
    
    TimerHandler timer;

    void ClearItems()
    {
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!PC || !Pawn) return;

        auto GPStatics = Instances::GetGPStatics();
        auto SystemLib = Instances::GetSystemLib();

        switch (clearMode->index)
        {
        case 0:
            {
                TArray<AActor*> Items;
                GPStatics->GetAllActorsOfClass(Instances::GetWorld(), ABP_DroppedItem_C::StaticClass(), &Items);

                if (Items.IsValid())
                {
                    for (size_t i = 0; i < Items.Num(); i++)
                    {
                        if (!Items[i])continue;

                        auto Item = (ABP_DroppedItem_C*)Items[i];

                        if (clearType->index == 0) Item->K2_DestroyActor();
                        else Pawn->PickUp_SERVER(Item);
                    }
                }
                break;
            }

        case 1:
            {
                FVector PlayerLoc = Pawn->K2_GetActorLocation();

                TArray<EObjectTypeQuery> ObjectTypes;
                ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery2); 
                ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery4);

                TArray<AActor*> IgnoredActors;
                IgnoredActors.Add(Pawn);

                TArray<AActor*> OutActors;

                bool bResult = UKismetSystemLibrary::SphereOverlapActors(
                    Pawn,
                    PlayerLoc,
                    radius->value,
                    ObjectTypes,
                    ABP_DroppedItem_C::StaticClass(),
                    IgnoredActors,
                    &OutActors
                );

                if (bResult)
                {
                    for (AActor* HitActor : OutActors)
                    {
                        ABP_DroppedItem_C* DroppedItem = static_cast<ABP_DroppedItem_C*>(HitActor);

                        if (DroppedItem)
                        {
                            if (clearType->index == 0) DroppedItem->K2_DestroyActor();
                            else Pawn->PickUp_SERVER(DroppedItem);
                        }
                    }
                }
                break;
            }
        }
    }

    void OnUpdate() override
    {
        if (timer.passedMs((int)delay->value) || autoDisabled->value)
        {
            ClearItems();
            timer.reset();
            if (autoDisabled->value) this->Disable();
        }
    }
};
