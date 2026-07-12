#pragma once

#include "../Utils/Render/DX12DrawContext.h"
#include "../Utils/Render/Render2DUtil.h"
#include "../Utils/Render/Shader2DUtil.h"
#include "Module.h"
#include "../Core/Managers/FontManager.h"
#include "../Utils/Utils.h"
#include "../Utils/World/Events.h"
#include "settings/Settings.h""


inline void CustomSlider(const char* label, float* v, float v_min, float v_max, const char* format,
                         ClientColors m_clientColors)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float labelWidth = 65.0f;
    float sliderWidth = 115.0f;
    float height = 14.0f;
    float trackHeight = 4.0f;
    float grabRadius = 5.0f;

    ImVec2 startPos = ImGui::GetCursorScreenPos();

    float textOffsetY = (height - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + textOffsetY));
    ImGui::Text(xorstr_("%s"), label);

    ImVec2 sliderPos = ImVec2(startPos.x + labelWidth, startPos.y);

    ImGui::PushID(label);

    ImGui::SetCursorScreenPos(sliderPos);
    ImGui::InvisibleButton(xorstr_("##Slider"), ImVec2(sliderWidth, height));

    bool isHovered = ImGui::IsItemHovered();
    bool isActive = ImGui::IsItemActive();

    if (isActive && ImGui::IsMouseDown(0))
    {
        float mouseX = ImGui::GetIO().MousePos.x;
        float t = (mouseX - sliderPos.x) / sliderWidth;
        t = ImClamp(t, 0.0f, 1.0f);
        *v = v_min + t * (v_max - v_min);
    }

    ImGuiID animId = ImGui::GetID(xorstr_("##AnimVal"));
    float* animVal = ImGui::GetStateStorage()->GetFloatRef(animId, *v);

    float dt = ImGui::GetIO().DeltaTime;
    float speed = 15.0f;
    *animVal += (*v - *animVal) * dt * speed;

    if (std::abs(*v - *animVal) < 0.001f * (v_max - v_min))
    {
        *animVal = *v;
    }
    float t = (*animVal - v_min) / (v_max - v_min);
    t = ImClamp(t, 0.0f, 1.0f);

    float trackY = sliderPos.y + (height - trackHeight) * 0.5f;

    drawList->AddRectFilled(
        ImVec2(sliderPos.x, trackY),
        ImVec2(sliderPos.x + sliderWidth, trackY + trackHeight),
        m_clientColors.shadow.GetU32WithAlpha(0.5f),
        trackHeight * 0.5f
    );

    ImU32 activeColor = isActive ? m_clientColors.font.GetU32() : m_clientColors.font.GetU32();
    drawList->AddRectFilled(
        ImVec2(sliderPos.x, trackY),
        ImVec2(sliderPos.x + sliderWidth * t, trackY + trackHeight),
        activeColor,
        trackHeight * 0.5f
    );

    if (isActive)
    {
        drawList->AddCircleFilled(
            ImVec2(sliderPos.x + sliderWidth * t, trackY + trackHeight * 0.5f),
            grabRadius + 2.0f,
            m_clientColors.shadow.GetU32WithAlpha(0.3f)
        );
    }
    drawList->AddCircleFilled(
        ImVec2(sliderPos.x + sliderWidth * t, trackY + trackHeight * 0.5f),
        grabRadius,
        activeColor
    );

    ImVec2 valuePos = ImVec2(startPos.x + labelWidth + sliderWidth + 10.0f, startPos.y + textOffsetY);
    ImGui::SetCursorScreenPos(valuePos);

    ImGuiID inputId = ImGui::GetID(xorstr_("##InputState"));
    bool* isEditing = ImGui::GetStateStorage()->GetBoolRef(inputId, false);

    if (*isEditing)
    {
        float inputWidth = 50.0f;
        ImGui::PushItemWidth(inputWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, m_clientColors.shadow.GetU32WithAlpha(0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, m_clientColors.font.GetU32());

        if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere(0);

        if (ImGui::InputFloat(xorstr_("##v"), v, 0.0f, 0.0f, format, ImGuiInputTextFlags_AutoSelectAll))
        {
        }

        if (ImGui::IsItemDeactivated()) *isEditing = false;

        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();
    }
    else
    {
        char buf[64];
        sprintf_s(buf, format, *v);
        ImGui::Text(xorstr_("%s"), buf);

        ImVec2 textSize = ImGui::CalcTextSize(buf);
        ImGui::SetCursorScreenPos(valuePos);
        if (ImGui::InvisibleButton(xorstr_("##ValueBtn"), ImVec2(textSize.x > 30 ? textSize.x : 30, height)))
            *isEditing =
                true;
        if (ImGui::IsItemHovered())
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddLine(ImVec2(valuePos.x, valuePos.y + textSize.y),
                        ImVec2(valuePos.x + textSize.x, valuePos.y + textSize.y), m_clientColors.font.GetU32());
        }
    }

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + height + 12.0f));
}

static BindSetting* BindingSetting = nullptr;

class WidgetRenderer
{
public:
    static void DrawArrow(ImDrawList* dl, ImVec2 center, float size, float anim, ImU32 color)
    {
        float angle = (1.0f - anim) * IM_PI;
        
        FontManager::Get().DrawIconRotated(dl, center, "e", 16.0f, angle, color);
    }

