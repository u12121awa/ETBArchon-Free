#pragma once
#include "../Module.h"
#include "../../Utils/Render/Shader2DUtil.h"

class ClickGUI final : public Module
{
public:
    BooleanSetting* enabledBlur = New<>(BooleanSetting(xorstr_("EnabledBlur"), true));
    NumberSetting* currentTheme = New<>(NumberSetting(xorstr_("CurrentTheme"), 0.0f, 0.0f, 0.0f, xorstr_("%.0f"), []{return false;}));
    NumberSetting* blurRadius = New<>(NumberSetting(xorstr_("BlurRadius"), 5.0f, 0.0f, 0.0f, xorstr_("%.2f"), []{return false;}));
    NumberSetting* blurStrength = New<>(NumberSetting(xorstr_("BlurStrength"), 15.0f, 0.0f, 0.0f, xorstr_("%.2f"), []{return false;}));
    NumberSetting* blurOpacity = New<>(NumberSetting(xorstr_("BlurOpacity"), 0.55f, 0.0f, 0.0f, xorstr_("%.2f"), []{return false;}));
    NumberSetting* glowStrength = New<>(NumberSetting(xorstr_("GlowStrength"), 8.0f, 0.0f, 0.0f, xorstr_("%.2f"), []{return false;}));
    NumberSetting* glowAlpha = New<>(NumberSetting(xorstr_("GlowAlpha"), 0.6f, 0.0f, 0.0f, xorstr_("%.2f"), []{return false;}));

    ClickGUI() : Module(xorstr_("ClickGUI"))
    {
        bind->bind = Bind('I');
    }

    void OnEnable() override;

    void OnUpdate() override
    {
        Shader2DUtil::Get().SetEnableBlur(enabledBlur->value);
    }
};
