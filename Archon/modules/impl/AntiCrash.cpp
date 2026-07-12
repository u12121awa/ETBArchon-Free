#include "pch.h"
#include "AntiCrash.h"

#include "../../Core/Managers/DynamicIslandManager.h"
#include "../../Utils/Utils.h"

std::vector<UCostume*> Costumes;

void AntiCrash::OnFunctionEvent(FunctionEvent* event)
{
    VMProtectBeginMutation("AntiCrash::OnFunctionEvent");

    if (nullFunction->value && (!event->context || !Utils::ShouldUsePointer64(event->context) || !event->function || !Utils::ShouldUsePointer64(event->function)))
    {
        Log::Error(xorstr_("Function is Null!"));
        NotificationManager::AddNotification(NotificationType::ERR, xorstr_("Crash Cancel"), xorstr_("Function is Null!"), 5000);
       
        event->cancel = true;
        VMProtectEnd();
        return;
    }

    
    if (dismembered->value && event->functionName == xorstr_("Function BP_Dismembered.BP_Dismembered_C.ReceiveBeginPlay"))
    {
        ABP_Dismembered_C* context = static_cast<ABP_Dismembered_C*>(event->context);

        if (!context || !context->Costume)
        {
            if (Costumes.empty())
            {
                Costumes = Instances::GetAllInstancesOf<UCostume>();
            }
            
            Log::Error(xorstr_("Dismembered / Costume is null!"));
            NotificationManager::AddNotification(NotificationType::ERR, xorstr_("Crash Cancel"), xorstr_("Dismembered Crash!"), 5000);
            if (Costumes.empty())
            {
                event->cancel = true;
            }
            else
            {
                context->Costume = Costumes[0];
            }
            VMProtectEnd();
            return;
        }
    }

    if (ragdoll->value && event->functionName == xorstr_("Function BP_Ragdoll.BP_Ragdoll_C.ReceiveBeginPlay"))
    {
        ABP_Ragdoll_C* context = static_cast<ABP_Ragdoll_C*>(event->context);

        if (!context || !context->Costume)
        {
            if (Costumes.empty())
            {
                Costumes = Instances::GetAllInstancesOf<UCostume>();
            }
            Log::Error(xorstr_("Ragdoll / Costume is null!"));
            NotificationManager::AddNotification(NotificationType::ERR, xorstr_("Crash Cancel"), xorstr_("Ragdoll Crash!"), 5000);
            if (Costumes.empty())
            {
                event->cancel = true;
            }
            else
            {
                context->Costume = Costumes[0];
            }
            VMProtectEnd();
            return;
        }
    }
    
    if (rowBoat->value && event->functionName == xorstr_("Function BP_RowBoat.BP_RowBoat_C.ReceiveUnpossessed"))
    {
        Log::Error(xorstr_("Cancel BP_RowBoat_C.ReceiveUnpossessed Crash"));
        NotificationManager::AddNotification(NotificationType::ERR, "Crash Cancel", xorstr_("BP_RowBoat_C.ReceiveUnpossessed"), 5000);
        event->cancel = true;
        VMProtectEnd();
        return;
    }

    VMProtectEnd();
}
