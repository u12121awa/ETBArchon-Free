#pragma once

class Module;
enum class NotificationType;

namespace NotificationManager
{
    void AddNotification(NotificationType type, std::string title, std::string description, long long time);
    void AddModuleToggle(Module* module);
};
