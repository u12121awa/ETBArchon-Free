#include "pch.h"
#include "DynamicIslandManager.h"
#include "FontManager.h"
#include <string>
#include <format>
#include <iso646.h>

#include "ModuleManager.h"
#include "../../Archon.h"
#include "../../Utils/Render/AnimateUtil.h"
#include "../../Utils/Render/Shader2DUtil.h"

TimedContent::TimedContent(Module* module, long duration) 
    : type(NotificationType::MODULE), module(module),
      animation(Easing::EASE_IN_OUT_QUINT, duration, module && module->IsEnabled() ? 0.0f : 1.0f)
{
    this->title = xorstr_("Module Toggled");
    bool enabled = module ? module->IsEnabled() : false; 
    
    std::string modName = module ? module->name : xorstr_("Unknown");
    this->description = modName + (enabled ? xorstr_(" has been Enabled") : xorstr_(" has been Disabled"));
    
    this->time = 2000;
    this->timer.reset();
}

DynamicIslandManager::DynamicIslandManager() {
    config = ModuleManager::Get().GetModule<DynamicIsland>();
}


float DynamicIslandManager::GetTextWidth(const std::string& text) {
    ImFont* font = FontManager::Get().GetMainFont();
    if (font) return font->CalcTextSizeA(config->fontSize->value, FLT_MAX, 0.0f, text.c_str()).x;
    return ImGui::CalcTextSize(text.c_str()).x;
}

float DynamicIslandManager::GetFontHeight() {
    ImFont* font = FontManager::Get().GetMainFont();
    if (font) return config->fontSize->value;
    return ImGui::GetTextLineHeightWithSpacing();
}

