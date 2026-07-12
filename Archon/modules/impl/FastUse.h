#pragma once
#include "../Module.h"

class FastUse : public Module
{
public:
    BooleanSetting* firework = New<>(BooleanSetting(xorstr_("Firework"), true));
    BooleanSetting* flareGun = New<>(BooleanSetting(xorstr_("FlareGun"), true));
    
    
    FastUse() : Module(xorstr_("FastUse"))
    {
    }
    void FastUse_RapidFire(ABPCharacter_Demo_C* Player)
    {
        if (!Player || !Player->CurrentItem_Rep) return;
        
        Player->UseItem_SERVER(Player->CurrentItem_Rep);
        
        int32 OriginalSlot = Player->CurrentEquipSlot;
        
        int32 TempSlot = (OriginalSlot == 0) ? 1 : 0;
        
        Player->UnequipItem();
        
        Player->CurrentEquipSlot = TempSlot; 
        Player->Equip();
        
        Player->CurrentEquipSlot = OriginalSlot;
        Player->Equip();
    }
    
    void OnUpdate() override
    {
        auto PC = Instances::GetPlayerCharacter();
        if (!PC) return;

        ABP_Item_C* CurrentItem = PC->CurrentItem_Rep;
        
        if (!CurrentItem) return;

        if (firework->value && CurrentItem->IsA(ABP_Item_Firework_C::StaticClass()))
        {
            auto* FireworkItem = static_cast<ABP_Item_Firework_C*>(CurrentItem);
            FireworkItem->ThrowTime = 0.0f;
            FireworkItem->ExplodeTimer->TheTimeline.Length = 0.0f;
            FireworkItem->CanUse = true;
        }

        if (flareGun->value )
        {
            FastUse_RapidFire(PC);
        }

        
    }
};