    static void RenderGroup(float x, float y, float w, GroupSetting* s, const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float dt = ImGui::GetIO().DeltaTime;
        float h = 20.0f;

        float rightMargin = 5.0f;
        float arrowWidthAlloc = 20.0f;
        float arrowSize = 8.0f;

        s->arrowAnim.Update(s->expand ? 1.0f : 0.0f, dt, 15.0f);
        float anim = s->arrowAnim.value;

        dl->AddText(ImVec2(x, y - 1.0f), colors.font.GetU32(), s->name.c_str());

        float arrowCenterX = x + w - rightMargin - (arrowWidthAlloc / 2.0f);
        float arrowCenterY = y + h / 2.0f - 2.0f;

        ImU32 arrowColor = colors.font.GetU32WithAlpha(0.5f + 0.5f * anim);

        DrawArrow(dl, ImVec2(arrowCenterX, arrowCenterY), arrowSize, anim, arrowColor);

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(s);

        ImGui::InvisibleButton("##Grp", ImVec2(w, h));

        if (ImGui::IsItemClicked(1))
        {
            s->expand = !s->expand;
        }

        ImGui::PopID();
    }

    static void RenderBooleanGroup(float x, float y, float w, BooleanGroupSetting* s, const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float dt = ImGui::GetIO().DeltaTime;

        float height = 20.0f;

        float rightMargin = 5.0f;
        float arrowWidthAlloc = 20.0f;
        float arrowSize = 8.0f;

        float arrowSwitchSpacing = 5.0f;

        float switchWidth = 40.0f;
        float circleScale = 10.0f;
        float inPadding = 6.0f;

        s->toggleAnim.Update(s->value ? 1.0f : 0.0f, dt, 10.0f);
        float toggleAnimVal = s->toggleAnim.value;

        s->arrowAnim.Update(s->expand ? 1.0f : 0.0f, dt, 15.0f);
        float arrowAnimVal = s->arrowAnim.value;

        ImVec4 colEnabled = ImGui::ColorConvertU32ToFloat4(colors.font.GetU32());
        ImVec4 colDisabled = ImGui::ColorConvertU32ToFloat4(colors.font.GetU32WithAlpha(0.39f));
        ImVec4 curCol = ImVec4(
            colDisabled.x + (colEnabled.x - colDisabled.x) * toggleAnimVal,
            colDisabled.y + (colEnabled.y - colDisabled.y) * toggleAnimVal,
            colDisabled.z + (colEnabled.z - colDisabled.z) * toggleAnimVal,
            colDisabled.w + (colEnabled.w - colDisabled.w) * toggleAnimVal
        );
        ImU32 finalColor = ImGui::ColorConvertFloat4ToU32(curCol);

        float arrowCenterX = x + w - rightMargin - (arrowWidthAlloc / 2.0f);
        float arrowCenterY = y + height / 2.0f - 2.0f;

        float boxX = x + w - rightMargin - arrowWidthAlloc - arrowSwitchSpacing - switchWidth;
        float boxY = y;

        dl->AddRectFilled(
            ImVec2(boxX, boxY),
            ImVec2(boxX + switchWidth, boxY + height),
            colors.secondaryBackground.GetU32(),
            5.f
        );

        float xOffset = inPadding + (switchWidth - circleScale - 2 * inPadding) * toggleAnimVal;
        float knobX = boxX + xOffset;
        float knobY = boxY + (height - circleScale) / 2.0f;
        float radius = circleScale / 2.0f;

        dl->AddCircleFilled(ImVec2(knobX + radius, knobY + radius), radius, finalColor);

        dl->AddText(ImVec2(x, y - 1.0f), RGBA(colEnabled).ToU32(), s->name.c_str());

        ImU32 arrowColor = colors.font.GetU32WithAlpha(0.4f + 0.6f * arrowAnimVal);
        DrawArrow(dl, ImVec2(arrowCenterX, arrowCenterY), arrowSize, arrowAnimVal, arrowColor);

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(s);

        if (ImGui::InvisibleButton("##BoolGrp", ImVec2(w, height + 5.0f)))
        {
            s->value = !s->value;
        }

        if (ImGui::IsItemClicked(1))
        {
            s->expand = !s->expand;
        }

        ImGui::PopID();
    }
    static void RenderCheckbox(float x, float y, float w, BooleanSetting* setting, const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float dt = ImGui::GetIO().DeltaTime;

        float height = 20.0f;
        float width = 40.0f;
        float circleScale = 10.0f;
        float inPadding = 6.0f;
        float rightMargin = 5.0f;

        setting->toggleAnim.Update(setting->value ? 1.0f : 0.0f, dt, 10.0f);
        float anim = setting->toggleAnim.value;

        ImVec4 colEnabled = ImGui::ColorConvertU32ToFloat4(colors.font.GetU32());
        ImVec4 colDisabled = ImGui::ColorConvertU32ToFloat4(colors.font.GetU32WithAlpha(0.39f));

        ImVec4 curCol = ImVec4(
            colDisabled.x + (colEnabled.x - colDisabled.x) * anim,
            colDisabled.y + (colEnabled.y - colDisabled.y) * anim,
            colDisabled.z + (colEnabled.z - colDisabled.z) * anim,
            colDisabled.w + (colEnabled.w - colDisabled.w) * anim
        );
        ImU32 finalColor = ImGui::ColorConvertFloat4ToU32(curCol);

        float boxX = x + w - width - rightMargin;
        float boxY = y;

        dl->AddRectFilled(
            ImVec2(boxX, boxY),
            ImVec2(boxX + width, boxY + height),
            colors.secondaryBackground.GetU32(),
            5.f
        );

        float xOffset = inPadding + (width - circleScale - 2 * inPadding) * anim;
        float knobX = boxX + xOffset;
        float knobY = boxY + (height - circleScale) / 2.0f;

        float radius = circleScale / 2.0f;
        ImVec2 knobCenter(knobX + radius, knobY + radius);

        dl->AddCircleFilled(knobCenter, radius, finalColor);

        dl->AddText(ImVec2(x, y - 1.0f), RGBA(colEnabled).ToU32(), setting->name.c_str());

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(setting);

        if (ImGui::InvisibleButton(xorstr_("##Chk"), ImVec2(w, height + 5.0f)))
        {
            setting->value = !setting->value;
        }

        ImGui::PopID();
    }

