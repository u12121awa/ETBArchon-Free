#pragma once
#include "../Module.h"

class DynamicIsland : public Module
{
public:
    NumberSetting* scale = New<>(NumberSetting(xorstr_("Scale"), 2.0f, 1.0f, 4.0f, "%.1f", []{ return false; }));
    NumberSetting* yOffset = New<>(NumberSetting(xorstr_("YOffset"), 80.0f, 0.0f, 200.0f));
    NumberSetting* fontSize = New<>(NumberSetting(xorstr_("FontSize"), 24.0f, 1.0f, 30.0f));
    NumberSetting* logoSize = New<>(NumberSetting(xorstr_("LogoSize"), 0.6f, 0.1f, 2.0f));
    NumberSetting* iconSize = New<>(NumberSetting(xorstr_("IconSize"), 50.0f, 1.0f, 80.0f));
    NumberSetting* radius = New<>(NumberSetting(xorstr_("Radius"), 10.0f, 0.0f, 20.0f));
    ColorSetting* blurColor = New<>(ColorSetting(xorstr_("BlurColor"), RGBA(0, 14, 37, 204).ToU32()));
    NumberSetting* blurOpacity = New<>(NumberSetting(xorstr_("BlurOpacity"), 0.55f, 0.0f, 1.0f, xorstr_("%.1f")));
    NumberSetting* blurStrength = New<>(NumberSetting(xorstr_("BlurStrength"), 20.0f, 5.0f, 50.0f, xorstr_("%.1f")));
    NumberSetting* glowStrength = New<>(NumberSetting(xorstr_("GlowStrength"), 8.0f, 1.0f, 20.0f, xorstr_("%.2f")));
    NumberSetting* glowAlpha = New<>(NumberSetting(xorstr_("GlowAlpha"), 0.45f, 0.1f, 1.0f, xorstr_("%.2f")));
    ModeSetting* glowColorMode = New<>(ModeSetting(xorstr_("GlowMode"), 1, std::vector<std::string>{xorstr_("Custom"), xorstr_("Sky")}));
    ColorSetting* glowColor = New<>(ColorSetting(xorstr_("GlowColor"), RGBA(0, 14, 37, 255).ToU32(), [this] { return glowColorMode->index == 0; }));
    ModeSetting* animation = New<>(ModeSetting(xorstr_("Animation"), 4, std::vector<std::string>{xorstr_("Thunder"), xorstr_("Mio"), xorstr_("My"), xorstr_("Old"), xorstr_("Normal"), xorstr_("None")}));
    NumberSetting* speed = New<>(NumberSetting(xorstr_("Speed"), 0.5f, 0.1f, 1.0f));
    NumberSetting* duration = New<>(NumberSetting(xorstr_("Duration"), 800.0f, 0.0f, 1000.0f));
    NumberSetting* moduleDuration = New<>(NumberSetting(xorstr_("MDuration"), 300.0f, 0.0f, 1000.0f));

    DynamicIsland() : Module(xorstr_("DynamicIsland"))
    {
    }
};