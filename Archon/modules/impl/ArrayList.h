#pragma once
#include "../Module.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "../../Core/Managers/FontManager.h"
#include "../../Core/Managers/ModuleManager.h"
#include "../../Utils/Render/Shader2DUtil.h"
#include "../../Utils/Render/Render2DUtil.h"

struct RoundRect
{
    float x, y, width, height;
    float radius;

    RoundRect(float _x, float _y, float _w, float _h, float _r)
        : x(_x), y(_y), width(_w), height(_h), radius(_r) {}
        
    float GetX1() const { return x + width; }
    float GetY1() const { return y + height; }
};

struct ListEntry
{
    Module* mod;
    std::string text;
    float textWidth;
    ImU32 textColor; 
    ImU32 rectColor;
};

class ArrayList : public Module
{
public:
    NumberSetting* xPos = New<>(NumberSetting(xorstr_("XPos"), 50.0f, 0.0f, 1920.0f));
    NumberSetting* yPos = New<>(NumberSetting(xorstr_("YPos"), 50.0f, 0.0f, 1280.0f));
    
    NumberSetting* scale = New<>(NumberSetting(xorstr_("Scale"), 1.0f, 1.0f, 3.f, xorstr_("%.1f")));
    NumberSetting* fontSize = New<>(NumberSetting(xorstr_("FontSize"), 20.0f, 1.0f, 50.f, xorstr_("%.1f")));
    BooleanSetting* boldFont = New<>(BooleanSetting(xorstr_("BoldFont"), true));
    NumberSetting* paddingX = New<>(NumberSetting(xorstr_("PaddingX"), 8.0f, 0.0f, 20.f, xorstr_("%.1f")));
    NumberSetting* paddingY = New<>(NumberSetting(xorstr_("PaddingY"), 2.0f, 0.0f, 20.f, xorstr_("%.1f")));
    NumberSetting* itemSpacing = New<>(NumberSetting(xorstr_("Spacing"), 10.0f, 0.0f, 10.f, xorstr_("%.1f")));
    NumberSetting* offsetH = New<>(NumberSetting(xorstr_("HeightOffset"), 0.0f, 0.0f, 20.f, xorstr_("%.1f")));

    BooleanSetting* sortReverse = New<>(BooleanSetting(xorstr_("SortReverse"), false));
    BooleanSetting* sideBar = New<>(BooleanSetting(xorstr_("SideBar"), true));
    
    BooleanSetting* blurBackGround = New<>(BooleanSetting(xorstr_("Blur"), true));
    ModeSetting* renderMode = New<>(ModeSetting(xorstr_("BlurMode"), 0, std::vector<std::string>{ xorstr_("Merged"), xorstr_("Separately") }, [this] { return blurBackGround->value; }));

    NumberSetting* rounding = New<>(NumberSetting(xorstr_("Rounding"), 8.0f, 0.0f, 10.0f, xorstr_("%.1f")));
    ColorSetting* fillColor = New<>(ColorSetting(xorstr_("FillColor"), RGBA(20, 20, 20, 150).ToU32(), [this] { return !blurBackGround->value; }));
    ColorSetting* blurColor = New<>(ColorSetting(xorstr_("BlurColor"), RGBA(30, 30, 30, 200).ToU32(), [this] { return blurBackGround->value; }));
    NumberSetting* blurStrength = New<>(NumberSetting(xorstr_("BlurStrength"), 8.0f, 1.0f, 50.0f, xorstr_("%.1f"), [this] { return blurBackGround->value; }));
    NumberSetting* blurOpacity = New<>(NumberSetting(xorstr_("BlurOpacity"), 0.55f, 0.0f, 1.0f, xorstr_("%.2f"), [this] { return blurBackGround->value; }));
    
    NumberSetting* glowStrength = New<>(NumberSetting(xorstr_("GlowStrength"), 0.0f, 0.0f, 20.0f, xorstr_("%.1f")));
    NumberSetting* glowAlpha = New<>(NumberSetting(xorstr_("GlowAlpha"), 1.0f, 0.1f, 1.0f, xorstr_("%.2f"), [this] { return glowStrength->value > 0.0f; }));

    BooleanSetting* rainbow = New<>(BooleanSetting(xorstr_("Rainbow"), true));
    NumberSetting* rainbowSpeed = New<>(NumberSetting(xorstr_("Speed"), 10.0f, 1.0f, 20.0f, xorstr_("%.1f"), [this] { return rainbow->value; }));
    NumberSetting* saturation  = New<>(NumberSetting(xorstr_("Saturation"), 0.6f, 0.1f, 1.0f, xorstr_("%.2f"), [this] { return rainbow->value; }));
    NumberSetting* offsetColor = New<>(NumberSetting(xorstr_("ColorOffset"), 10.0f, 1.0f, 20.0f, xorstr_("%.1f"), [this] { return rainbow->value; }));
    
    ColorSetting* mainColor = New<>(ColorSetting(xorstr_("MainColor"), RGBA(180, 180, 180, 255).ToU32(), [this] { return !rainbow->value; }));
    ColorSetting* infoColor = New<>(ColorSetting(xorstr_("InfoColor"), RGBA(180, 180, 180, 255).ToU32()));
    
    float posX = 50.0f;
    float posY = 50.0f;

    ArrayList() : Module(xorstr_("ArrayList")) {}