    static void RenderSlider(float x, float y, float w, NumberSetting* setting, const ClientColors& colors)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float labelWidth = 20.0f;
        float height = 15.0f;
        float trackHeight = 4.0f;
        float grabRadius = 5.0f;

        char buf[64];
        sprintf_s(buf, setting->format.c_str(), setting->value);

        float widthBox = 20.0f;
        float boxX = x + w - widthBox - (ImGui::CalcTextSize(buf).x / 2.0f);

        ImVec2 startPos(x, y);
        float stringWidth = ImGui::CalcTextSize(setting->name.c_str()).x;

        float sliderStartX = startPos.x + stringWidth + labelWidth;
        float sliderEndX = x + w - widthBox - 30;
        float sliderWidth = sliderEndX - sliderStartX;


        drawList->AddText(ImVec2(startPos.x, startPos.y - 1.0f), colors.font.GetU32(), setting->name.c_str());

        ImVec2 sliderPos = ImVec2(startPos.x + stringWidth + labelWidth, startPos.y);

        ImGui::SetCursorScreenPos(sliderPos);
        ImGui::PushID(setting);
        ImGui::InvisibleButton(xorstr_("##Slider"), ImVec2(sliderWidth, height));

        bool isHovered = ImGui::IsItemHovered();
        bool isActive = ImGui::IsItemActive();

        if (isActive && ImGui::IsMouseDown(0))
        {
            float mouseX = ImGui::GetIO().MousePos.x;
            float t = (mouseX - sliderPos.x) / sliderWidth;
            t = ImClamp(t, 0.0f, 1.0f);
            setting->value = Utils::FormatFloatValue(setting->min + t * (setting->max - setting->min), setting->format.c_str());
        }

        ImGuiID animId = ImGui::GetID(xorstr_("##AnimVal"));
        float* animVal = ImGui::GetStateStorage()->GetFloatRef(animId, setting->value);

        float dt = ImGui::GetIO().DeltaTime;
        float speed = 15.0f;
        *animVal += (setting->value - *animVal) * dt * speed;

        if (std::abs(setting->value - *animVal) < 0.001f * (setting->max - setting->min))
        {
            *animVal = setting->value;
        }

        float t = (*animVal - setting->min) / (setting->max - setting->min);
        t = ImClamp(t, 0.0f, 1.0f);

        float trackY = sliderPos.y + 1.0f + (height - trackHeight) * 0.5f;

        drawList->AddRectFilled(
            ImVec2(sliderPos.x, trackY),
            ImVec2(sliderPos.x + sliderWidth, trackY + trackHeight),
            colors.shadow.GetU32WithAlpha(0.5f),
            trackHeight * 0.5f
        );

        ImU32 activeColor = isActive ? colors.font.GetU32() : colors.font.GetU32();
        drawList->AddRectFilled(
            ImVec2(sliderPos.x, trackY),
            ImVec2(sliderPos.x + sliderWidth * t, trackY + trackHeight),
            activeColor,
            trackHeight * 0.5f
        );

        if (isActive)
        {
            drawList->AddCircleFilled(
                ImVec2(sliderPos.x + sliderWidth * t, trackY + trackHeight * 0.5f),
                grabRadius + 2.0f,
                colors.shadow.GetU32WithAlpha(0.3f)
            );
        }
        drawList->AddCircleFilled(
            ImVec2(sliderPos.x + sliderWidth * t, trackY + trackHeight * 0.5f),
            grabRadius,
            activeColor
        );

        ImVec2 valuePos = ImVec2(boxX, startPos.y - 1.0f);
        ImGui::SetCursorScreenPos(valuePos);

        ImGuiID inputId = ImGui::GetID(xorstr_("##InputState"));
        bool* isEditing = ImGui::GetStateStorage()->GetBoolRef(inputId, false);

        if (*isEditing)
        {
            float inputWidth = 50.0f;
            ImGui::PushItemWidth(inputWidth);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.shadow.GetU32WithAlpha(0.5f));
            ImGui::PushStyleColor(ImGuiCol_Text, colors.font.GetU32());

            if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere(0);

            if (ImGui::InputFloat(xorstr_("##v"), &setting->value, 0.0f, 0.0f, setting->format.c_str(),
                                  ImGuiInputTextFlags_AutoSelectAll))
            {
            }