void DynamicIslandManager::OnRender2D(DX12DrawContext context) {
    if (!config) {
        config = ModuleManager::Get().GetModule<DynamicIsland>();
        return;
    }

    if (config->IsDisabled()) return;
    
    float SCALE = config->scale->value;
    float fontSize = config->fontSize->value;
    float logoSizeMultiplier = config->logoSize->value; 
    
    ImDrawList* dl = context.BackGroundDrawList;
    float screenWidth = context.ScreenWidth;
    float centerX = screenWidth * 0.5f;
    
    auto it = timedContents.begin();
    while (it != timedContents.end()) {
        TimedContent& tContent = *it;
        if (!tContent.timer.passedMs(tContent.time)) {
            float maxTextW = std::max(GetTextWidth(tContent.title), GetTextWidth(tContent.description));
            float requiredWidth = (50.0f * SCALE) + maxTextW;
            float requiredHeight = 40.0f * SCALE; 
            this->contents.emplace_back(
                [this, &tContent](ImDrawList* d, float x, float y) {
                    this->renderTimedContent(d, x, y, tContent);
                }, requiredWidth, requiredHeight, PriorityUtil::LOW
            );
            ++it;
        } else {
            it = timedContents.erase(it);
        }
    }
    
    if (!contents.empty()) {
        int maxWeight = -1;
        for (const auto& c : contents) if (c.weight > maxWeight) maxWeight = c.weight;
        contents.erase(std::remove_if(contents.begin(), contents.end(), 
            [maxWeight](const Content& c) { return c.weight < maxWeight; }), contents.end());
    }
    
    float fps = ImGui::GetIO().Framerate;
    std::string user = Archon::GetWorldManager()->GetPlayerName();
    std::string ping = std::to_string(Archon::GetWorldManager()->GetPing());
    
    std::string prefixText = xorstr_("Archon Free"); 
    std::string statsText = std::format(" | {} | {:.0f}fps | {}ms", user, fps, ping);
    
    float targetWidth, targetHeight;
    bool isDefault = contents.empty() && timedContents.empty();
    

    float currentIconW = 0.0f;
    float currentIconH = 0.0f;
    float iconSpacing = 6.0f * SCALE;
    
    if (isDefault) {
        float baseW = 45.0f;
        float baseH = 35.0f;
        
        currentIconW = baseW * logoSizeMultiplier;
        currentIconH = baseH * logoSizeMultiplier;
        
        float prefixW = GetTextWidth(prefixText);
        float statsW = GetTextWidth(statsText);
        
        targetWidth = (prefixW + statsW) + (16.0f * SCALE) + currentIconW + iconSpacing;
        
        float contentMaxHeight = std::max(GetFontHeight(), currentIconH);
        targetHeight = contentMaxHeight + (8.0f * SCALE); 

    } else {
        targetWidth = getMaxWidth();
        targetHeight = getTotalHeight();
    }
    
    float targetY = config->yOffset->value;
    float targetX = centerX - (currentWidth / 2.0f);
    
    if (renderX == -1.0f) { 
        renderX = targetX; renderY = targetY;
        currentWidth = targetWidth; currentHeight = targetHeight; 
    }
    
    renderX = smooth(renderX, centerX - (targetWidth / 2.0f));
    renderY = smooth(renderY, targetY); 
    currentWidth = smooth(currentWidth, targetWidth);
    currentHeight = smooth(currentHeight, targetHeight);
    
    float targetRadius = config->radius->value * SCALE;
    radius = smooth(radius, targetRadius);
    
    ImU32 bgColor = config->blurColor->GetU32();
    
    RGBA c0 = Render2DUtil::GetColor(0); 
    RGBA c90 = Render2DUtil::GetColor(90);
    RGBA c180 = Render2DUtil::GetColor(180); 
    RGBA c270 = Render2DUtil::GetColor(270);

    RGBA4C R4C = RGBA4C(RGBA(config->glowColor->GetU32()));
    
    if (config->glowColorMode->GetCurrent() == xorstr_("Sky")) {
        R4C = RGBA4C(c270, c0, c180, c90);
    }

    Shader2DUtil::Get().OnDrawRoundBlur(context, renderX, renderY, currentWidth, currentHeight, radius, RGBA(bgColor), config->blurStrength->value, config->blurOpacity->value);
    Shader2DUtil::Get().OnDrawRoundGlow(context, renderX, renderY, currentWidth, currentHeight, radius, config->glowStrength->value, config->glowAlpha->value, R4C);
    
    dl->PushClipRect(ImVec2(renderX, renderY), ImVec2(renderX + currentWidth, renderY + currentHeight), true);

    if (isDefault) {
        float paddingLeft = 8.0f * SCALE;
        
        float iconX = renderX + paddingLeft;
        float iconY = renderY + (currentHeight - currentIconH) / 2.0f;
        
        RGBA4C iconColor = RGBA4C(c270, c0, c180, c90);
        
        Shader2DUtil::Get().OnDrawStencilImage(
            context, 
            iconX, iconY, 
            currentIconW, currentIconH,
            iconColor, 
            Archon::GetArchonGUI()->GetLogoTexturePtr()->gpuTextureHandle
        );
        
       
        float textStartX = iconX + currentIconW + iconSpacing - 4.0f;
        float textY = renderY + (currentHeight - GetFontHeight()) / 2.0f;
        
        RGBA color1 = c0;
        
        RGBA color2 = c90;
        
        int startR = color1.IR(); 
        int startG = color1.IG(); 
        int startB = color1.IB();

        int endR = color2.IR(); 
        int endG = color2.IG(); 
        int endB = color2.IB();

        int totalLen = prefixText.length();
        float currentX = textStartX;
        
        for (int i = 0; i < totalLen; ++i) {
            std::string charStr(1, prefixText[i]);
            float charW = GetTextWidth(charStr);
            
            float ratio = (totalLen > 1) ? ((float)i / (float)(totalLen - 1)) : 0.0f;
            
            int r = startR + static_cast<int>((endR - startR) * ratio);
            int g = startG + static_cast<int>((endG - startG) * ratio);
            int b = startB + static_cast<int>((endB - startB) * ratio);
            
            ImU32 charColor = IM_COL32(r, g, b, 255);
            
            FontManager::Get().DrawTextW(dl, ImVec2(currentX, textY), charStr.c_str(), fontSize, charColor);
            
            currentX += charW;
        }
        
        FontManager::Get().DrawTextW(dl, ImVec2(currentX, textY), statsText.c_str(), fontSize, IM_COL32_WHITE);

    } else {
        float tempY = renderY;
        for (const auto& content : contents) {
            content.renderFunc(dl, renderX, tempY);
            tempY += content.height;
        }
        contents.clear();
    }

    dl->PopClipRect();
}

void DynamicIslandManager::renderTimedContent(ImDrawList* dl, float x, float y, TimedContent& content) {
    if (content.type == NotificationType::MODULE) {
        renderModuleIcon(dl, x, y, content);
    } else {
        renderNotificationIcon(dl, x, y, content);
    }
    float SCALE = config->scale->value;

    float fontSize = config->fontSize->value;
    
    float titleX = x + (38.0f * SCALE);
    float titleY = y + (9.0f * SCALE);
    float descX = x + (38.0f * SCALE);
    float descY = y + (24.0f * SCALE);

    FontManager::Get().DrawTextW(dl, ImVec2(titleX, titleY), content.title.c_str(), fontSize, IM_COL32_WHITE);
    FontManager::Get().DrawTextW(dl, ImVec2(descX, descY), content.description.c_str(), fontSize, IM_COL32(200, 200, 200, 255));
}