    void OnRender2D(DX12DrawContext context) override
    {
        posX = xPos->value;
        posY = yPos->value;
        bool alignRight = (posX > context.ScreenWidth / 2.0f);

        auto modules = GetSortedModules();
        if (modules.empty()) return;

        std::vector<struct RoundRect> rects;
        PrepareRects(modules, rects, alignRight);

        RenderBlurAndGlow(context, rects);
        RenderModules(context, modules, rects, alignRight);
    }

private:

    ImFont* GetArrayListFont()
    {
        if (boldFont->value) return FontManager::Get().GetMainBoldFont();
        return FontManager::Get().GetMainFont();
    }
    
    std::vector<ListEntry> GetSortedModules()
    {
        std::vector<ListEntry> out;
        auto& allMods = ModuleManager::Get().modules;
        
        ImFont* font = GetArrayListFont();
        float fs = fontSize->value * scale->value;

        for (auto& m : allMods)
        {
            if (!m || !m->enabled->value) continue;

            ListEntry e;
            e.mod = m.get();
            
            e.text = m->name;
            if (!m->displayInfo.empty()) {
                e.text += " [" + m->displayInfo + "]";
            }

            ImVec2 sz = ImGui::CalcTextSize3(e.text.c_str(), font, fs);
            e.textWidth = sz.x;

            out.push_back(e);
        }

        std::sort(out.begin(), out.end(), [](const ListEntry& a, const ListEntry& b) {
            return a.textWidth > b.textWidth;
        });

        if (sortReverse->value) {
            std::reverse(out.begin(), out.end());
        }

        return out;
    }

    void PrepareRects(std::vector<ListEntry>& modules, std::vector<struct RoundRect>& rects, bool alignRight)
    {
        rects.reserve(modules.size());
        
        float s = scale->value;
        float padX = paddingX->value * s;
        float currentY = posY;
        float itemH = (fontSize->value + paddingY->value * 2.0f) * s; 
        if (offsetH->value > 0) itemH = offsetH->value * s;

        float extraW = sideBar->value ? (3.0f * s) : 0.0f;

        for (auto& entry : modules)
        {
            float w = entry.textWidth + (padX * 2.0f) + extraW;
            float h = itemH;
            
            float x = alignRight ? (posX - w) : posX;
            
            rects.emplace_back(x, currentY, w, h, rounding->value * s);

            currentY += h + (itemSpacing->value * s);
        }
    }

    void RenderBlurAndGlow(DX12DrawContext context, const std::vector<struct RoundRect>& rects)
    {
        if (rects.empty()) return;

        float colorOffset = 0.0f;
        if (glowStrength->value > 0.0f)
        {
            for (const auto& r : rects)
            {
                RGBA col = GetCurrentColor(colorOffset);
                Shader2DUtil::Get().OnDrawRoundGlow(
                    context,
                    r.x, r.y, r.width, r.height, r.radius,
                    glowStrength->value, glowAlpha->value, RGBA4C(col)
                );
                colorOffset += offsetColor->value;
            }
        }

        if (blurBackGround->value)
        {
            if (renderMode->index == 0)
            {
                Shader2DUtil::Get().OnDrawMergeBlur(context, rects, RGBA(blurColor->GetU32()), blurStrength->value, blurOpacity->value);
            }
            else
            {
                for (const auto& r : rects)
                {
                    Shader2DUtil::Get().OnDrawRoundBlur(
                        context,
                        r.x, r.y, r.width, r.height,
                        r.radius,
                        RGBA(blurColor->GetU32()),
                        blurStrength->value, blurOpacity->value
                    );
                }
            }
           
        }
        else
        {
            ImU32 fill = fillColor->GetU32();
            for (const auto& r : rects)
            {
                context.BackGroundDrawList->AddRectFilled(
                    ImVec2(r.x, r.y), 
                    ImVec2(r.GetX1(), r.GetY1()), 
                    fill, r.radius
                );
            }
        }
    }

    void RenderModules(DX12DrawContext context, const std::vector<ListEntry>& modules, const std::vector<struct RoundRect>& rects, bool alignRight)
    {
        float s = scale->value;
        ImFont* font = GetArrayListFont();
        float fs = fontSize->value * s;
        float colorOffset = 0.0f;

        for (size_t i = 0; i < modules.size(); i++)
        {
            const auto& e = modules[i];
            const auto& r = rects[i];
            
            RGBA col = GetCurrentColor(colorOffset);

            if (sideBar->value)
            {
                float barW = 2.0f * s;
                float barX = alignRight ? (r.GetX1() - barW) : r.x;
                
                Shader2DUtil::Get().OnDrawRoundedRect(
                    context,
                    barX, r.y, barW, r.height, 
                    0.0f, RGBA4C(col)
                );
            }

            float padX = paddingX->value * s;
            float extra = sideBar->value ? (3.0f * s) : 0.0f;
            float visualFixY = 0.1f * fs;
            float tx = alignRight 
                ? (r.GetX1() - padX - e.textWidth - extra) 
                : (r.x + padX + extra);
                
            ImVec2 sz = ImGui::CalcTextSize3(e.text.c_str(), font, fs);
            float ty = (r.y + (r.height - sz.y) / 2.0f) - visualFixY;

            context.BackGroundDrawList->AddText(font, fs, ImVec2(tx, ty), col.ToU32(), e.text.c_str());

            colorOffset += offsetColor->value;
        }
    }

    RGBA GetCurrentColor(float offset) const
    {
        if (rainbow->value) 
            return Render2DUtil::Astolfo((int)offset, 100.0f, saturation->value, rainbowSpeed->value);
        return RGBA(mainColor->GetU32());
    }
};