            if (ImGui::IsItemDeactivated()) *isEditing = false;

            ImGui::PopStyleColor(2);
            ImGui::PopItemWidth();
        }
        else
        {
            drawList->AddText(valuePos, colors.font.GetU32(), buf);

            ImVec2 textSize = ImGui::CalcTextSize(buf);
            ImGui::SetCursorScreenPos(valuePos);
            if (ImGui::InvisibleButton(xorstr_("##ValueBtn"), ImVec2(textSize.x > 30 ? textSize.x : 30, height)))
                *isEditing
                    = true;

            if (ImGui::IsItemHovered())
            {
                drawList->AddLine(ImVec2(valuePos.x, valuePos.y + textSize.y),
                                  ImVec2(valuePos.x + textSize.x, valuePos.y + textSize.y), colors.font.GetU32());
            }
        }

        ImGui::PopID();
    }

   static void RenderMode(float x, float y, float w, ModeSetting* setting, const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float dt = ImGui::GetIO().DeltaTime;

        setting->toggleAnim.Update(setting->expanded ? 1.0f : 0.0f, dt, 10.0f);
        float globalAlpha = setting->toggleAnim.value;

        float hHeader = 24.0f;
        float widthBox = 60.0f * 2;

        float targetH = hHeader;
        if (setting->expanded) targetH += setting->modes.size() * 24.0f + 4.0f;
        setting->heightAnim.Update(targetH, dt, 15.0f);
        float curH = setting->heightAnim.value;

        float boxX = x + w - widthBox - 5.0f;

        dl->AddRectFilled(ImVec2(boxX, y - 4.0f), ImVec2(boxX + widthBox, y + curH),
                          colors.secondaryBackground.GetU32(), 6.0f);

        dl->AddText(ImVec2(x, y - 1.0f), colors.font.GetU32(), setting->name.c_str());
        dl->AddText(ImVec2(boxX + 10.0f, y - 1.0f), colors.font.GetU32(), setting->GetCurrent().c_str());

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(setting);
        ImGui::InvisibleButton(xorstr_("##Head"), ImVec2(w, hHeader));
        if (ImGui::IsItemClicked(0)) setting->Cycle();
        if (ImGui::IsItemClicked(1)) setting->expanded = !setting->expanded;

        if (curH > hHeader + 1.0f)
        {
            dl->PushClipRect(ImVec2(boxX, y + hHeader), ImVec2(boxX + widthBox, y + curH), true);
            
            if (setting->itemAnims.size() != setting->modes.size()) setting->itemAnims.resize(setting->modes.size());
            if (setting->hoverAnims.size() != setting->modes.size()) setting->hoverAnims.resize(setting->modes.size());
            
            size_t safeSize = setting->modes.size(); 

            for (int i = 0; i < safeSize; ++i)
            {
                if (i >= setting->itemAnims.size() || 
                    i >= setting->hoverAnims.size() || 
                    i >= setting->modes.size()) 
                {
                    break; 
                }

                float itemY = y + hHeader + 4.0f + i * 24.0f;

                ImGui::SetCursorScreenPos(ImVec2(boxX, itemY));
                std::string id = xorstr_("##Itm") + std::to_string(i);

                if (ImGui::InvisibleButton(id.c_str(), ImVec2(widthBox, 24.0f)))
                {
                    setting->index = i;
                }
                bool hovered = ImGui::IsItemHovered();
                bool selected = (setting->index == i);
                
                setting->itemAnims[i].Update(selected ? 1.0f : 0.0f, dt, 15.0f);
                setting->hoverAnims[i].Update((hovered || selected) ? 1.0f : 0.0f, dt, 8.0f);

                float selVal = setting->itemAnims[i].value;
                float hovVal = setting->hoverAnims[i].value;

                ImU32 baseRGB = colors.font.GetU32();

                float finalAlpha = globalAlpha * (0.6f + 0.4f * hovVal);

                ImU32 finalCol = RGBA(baseRGB).WithAlpha(finalAlpha * 255.0f).ToU32();

                dl->AddText(ImVec2(boxX + 10.0f, itemY), finalCol, setting->modes[i].c_str());
            }
            
            dl->PopClipRect(); 
        }
        ImGui::PopID();
    }

    static void RenderBind(float x, float y, float w, BindSetting* setting, const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float dt = ImGui::GetIO().DeltaTime;
        float h = 20.0f;

        setting->toggleAnim.Update(setting->listening ? 0.0f : 1.0f, dt, 10.0f);
        float alpha = setting->toggleAnim.value;

        std::string txt = setting->listening ? "..." : setting->bind.ToString();
        ImVec2 sz = ImGui::CalcTextSize(txt.c_str());
        float boxW = sz.x + 20.0f;

        float boxX = x + w - boxW - 5.0f;
        dl->AddRectFilled(ImVec2(boxX, y - 1.7f), ImVec2(boxX + boxW, y + 1.f + h), colors.secondaryBackground.GetU32(),
                          4.0f);

        dl->AddText(ImVec2(boxX + 5.0f * 2, y), RGBA(colors.font.GetU32()).WithAlpha(127.5f + (127.5f * alpha)).ToU32(),
                    txt.c_str());
        dl->AddText(ImVec2(x, y - 1.0f), colors.font.GetU32(), setting->name.c_str());

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(setting);
        if (ImGui::InvisibleButton(xorstr_("##Bnd"), ImVec2(w, h + 5.0f)))
        {
            setting->listening = true;
            BindingSetting = setting;
        }

        ImGui::PopID();
    }


    static void RenderString(float x, float y, float w, StringSetting* setting, const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        float hHeader = 24.0f;
        float widthBox = 100.f;


        float boxX = x + w - widthBox - 5.0f;

        dl->AddRectFilled(ImVec2(boxX, y - 4.0f), ImVec2(boxX + widthBox, y + hHeader),
                          colors.secondaryBackground.GetU32(), 6.0f);
        dl->AddText(ImVec2(x, y - 1.0f), colors.font.GetU32(), setting->name.c_str());

        float inputWidth = 80.0f;
        float startX = x + (w + 60.0) / 2.0f;
        ImGui::SetCursorScreenPos(ImVec2(startX, y - 2.0f));

        ImGui::PushItemWidth(inputWidth);
        ImGui::PushID(setting);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.shadow.GetU32WithAlpha(0.f));
        ImGui::PushStyleColor(ImGuiCol_Text, colors.font.GetU32());

        float textWidth = ImGui::CalcTextSize(setting->text.c_str()).x;

        float padX = (inputWidth - textWidth) * 0.5f - ImGui::GetStyle().FramePadding.x;

        if (padX < 0.0f) padX = 0.0f;

        bool isEditing = (ImGui::GetActiveID() == ImGui::GetID(xorstr_("##Str")));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padX, ImGui::GetStyle().FramePadding.y));


        char buf[128];
        strcpy_s(buf, setting->text.c_str());

        if (ImGui::InputText(xorstr_("##Str"), buf, 128))
        {
            setting->text = buf;
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::PopID();
        ImGui::PopItemWidth();
    }


    static void RenderColor(DX12DrawContext& context, float x, float y, float w, ColorSetting* setting,
                            const ClientColors& colors)
    {
        static ColorSetting* activePicker = nullptr;
        static ImVec2 windowPos = ImVec2(100, 100);

        bool toggled = false;

        {
            ImDrawList* dl = ImGui::GetWindowDrawList();

            float colorHeight = 15.0f;
            float ColorWidth = 35.0f;
            float rightMargin = 5.0f;

            ImVec4 colEnabled = ImGui::ColorConvertU32ToFloat4(colors.font.GetU32());
            float boxX = x + w - ColorWidth - rightMargin;
            float boxY = y;
            dl->AddRectFilled(ImVec2(boxX, boxY), ImVec2(boxX + ColorWidth, boxY + colorHeight), setting->GetU32(),
                              4.f);

            Shader2DUtil::Get().OnDrawRoundGlow(context, boxX, boxY, ColorWidth, colorHeight, 4.f, 8,
                                                RGBA(setting->GetU32()).FA() / 255.f * 0.6f,
                                                RGBA(RGBA(setting->GetU32())));
            dl->AddText(ImVec2(x, y - 1.0f), RGBA(colEnabled).ToU32(), setting->name.c_str());

            ImGui::SetCursorScreenPos(ImVec2(x, boxY));
            std::string btnId = xorstr_("##ColBtn_") + setting->module + xorstr_(" / ") + setting->name;

            if (ImGui::InvisibleButton(btnId.c_str(), ImVec2(w, colorHeight + 5.0f)))
            {
                toggled = true;

                if (activePicker == setting)
                {
                    activePicker = nullptr;
                }
                else
                {
                    activePicker = setting;
                    float gap = 20.0f;
                    float targetX = context.WindowsPos.x + context.WindowsSize.x + gap;
                    float targetY = context.WindowsPos.y;
                    windowPos = ImVec2(targetX, targetY);
                }
            }
        }

        if (activePicker == setting)
        {
            float width = 250.0f;
            float height = 365.0f;

            ImGui::SetNextWindowPos(windowPos);
            ImGui::SetNextWindowSize(ImVec2(width, height));

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

            if (ImGui::Begin(xorstr_("##ColorPickerWindow"), nullptr, flags))
            {
                if (ImGui::IsMouseClicked(0) &&
                    !ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
                    !toggled)
                {
                    activePicker = nullptr;
                }

                DX12DrawContext popupContext = context;
                ImDrawList* winDl = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetWindowPos();

                ImU32 bgCol = colors.background.GetU32();
                ImU32 secBgCol = colors.secondaryBackground.GetU32();
                ImU32 brightBgCol = colors.brighterBackground.GetU32();
                ImU32 fontCol = colors.font.GetU32();

                Shader2DUtil::Get().PauseScissor();
                Shader2DUtil::Get().OnDrawRoundGlow(popupContext, pos.x, pos.y, width, height, 5.0f, 8.0f, 0.5f,
                                                    RGBA4C(RGBA(secBgCol)));
                Shader2DUtil::Get().OnDrawRoundBlur(popupContext, pos.x, pos.y, width, height, 5.0f, RGBA(bgCol), 15.0f,
                                                    0.6f);

                float headerHeight = 30.0f;
                Shader2DUtil::Get().OnDrawRoundedRect(popupContext, pos.x, pos.y, width, headerHeight, 5.0f,
                                                      RGBA4C(RGBA(brightBgCol)));
                Shader2DUtil::Get().ResumeScissor();
                winDl->AddRectFilled(ImVec2(pos.x, pos.y + headerHeight / 2.0f),
                                     ImVec2(pos.x + width, pos.y + headerHeight), brightBgCol);

                winDl->AddText(ImVec2(pos.x + 10, pos.y + 6), fontCol,
                               std::string(setting->module + xorstr_(" / ") + setting->name).c_str());

                const char* closeIcon = xorstr_("X");
                ImVec2 closePos = ImVec2(pos.x + width - 25, pos.y + 5);
                ImGui::SetCursorScreenPos(ImVec2(closePos.x - 2, closePos.y - 2));
                if (ImGui::InvisibleButton(xorstr_("##ClosePick"), ImVec2(20, 20))) activePicker = nullptr;
                ImU32 closeColor = ImGui::IsItemHovered() ? RGBA(ImColor(255, 100, 100)).ToU32() : fontCol;
                winDl->AddText(closePos, closeColor, closeIcon);

                ImGui::SetCursorScreenPos(pos);
                ImGui::InvisibleButton(xorstr_("##DragArea"), ImVec2(width - 30, headerHeight));

                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
                {
                    windowPos.x += ImGui::GetIO().MouseDelta.x;
                    windowPos.y += ImGui::GetIO().MouseDelta.y;
                }

                float contentY = pos.y + headerHeight + 10.0f;
                ImGui::SetCursorScreenPos(ImVec2(pos.x + 10, contentY));
                ImGui::BeginGroup();
                {
                    ImGuiColorEditFlags pickerFlags = ImGuiColorEditFlags_NoSidePreview |
                        ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar
                        | ImGuiColorEditFlags_DisplayRGB;

                    ImGui::PushItemWidth(width - 20);
                    if (ImGui::ColorPicker4(xorstr_("##picker"), setting->color, pickerFlags)) setting->UpdateHSB();
                    ImGui::PopItemWidth();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float r = (setting->color[0] * 255.0f);
                    float g = (setting->color[1] * 255.0f);
                    float b = (setting->color[2] * 255.0f);
                    float a = (setting->color[3] * 255.0f);

                    float itemWidth = (width - 20 - 15) / 4.0f;
                    ImGui::PushItemWidth(itemWidth);

                    CustomSlider(xorstr_("R"), &r, 0.0f, 255.0f, xorstr_("%.0f"), colors);
                    CustomSlider(xorstr_("G"), &g, 0.0f, 255.0f, xorstr_("%.0f"), colors);
                    CustomSlider(xorstr_("B"), &b, 0.0f, 255.0f, xorstr_("%.0f"), colors);
                    CustomSlider(xorstr_("A"), &a, 0.0f, 255.0f, xorstr_("%.0f"), colors);
                    ImGui::PopItemWidth();


                    setting->color[0] = r / 255.0f;
                    setting->color[1] = g / 255.0f;
                    setting->color[2] = b / 255.0f;
                    setting->color[3] = a / 255.0f;
                    setting->UpdateHSB();

                    float btnWidth = (width - 20 - 10) / 3.0f;
                    static float clipboard[4] = {1, 1, 1, 1};

                    auto CustomButton = [&](const char* label, float w, ImU32 bgCol1) -> bool
                    {
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        float h = 20.0f;

                        ImGuiID animId = ImGui::GetID(label);
                        float* anim = ImGui::GetStateStorage()->GetFloatRef(animId, 0.0f);


                        ImGui::PushID(label);
                        bool clicked = ImGui::InvisibleButton(xorstr_("##Btn"), ImVec2(w, h));
                        bool hovered = ImGui::IsItemHovered();
                        float dt = ImGui::GetIO().DeltaTime;
                        float target = hovered ? 1.0f : 0.0f;
                        *anim += (target - *anim) * dt * 10.0f;

                        ImGui::PopID();

                        winDl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), bgCol1, 4.0f);

                        ImVec2 txtSz = ImGui::CalcTextSize(label);
                        ImVec2 txtPos = ImVec2(p.x + (w - txtSz.x) * 0.5f, p.y + (h - txtSz.y) * 0.5f);
                        winDl->AddText(txtPos, colors.font.GetU32WithAlpha(0.5f + (0.5f * (*anim))), label);

                        return clicked;
                    };
                    ImU32 bgCol1 = colors.secondaryBackground.GetU32();

                    if (CustomButton(xorstr_("Copy"), btnWidth, bgCol1))
                    {
                        memcpy(clipboard, setting->color, sizeof(float) * 4);
                    }
                    ImGui::SameLine();

                    if (CustomButton(xorstr_("Paste"), btnWidth,
                                     RGBA(ImGui::ColorConvertFloat4ToU32(
                                         ImVec4(clipboard[0], clipboard[1], clipboard[2], clipboard[3]))).WithAlpha(
                                         255.f * 0.6f).ToU32()))
                    {
                        memcpy(setting->color, clipboard, sizeof(float) * 4);
                        setting->UpdateHSB();
                    }
                    ImGui::SameLine();
                    if (setting->rainbow)
                    {
                        bgCol1 = setting->GetU32WithAlpha(0.6f);
                    }
                    if (CustomButton(xorstr_("Rainbow"), btnWidth, bgCol1))
                    {
                        setting->rainbow = !setting->rainbow;
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::End();
        }
    }

    static void OnKeyEvent(KeyEvent* event)
    {
        if (BindingSetting && BindingSetting->listening && event->type == Pressed)
        {
            event->cancel = true;
            if (event->key == VK_ESCAPE && !event->mouse)
            {
                BindingSetting->bind.mouse = false;
                BindingSetting->bind.key = -1;
                BindingSetting->listening = false;
                BindingSetting = nullptr;
                return;
            }
            BindingSetting->bind.mouse = event->mouse;
            BindingSetting->bind.key = event->key;
            BindingSetting->listening = false;
            BindingSetting = nullptr;
        }
    }
};

