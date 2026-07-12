#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../../Utils/Render/Shader/DX12Shader.h"
#include "../../Utils/ClientData.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

enum KeyType
{
    Pressed,
    Released
};


struct Bind
{
    int key;
    bool mouse;
    bool hold;

    Bind(int key = -1, bool mouse = false, bool hole = false)
        : key(key), mouse(mouse), hold(hole)
    {
    }

    std::string ToString() const
    {
        if (key == -1) return xorstr_("None");

        if (mouse)
        {
            return xorstr_("M") + std::to_string(key);
        }

        char nameBuffer[128] = {0};
        UINT scanCode = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);

        switch (key)
        {
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_RCONTROL:
        case VK_RMENU:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_INSERT:
        case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK:
            scanCode |= 0x100;
            break;
        }

        if (GetKeyNameTextA(scanCode << 16, nameBuffer, 128) > 0)
        {
            std::string name(nameBuffer);
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            return name;
        }

        return xorstr_("KEY ") + std::to_string(key);
    }
};


enum class SettingType { Boolean, Number, Mode, String, Bind, Color, Group, BooleanGroup };

struct Setting
{
    using VisibilityCheck = std::function<bool()>;

    std::string name;
    SettingType type;
    std::string module;
    bool expand;
    Setting* group;
    
    VisibilityCheck isVisible; 
    
    Setting(std::string name, SettingType type, VisibilityCheck visible = nullptr) 
        : name(name), type(type), isVisible(visible), expand(false), group(nullptr)
    {
    }

    virtual ~Setting() = default;
    
    bool IsVisible() const
    {
        if (group && !group->expand)
        {
            return false;
        }
        if (isVisible == nullptr) return true;
        return isVisible();
    }
    
    template <typename T>
    void SetGroup(T* g)
    {
        group = g;
    }
};

struct GroupSetting : Setting
{
    FloatAnimation arrowAnim;

    GroupSetting(std::string name, bool expand = false, VisibilityCheck visible = nullptr)
        : Setting(name, SettingType::Group, visible)
    {
        this->expand = expand;
        arrowAnim.Init(expand ? 1.0f : 0.0f);
    }
};

struct BooleanGroupSetting : Setting
{
    bool value;
    FloatAnimation arrowAnim;
    FloatAnimation toggleAnim;
    
    BooleanGroupSetting(std::string name, bool value, bool expand = false, VisibilityCheck visible = nullptr)
        : Setting(name, SettingType::BooleanGroup, visible), value(value)
    {
        this->expand = expand;
        arrowAnim.Init(expand ? 1.0f : 0.0f);
        toggleAnim.Init(value ? 1.0f : 0.0f);
    }
};

struct BooleanSetting : Setting
{
    bool value;
    std::vector<std::shared_ptr<Setting>> subSettings;
    FloatAnimation toggleAnim;
    FloatAnimation arrowAnim;
    
    BooleanSetting(std::string name, bool value, VisibilityCheck visible = nullptr)
        : Setting(name, SettingType::Boolean, visible), value(value)
    {
        toggleAnim.Init(value ? 1.0f : 0.0f);
    }
};

struct NumberSetting : Setting
{
    float value, min, max;
    std::string format;
    
    NumberSetting(std::string name, float value, float minValue, float maxValue, std::string format = xorstr_("%.1f"), VisibilityCheck visible = nullptr)
        : Setting(name, SettingType::Number, visible), value(value), min(minValue), max(maxValue), format(format)
    {
    }
};

struct ModeSetting : Setting
{
    int index;
    bool expanded;
    std::vector<std::string> modes;

    FloatAnimation heightAnim;
    FloatAnimation toggleAnim;

    std::vector<FloatAnimation> itemAnims;
    std::vector<FloatAnimation> hoverAnims;
    
    ModeSetting(std::string name, int idx, std::vector<std::string> mods, VisibilityCheck visible = nullptr)
        : Setting(name, SettingType::Mode, visible), index(idx), modes(mods), expanded(false)
    {
        heightAnim.Init(24.0f);
        itemAnims.resize(modes.size());
        hoverAnims.resize(modes.size());
    }

    std::string GetCurrent() { return modes[index]; }
    void Cycle() { index = (index + 1) % modes.size(); }
};

struct StringSetting : Setting
{
    std::string text;
    
    StringSetting(std::string name, std::string string, VisibilityCheck visible = nullptr) 
        : Setting(name, SettingType::String, visible), text(string)
    {
    }
};

struct BindSetting : Setting
{
    Bind bind;
    bool listening;
    FloatAnimation toggleAnim;
    
    BindSetting(std::string name, Bind bind, VisibilityCheck visible = nullptr) 
        : Setting(name, SettingType::Bind, visible), bind(bind), listening(false)
    {
    }
};

struct ColorSetting : Setting
{
    float color[4];
    float defaultColor[4];
    bool rainbow;
    bool open;

    float hue, saturation, brightness, alpha;
    
    ColorSetting(std::string name, ImColor imColor, VisibilityCheck visible = nullptr) 
        : Setting(name, SettingType::Color, visible), rainbow(false), open(false)
    {
        color[0] = imColor.Value.x;
        color[1] = imColor.Value.y;
        color[2] = imColor.Value.z;
        color[3] = imColor.Value.w;

        memcpy(defaultColor, color, sizeof(float) * 4);

        UpdateHSB();
    }
    
    void UpdateHSB()
    {
        ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], hue, saturation, brightness);
        alpha = color[3];
    }

    void UpdateFromHSB()
    {
        ImGui::ColorConvertHSVtoRGB(hue, saturation, brightness, color[0], color[1], color[2]);
        color[3] = alpha;
    }

    ImU32 GetNU32()
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
    }

    ImU32 GetU32()
    {
        if (rainbow)
        {
            return GetRainbowU32();
        }
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
    }

    ImU32 GetU32WithAlpha(float a)
    {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(GetU32());
        c.w = a;
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    ImU32 GetRainbowU32()
    {
        float speed = 0.2f;
        float time = (float)ImGui::GetTime();

        float currentHue = hue - (time * speed);
        currentHue = currentHue - floor(currentHue);

        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(currentHue, saturation, brightness, r, g, b);

        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, alpha));
    }

    void SetDefault()
    {
        memcpy(color, defaultColor, sizeof(float) * 4);
        UpdateHSB();
        rainbow = false;
    }

    void SetColor(ImColor c)
    {
        color[0] = c.Value.x;
        color[1] = c.Value.y;
        color[2] = c.Value.z;
        color[3] = c.Value.w;
        UpdateHSB();
    }
};