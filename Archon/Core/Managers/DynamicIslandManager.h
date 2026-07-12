#pragma once

#include <vector>
#include <string>
#include <functional>
#include <algorithm>

#include "../../modules/Module.h"
#include "../../modules/impl/DynamicIsland.h"
#include "../../Utils/Render/Animation.h"
#include "../../Utils/World/TimerHandler.h"


enum class NotificationType {
    SUCCESS, 
    ERR,
    WARN,
    INFO, 
    MODULE
};

namespace PriorityUtil {
    const int TOP = 100;
    const int MEDIUM = 50;
    const int LOW = 10;
}

struct TimedContent {
    NotificationType type;
    Module* module = nullptr;
    std::string title;
    std::string description;
    
    Animation animation;
    
    TimerHandler timer;
    long long time;
    
    TimedContent(NotificationType type, std::string title, std::string description, long long time, long duration)
         : type(type), title(title), description(description), time(time),
           animation(Easing::EASE_OUT_QUINT, duration) 
    {
        timer.reset();
    }
    
    TimedContent(Module* module, long duration);
};

struct Content {
    enum class Type { Tab, Normal };
    
    std::function<void(ImDrawList*, float, float)> renderFunc;
    float width;
    float height;
    int weight;
    Type type;

    Content(std::function<void(ImDrawList*, float, float)> func, float w, float h, int weight, Type type = Type::Normal)
        : renderFunc(func), width(w), height(h), weight(weight), type(type) {}
};

class DynamicIslandManager {
public:
    DynamicIslandManager();
    
    void OnRender2D(DX12DrawContext context);
    void AddNotification(NotificationType type, std::string title, std::string description, long long time);
    void AddModuleToggle(Module* module);

private:
    std::shared_ptr<DynamicIsland> config = nullptr; 
    
    std::vector<TimedContent> timedContents;
    std::vector<Content> contents;
    
    float renderX = -1.0f;
    float renderY = 40.0f;
    float currentWidth = -1.0f;
    float currentHeight = -1.0f;
    float radius = 0.0f;
    
    float smooth(float current, float target);
    float getMaxWidth();
    float getTotalHeight();
    ImU32 getContentColor(NotificationType type);
    float GetFontHeight();
    float GetTextWidth(const std::string& text);
    void renderTimedContent(ImDrawList* drawList, float x, float y, TimedContent& content);
    void renderModuleIcon(ImDrawList* drawList, float x, float y, TimedContent& content);
    void renderNotificationIcon(ImDrawList* drawList, float x, float y, TimedContent& content);
};