static std::shared_ptr<Module> BindingModule = nullptr;
static bool ToggleHovered = false;
static bool HoleHovered = false;

class ModuleRenderer
{
public:
    static float Render(DX12DrawContext& context, std::shared_ptr<Module> mod, float x, float y, float w,
                        const ClientColors& colors)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl->_ClipRectStack.Size <= 0)
        {
            return 0.0f;
        }
        float dt = ImGui::GetIO().DeltaTime;
        float baseH = 40.0f;
        float interval = 5.0f;
        float bottomH = 5.0f;

        float targetH = baseH;
        if (mod->expanded->value)
        {
            for (auto& s : mod->settings)
            {
                if (!s.get()->IsVisible()) continue;

                if (s->type == SettingType::Mode)
                {
                    if (((ModeSetting*)s.get())->expanded)
                    {
                        targetH += 24.f + ((ModeSetting*)s.get())->modes.size() * 24.f + 6.0f;
                    }
                    else
                    {
                        targetH += 24.f + 4.0f;
                    }
                }
                else if (s->type == SettingType::Color)
                {
                    targetH += 15.0f * 2;
                }
                else if (s->type == SettingType::String)
                {
                    targetH += 15.0f * 2;
                }
                else if (s->type == SettingType::Boolean)
                {
                    targetH += 15.0f * 2;
                }
                else
                {
                    targetH += 15.0f * 2;
                }
                targetH += interval;
            }
            targetH += bottomH;
            mod->height->value = targetH;
        }

        mod->heightAnim.Update(targetH, dt, 15.0f);

        if (std::abs(targetH - mod->heightAnim.value) < 0.5f)
        {
            mod->heightAnim.value = targetH;
        }

        float curH = mod->heightAnim.value;

        mod->toggleAnim.Update(mod->IsEnabled() ? 1.0f : 0.0f, dt, 10.0f);
        float anim = mod->toggleAnim.value;

        ImU32 cOff = colors.brighterBackground.GetU32WithAlpha(0.8f);
        ImU32 cOn = colors.secondaryBackground.GetU32();

        ImVec4 v1 = ImGui::ColorConvertU32ToFloat4(cOff);
        ImVec4 v2 = ImGui::ColorConvertU32ToFloat4(cOn);
        ImVec4 vCur = ImVec4(v1.x + (v2.x - v1.x) * 0, v1.y + (v2.y - v1.y) * 0, v1.z + (v2.z - v1.z) * 0,
                             v1.w + (v2.w - v1.w) * 0);

        RGBA bgRGBA = RGBA((int)(vCur.x * 255), (int)(vCur.y * 255), (int)(vCur.z * 255), (int)(vCur.w * 255));

        ImVec2 clip1 = dl->GetClipRectMin();
        ImVec2 clip2 = dl->GetClipRectMax();

        Shader2DUtil::Get().StartScissor(clip1.x, clip1.y, clip2.x, clip2.y);

        Shader2DUtil::Get().OnDrawRoundedRect(context, x, y, w, curH, 6.0f, RGBA4C(bgRGBA));

        if (anim > 0.01f)
        {
            Shader2DUtil::Get().OnDrawRoundGlow(context, x, y, w, curH, 6.0f, 5.0f * anim, 0.4f * anim,
                                                RGBA4C(bgRGBA));
        }
        float scX = std::max(clip1.x, x);
        float scY = std::max(clip1.y, y);

        float scR = std::min(clip2.x, x + w);
        float scB = std::min(clip2.y, y + curH);

        float scW = std::max(0.0f, scR - scX);
        float scH = std::max(0.0f, scB - scY);

        Shader2DUtil::Get().StartScissorSize(scX, scY, scW, scH);

        bool binding = BindingModule == mod;


        if (binding)
        {
            std::string bindTxt = xorstr_("Bind - ") + mod->bind->bind.ToString();
            dl->AddText(ImGui::GetFont(), 20.0f, ImVec2(x + 8.0f, y + 10.0f), colors.font.GetU32(), bindTxt.c_str());

            bool isHold = mod->bind->bind.hold;
            const char* txtToggle = xorstr_("Toggle");
            const char* txtSep = xorstr_(" / ");
            const char* txtHold = xorstr_("Hold");

            ImFont* font = ImGui::GetFont();
            float fontSize = 20.0f;

            float wToggle = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, txtToggle).x;
            float wSep = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, txtSep).x;
            float wHold = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, txtHold).x;

            float totalWidth = wToggle + wSep + wHold;

            float cursorX = x + w - totalWidth - 15.0f;
            float cursorY = y + 10.0f;
            ImVec2 mousePos = ImGui::GetIO().MousePos;

            float toggleEndX = cursorX + wToggle;
            float holdStartX = toggleEndX + wSep;
            float holdEndX = holdStartX + wHold;

            ToggleHovered = (mousePos.x >= cursorX && mousePos.x <= toggleEndX);
            HoleHovered = (mousePos.x >= holdStartX && mousePos.x <= holdEndX);

            ImU32 colActive = colors.font.GetU32();
            ImU32 colDim = RGBA(colors.font.GetU32()).WithAlpha(127.f).ToU32();
            ImU32 colSep = colors.font.GetU32();

            ImU32 colorToggle = (!isHold) ? colActive : colDim;
            ImU32 colorHold = (isHold) ? colActive : colDim;


            dl->AddText(font, fontSize, ImVec2(cursorX, cursorY), colorToggle, txtToggle);
            cursorX += wToggle;

            dl->AddText(font, fontSize, ImVec2(cursorX, cursorY), colSep, txtSep);
            cursorX += wSep;

            dl->AddText(font, fontSize, ImVec2(cursorX, cursorY), colorHold, txtHold);
        }
        else
        {
            dl->AddText(ImVec2(x + 8, y + 10), RGBA(colors.font.GetU32()).WithAlpha(127.5f + (127.5f * anim)).ToU32(),
                        20, mod->name.c_str());

            if (mod->bind->bind.key != -1)
            {
                std::string bindStr = mod->bind->bind.ToString();

                float fontSize = 20.0f;
                ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, bindStr.c_str());

                float bindX = x + w - textSize.x - 15.0f;


                float bindY = y + 10.0f;

                float textAlpha = 127.5f + (127.5f * anim);
                ImU32 bindColor = RGBA(colors.font.GetU32()).WithAlpha(textAlpha).ToU32();

                dl->AddText(ImGui::GetFont(), fontSize, ImVec2(bindX, bindY), bindColor, bindStr.c_str());
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(mod.get());
        ImGui::InvisibleButton(xorstr_("##H"), ImVec2(w, baseH));
        if (binding)
        {
        }
        else
        {
            if (ImGui::IsItemClicked(0)) mod->Toggle();
            if (ImGui::IsItemClicked(1) && mod->CanExpanded()) mod->expanded->value = !mod->expanded->value;
            if (ImGui::IsItemClicked(2)) BindingModule = mod;
        }

        ImGui::PopID();

        if (curH > baseH + 1.0f)
        {
            dl->PushClipRect(ImVec2(x, y + baseH), ImVec2(x + w, y + curH), true);
            float sy = y + baseH + 5;
            float sx = x + 8;
            float sw = w - 16;

            for (auto& s : mod->settings)
            {
                if (!s.get()->IsVisible()) continue;

                float itemH = 0.0f;
                switch (s->type)
                {
                case SettingType::BooleanGroup:
                    WidgetRenderer::RenderBooleanGroup(sx, sy, sw, (BooleanGroupSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                case SettingType::Group:
                    WidgetRenderer::RenderGroup(sx, sy, sw, (GroupSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                case SettingType::Boolean:
                    WidgetRenderer::RenderCheckbox(sx, sy, sw, (BooleanSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                case SettingType::Number:
                    WidgetRenderer::RenderSlider(sx, sy, sw, (NumberSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                case SettingType::Mode:
                    WidgetRenderer::RenderMode(sx, sy, sw, (ModeSetting*)s.get(), colors);
                    itemH = ((ModeSetting*)s.get())->heightAnim.value + 7.0f;
                    break;
                case SettingType::Color:
                    WidgetRenderer::RenderColor(context, sx, sy, sw, (ColorSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                case SettingType::String:
                    WidgetRenderer::RenderString(sx, sy, sw, (StringSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                case SettingType::Bind:
                    WidgetRenderer::RenderBind(sx, sy, sw, (BindSetting*)s.get(), colors);
                    itemH = 15.0f * 2;
                    break;
                }
                if (s->group != nullptr && s->group->expand)
                {
    
                    float lineWidth = 2.0f;
                    ImU32 lineColor = colors.font.GetU32WithAlpha(0.6f); 
                    
                    float lineHeight = itemH + interval; 
                    
                    float centerY = sy + (itemH / 2.0f) - interval;
                    
                    float lineTop = centerY - (lineHeight / 2.0f);
                    float lineBottom = centerY + (lineHeight / 2.0f);
                    
                    float lineX = sx - 8.0f + (lineWidth / 3.0f); 
                    
                    dl->AddRectFilled(
                        ImVec2(lineX, lineTop), 
                        ImVec2(lineX + lineWidth, lineBottom), 
                        lineColor
                    );
                }
                sy += itemH + interval;
            }
            dl->PopClipRect();
        }
        Shader2DUtil::Get().StopScissor();
        return curH;
    }

    static void OnKeyEvent(KeyEvent* event)
    {
        if (BindingModule && event->type == Pressed)
        {
            event->cancel = true;
            if (event->key == 0 && event->mouse && (HoleHovered || ToggleHovered))
            {
                if (ToggleHovered) BindingModule->bind->bind.hold = false;
                if (HoleHovered) BindingModule->bind->bind.hold = true;

                return;
            }
            if (event->key == VK_ESCAPE && !event->mouse)
            {
                BindingModule->bind->bind.mouse = false;
                BindingModule->bind->bind.key = -1;
                BindingModule = nullptr;
                return;
            }
            BindingModule->bind->bind.mouse = event->mouse;
            BindingModule->bind->bind.key = event->key;
            BindingModule = nullptr;
        }
    }
};