void DynamicIslandManager::renderModuleIcon(ImDrawList* dl, float x, float y, TimedContent& content) {
    bool enabled = content.module ? content.module->IsEnabled() : false;
    float targetAnim = enabled ? 1.0f : 0.0f;
    
    content.animation.run(targetAnim);
    
    float anim = content.animation.getValue();
    
    float SCALE = config->scale->value;
    
    float bgX = x + (6.0f * SCALE);
    float bgY = y + (13.0f * SCALE);
    float bgW = 26.0f * SCALE;
    float bgH = 14.0f * SCALE;
    dl->AddRectFilled(ImVec2(bgX, bgY), ImVec2(bgX + bgW, bgY + bgH), IM_COL32(64, 64, 64, 255), 6.5f * SCALE);
    
    int r = (int)(128 - 80 * anim);
    int g = (int)(128 - 11 * anim);
    int b = (int)(128 + 19 * anim);
    ImU32 sliderColor = IM_COL32(std::clamp(r,0,255), std::clamp(g,0,255), std::clamp(b,0,255), 255);

    float slX = x + (7.0f * SCALE);
    float slY = y + (14.0f * SCALE);
    float slW = 24.0f * SCALE;
    float slH = 12.0f * SCALE;
    dl->AddRectFilled(ImVec2(slX, slY), ImVec2(slX + slW, slY + slH), sliderColor, 6.0f * SCALE);
    
    float dotX = x + ((8.0f + 12.0f * anim) * SCALE);
    float dotY = y + (15.0f * SCALE);
    float dotSize = 10.0f * SCALE;
    dl->AddRectFilled(ImVec2(dotX, dotY), ImVec2(dotX + dotSize, dotY + dotSize), IM_COL32(64, 64, 64, 255), 4.5f * SCALE);
}

void DynamicIslandManager::renderNotificationIcon(ImDrawList* dl, float x, float y, TimedContent& content) {
    float SCALE = config->scale->value;

    float iconSize = config->iconSize->value;
    
    float bgX = x + (7.0f * SCALE);
    float bgY = y + (8.0f * SCALE);
    float size = 24.0f * SCALE;
    dl->AddRectFilled(ImVec2(bgX, bgY), ImVec2(bgX + size, bgY + size), IM_COL32(40, 40, 40, 255), 7.0f * SCALE);
    
    const char* iconChar = xorstr_("?");
    ImU32 color = getContentColor(content.type);

    switch (content.type) {
        case NotificationType::SUCCESS: iconChar = xorstr_("K"); break; 
        case NotificationType::ERR: iconChar = xorstr_("I"); break;
        case NotificationType::WARN: iconChar = xorstr_("L"); break;
        case NotificationType::INFO: iconChar = xorstr_("J"); break;
    }
    float fontX = bgX + 1.0f;
    float fontY = bgY + 3.5f;
    
    FontManager::Get().DrawIconInRect(dl, ImVec2(fontX, fontY), ImVec2(fontX + size, fontY + size), iconChar, iconSize, color);
}

float DynamicIslandManager::smooth(float current, float target) {
    float dt = ImGui::GetIO().DeltaTime;
    float fps = ImGui::GetIO().Framerate;
    float speed = config ? config->speed->value : 0.5f;
    auto mode = config ? (AnimMode)config->animation->index : AnimMode::Normal;

    return AnimateUtil::animate(current, target, speed, mode, dt, fps);
}

ImU32 DynamicIslandManager::getContentColor(NotificationType type) {
    switch (type) {
        case NotificationType::SUCCESS: return IM_COL32(95, 143, 80, 200);
        case NotificationType::ERR: return IM_COL32(255, 70, 70, 200);
        case NotificationType::WARN: return IM_COL32(143, 80, 80, 200);
        case NotificationType::INFO: return IM_COL32(48, 117, 147, 200);
        default: return IM_COL32(200, 200, 200, 200);
    }
}

float DynamicIslandManager::getMaxWidth() {
    float maxW = 0;
    for (const auto& c : contents) if (c.width > maxW) maxW = c.width;
    return maxW;
}

float DynamicIslandManager::getTotalHeight() {
    float h = 0;
    for (const auto& c : contents) h += c.height;
    return h;
}

void DynamicIslandManager::AddNotification(NotificationType type, std::string title, std::string description, long long time) {
    timedContents.emplace_back(type, title, description, time, config->duration->value);
}

void DynamicIslandManager::AddModuleToggle(Module* module) {
    bool canAdd = true;
    for (auto& tc : timedContents) {
        if (tc.module == module) {
            tc.time = 2000;
            tc.timer.reset();
            bool enabled = module->IsEnabled();
            tc.description = std::string(module->name) + (enabled ? xorstr_(" has been Enabled") : xorstr_(" has been Disabled"));
            canAdd = false;
            break;
        }
    }
    
    if (canAdd) {
        timedContents.emplace_back(module, config->moduleDuration->value);
    }
}
