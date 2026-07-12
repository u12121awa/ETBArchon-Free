#pragma once
#include "../Module.h"

class ChatSystem : public Module
{
public:
    ChatSystem() : Module(xorstr_("ChatSystem"))
    {
    }
    BooleanSetting* autoClose = New<>(BooleanSetting(xorstr_("AutoClose"), true));
  
    NumberSetting* oldChatOpacity = New<>(NumberSetting(xorstr_("OldChatOpacity"), 0.0f, 0.0f, 1.0f));
    NumberSetting* fontSize = New<>(NumberSetting(xorstr_("FontSize"), 20.0f, 1.0f, 40.0f));
    
    ColorSetting* windowBgColor = New<>(ColorSetting(xorstr_("WindowBgColor"), RGBA(ImVec4(0.0f, 0.0f, 0.0f, 0.5f)).ToU32()));
    NumberSetting* windowRounding = New<>(NumberSetting(xorstr_("WindowRounding"), 5.0f, 0.1f, 10.0f));
    NumberSetting* horizontalPadding = New<>(NumberSetting(xorstr_("HorizontalPadding"), 5.0f, 0.1f, 20.0f));
    
    ColorSetting* inputBgColor = New<>(ColorSetting(xorstr_("InputBgColor"), RGBA(ImVec4(0.2f, 0.2f, 0.2f, 0.8f)).ToU32()));
    NumberSetting* inputRounding = New<>(NumberSetting(xorstr_("InputRounding"), 5.0f, 0.1f, 10.0f));
    NumberSetting* inputPadding = New<>(NumberSetting(xorstr_("InputPadding"), 5.0f, 0.1f, 20.0f));
    NumberSetting* inputBottomOffset = New<>(NumberSetting(xorstr_("InputBottomOffset"), 5.0f, 0.1f, 20.0f));

    NumberSetting* inAnimationDuration = New<>(NumberSetting(xorstr_("In-Anim"), 0.25f, 0.01f, 1.0f));
    BooleanSetting* enableSlideIn = New<>(BooleanSetting(xorstr_("Slide-In"), true));
    BooleanSetting* enableFadeIn = New<>(BooleanSetting(xorstr_("Fade-In"), false));

    NumberSetting* messageLifeTime = New<>(NumberSetting(xorstr_("Life-Time"), 6.0f, 1.0f, 10.0f));
    NumberSetting* fadeDuration = New<>(NumberSetting(xorstr_("Exit-Anim"), 0.2f, 0.1f, 3.0f));

    BooleanSetting* renderFromBottom = New<>(BooleanSetting(xorstr_("RenderFromBottom"), true));
    BooleanSetting* useWipeAnimation = New<>(BooleanSetting(xorstr_("UseWipeAnimation"), true));

    NumberSetting* windowsX = New<>(NumberSetting(xorstr_("WX"), 0.0f, 0.0f, 0.0f, xorstr_("%.1f"), [] { return false; }));
    NumberSetting* windowsY = New<>(NumberSetting(xorstr_("WY"), 0.0f, 0.0f, 0.0f, xorstr_("%.1f"), [] { return false; }));
    NumberSetting* windowsSX = New<>(NumberSetting(xorstr_("WSX"), 400.0f, 0.0f, 0.0f, xorstr_("%.1f"), [] { return false; }));
    NumberSetting* windowsSY = New<>(NumberSetting(xorstr_("WSY"), 300.0f, 0.0f, 0.0f, xorstr_("%.1f"), [] { return false; }));
};
