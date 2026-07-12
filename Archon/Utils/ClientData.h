#pragma once

#include <string>
#include <memory>

struct PrimaryTheme
{
    std::string name;
    ImU32 backgroundColor;
    ImU32 secondaryBackgroundColor;
    ImU32 shadowColor;
    ImU32 brighterBackgroundColor;
    ImU32 fontColor;
};

struct ColorAnimation
{
    ImVec4 current;

    ColorAnimation() : current(0, 0, 0, 1)
    {
    }

    void Init(ImU32 color) { current = ImGui::ColorConvertU32ToFloat4(color); }

    void Update(ImU32 targetU32, float dt)
    {
        ImVec4 target = ImGui::ColorConvertU32ToFloat4(targetU32);
        float speed = 10.0f;
        current.x += (target.x - current.x) * dt * speed;
        current.y += (target.y - current.y) * dt * speed;
        current.z += (target.z - current.z) * dt * speed;
        current.w += (target.w - current.w) * dt * speed;
    }

    ImU32 GetU32() const { return ImGui::ColorConvertFloat4ToU32(current); }

    ImU32 GetU32WithAlpha(float alpha) const
    {
        ImVec4 tmp = current;
        tmp.w *= alpha;
        return ImGui::ColorConvertFloat4ToU32(tmp);
    }
};

struct FloatAnimation
{
    float value;

    FloatAnimation() : value(0.0f)
    {
    }

    void Init(float val) { value = val; }

    void Update(float target, float dt, float speed = 15.0f)
    {
        value += (target - value) * dt * speed;
    }
};

struct ScrollAnimation
{
    float current = 0.0f;
    float target = 0.0f;

    void Update(float dt)
    {
        float speed = 15.0f;
        current += (target - current) * dt * speed;

        if (std::abs(target - current) < 0.1f) current = target;
    }

    void HandleInput(float maxScroll, float scrollSpeed = 30.0f)
    {
        if (ImGui::IsWindowHovered())
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0)
            {
                target -= wheel * scrollSpeed;
            }
        }

        if (target < 0.0f) target = 0.0f;
        if (target > maxScroll) target = maxScroll;
    }
};

struct ClientColors
{
    ColorAnimation background;
    ColorAnimation secondaryBackground;
    ColorAnimation shadow;
    ColorAnimation brighterBackground;
    ColorAnimation font;

    void UpdateAll(const PrimaryTheme& targetTheme, float dt)
    {
        background.Update(targetTheme.backgroundColor, dt);
        secondaryBackground.Update(targetTheme.secondaryBackgroundColor, dt);
        shadow.Update(targetTheme.shadowColor, dt);
        brighterBackground.Update(targetTheme.brighterBackgroundColor, dt);
        font.Update(targetTheme.fontColor, dt);
    }

    void InitAll(const PrimaryTheme& theme)
    {
        background.Init(theme.backgroundColor);
        secondaryBackground.Init(theme.secondaryBackgroundColor);
        shadow.Init(theme.shadowColor);
        brighterBackground.Init(theme.brighterBackgroundColor);
        font.Init(theme.fontColor);
    }
};







