#include "pch.h"
#include "NotificationManager.h"

#include "../../Archon.h"

void NotificationManager::AddModuleToggle(Module* module)
{
    Archon::GetDynamicIslandManager()->AddModuleToggle(module);
}

void NotificationManager::AddNotification(NotificationType type, std::string title, std::string description, long long time)
{
    Archon::GetDynamicIslandManager()->AddNotification(type, title, description, time);
}

