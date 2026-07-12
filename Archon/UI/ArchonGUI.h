#pragma once

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../Utils/Render/DX12DrawContext.h"
#include "../Utils/Render/Render2DUtil.h"
#include "../Utils/Render/Shader2DUtil.h"
#include <vector>
#include <string>
#include <cmath>
#include <memory>

#include "../Core/Managers/ModuleManager.h"
#include "../modules/ModuleRenderer.h"
#include "../modules/impl/ClickGUI.h"
#include "../Utils/Render/TextureUtil.h"

class ArchonCategory
{
public:
    virtual ~ArchonCategory() = default;
    virtual std::string GetName() const = 0;
    virtual void Render(DX12DrawContext& context, const ClientColors& colors) = 0;
};

class ModulesCategory : public ArchonCategory
{
    std::string categoryName;
    ScrollAnimation m_scroll;

public:
    ModulesCategory(std::string name) : categoryName(name)
    {
    }

    std::string GetName() const override { return categoryName; }

    void Render(DX12DrawContext& context, const ClientColors& colors) override
    {
        auto modules = ModuleManager::Get().GetModulesByCategory(categoryName);

        float totalHeight = ImGui::GetWindowHeight();
        float cursorY = ImGui::GetCursorPosY();

        float maxScroll = ImGui::GetScrollMaxY();

        float dt = ImGui::GetIO().DeltaTime;
        m_scroll.HandleInput(maxScroll, 40.0f);
        m_scroll.Update(dt);

        ImGui::SetScrollY(m_scroll.current);

        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float fullWidth = ImGui::GetContentRegionAvail().x;

        float sidePadding = 50.0f;
        float topPadding = 15.0f;
        float colSpacing = 25.0f;
        float rowSpacing = 10.0f;
        int cols = 2;

        float availableWidth = fullWidth - (sidePadding * 2.0f);
        float colW = (availableWidth - (cols - 1) * colSpacing) / cols;
        std::vector<float> colY(cols, topPadding);

        for (int i = 0; i < modules.size(); ++i)
        {
            int c = i % cols;
            float x = startPos.x + sidePadding + c * (colW + colSpacing);
            float y = startPos.y + colY[c];

            float h = ModuleRenderer::Render(context, modules[i], x, y, colW, colors);
            colY[c] += h + rowSpacing;
        }

        float maxH = 0;
        for (float h : colY) if (h > maxH) maxH = h;
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::Dummy(ImVec2(fullWidth, maxH + 5.0f));
    }
};

class MapLoadCategory : public ArchonCategory
{
public:
    std::string GetName() const override { return xorstr_("MapLoad"); }

    void Render(DX12DrawContext& context, const ClientColors& colors) override
    {
        const char* text = (const char*)u8"Free版本没有";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float offsetX = (availSize.x - textSize.x) * 0.5f;
        float offsetY = (availSize.y - textSize.y) * 0.5f;
        if (offsetX < 0) offsetX = 0;
        if (offsetY < 0) offsetY = 0;
        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursorPos.x + offsetX, cursorPos.y + offsetY));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", text);
    }
};

class AntiCheatCategory : public ArchonCategory
{
public:
    std::string GetName() const override { return xorstr_("AntiCheat"); }

    void Render(DX12DrawContext& context, const ClientColors& colors) override
    {
        const char* text = (const char*)u8"Free版本没有";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float offsetX = (availSize.x - textSize.x) * 0.5f;
        float offsetY = (availSize.y - textSize.y) * 0.5f;
        if (offsetX < 0) offsetX = 0;
        if (offsetY < 0) offsetY = 0;
        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursorPos.x + offsetX, cursorPos.y + offsetY));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", text);
    }
};

class ArchonGUI
{
public:
    TextureData* m_LogoTexture;
    
    ArchonGUI()
         : m_blurRadius(5.0f), m_blurStrength(15.0f), m_blurOpacity(0.55f),
           m_glowStrength(8.0f), m_glowAlpha(0.6f),
           m_isOpen(false),
           m_sidebarWidth(100.0f),
           m_activeTab(0),
           m_showAboutWindow(false),
           m_currentThemeIdx(0),
           m_needResetPos(true)
    {
        m_LogoTexture = nullptr;
        m_mainGUIPosition = ImVec2(0, 0);
        m_mainGUISize = ImVec2(0, 0);
        m_aboutWindowPos = ImVec2(0, 0);
        
        auto clickGui = ModuleManager::Get().GetModule<ClickGUI>();
        if (clickGui)
        {
            m_currentThemeIdx = (int) clickGui->currentTheme->value;
            m_blurRadius = clickGui->blurRadius->value;
            m_blurStrength = clickGui->blurStrength->value;
            m_blurOpacity = clickGui->blurOpacity->value;
            m_glowStrength = clickGui->glowStrength->value;
            m_glowAlpha = clickGui->glowAlpha->value;
        }
        m_categories.push_back(std::make_shared<ModulesCategory>(xorstr_("Modules")));
        m_categories.push_back(std::make_shared<MapLoadCategory>());
        m_categories.push_back(std::make_shared<AntiCheatCategory>());

        m_tabTextAnims.resize(m_categories.size());
        for (auto& anim : m_tabTextAnims)
        {
            anim.Init(ImColor(128, 128, 128, 128));
        }

#define COL_RGB(r, g, b) ImColor(r, g, b, 255)
        m_primaryThemes.push_back({
            "Dark", COL_RGB(22, 25, 28), COL_RGB(20, 24, 26), COL_RGB(8, 10, 12), COL_RGB(30, 32, 34),
            COL_RGB(255, 255, 255)
        });
        m_primaryThemes.push_back({
            "Light", COL_RGB(230, 230, 230), COL_RGB(230, 230, 230), COL_RGB(150, 150, 150), COL_RGB(255, 255, 255),
            COL_RGB(0, 0, 0)
        });
        m_primaryThemes.push_back({
            "Blue", COL_RGB(27, 28, 44), COL_RGB(30, 31, 48), COL_RGB(24, 25, 38), COL_RGB(30, 35, 48),
            COL_RGB(255, 255, 255)
        });

        if (!m_primaryThemes.empty())
        {
            m_clientColors.InitAll(m_primaryThemes[m_currentThemeIdx]);
        }
    }

    void SetOpen(bool open)
    {

        m_isOpen = open;
        if (m_isOpen) m_needResetPos = true;
        ImGuiIO& io = ImGui::GetIO();

           
        auto PC = Instances::GetPlayerController();
        if (PC)
        {
            if (m_isOpen && PC->bShowMouseCursor == false)
            {
                io.MouseDrawCursor = true;
            }
            else if (!m_isOpen)
            {
                io.MouseDrawCursor = false;
            }
        }
    }
    

    void Toggle()
    {
        SetOpen(!m_isOpen);
    }

    bool IsOpen() const { return m_isOpen; }

    void DrawMainGUI(DX12DrawContext& context)
    {
        if (!m_isOpen) return;

        float dt = ImGui::GetIO().DeltaTime;
        if (m_currentThemeIdx < m_primaryThemes.size())
        {
            m_clientColors.UpdateAll(m_primaryThemes[m_currentThemeIdx], dt);
        }

        float targetWidth = 380.0f * 2;
        float targetHeight = 237.5f * 2;
        ImGuiIO& io = ImGui::GetIO();
        float centerX = (io.DisplaySize.x - targetWidth) / 2.0f;
        float centerY = (io.DisplaySize.y - targetHeight) / 2.0f;

        ImGuiCond posCond = m_needResetPos ? ImGuiCond_Always : ImGuiCond_Appearing;
        if (m_needResetPos) m_needResetPos = false;

        ImGui::SetNextWindowPos(ImVec2(centerX, centerY), posCond);
        ImGui::SetNextWindowSize(ImVec2(targetWidth, targetHeight), ImGuiCond_Appearing);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse;

        if (ImGui::Begin(xorstr_("Archon Main GUI"), &m_isOpen, windowFlags))
        {
            m_mainGUIPosition = ImGui::GetWindowPos();
            m_mainGUISize = ImGui::GetWindowSize();
            
            ImU32 animBgColor = m_clientColors.background.GetU32();
            ImU32 animGlowColor = m_clientColors.secondaryBackground.GetU32();

            RGBA bgRGBA = RGBA(animBgColor);
            RGBA glowRGBA = RGBA(animGlowColor);
            RGBA4C themeGlowColor = RGBA4C(glowRGBA, glowRGBA, glowRGBA, glowRGBA);

            Shader2DUtil::Get().OnDrawRoundGlow(context, m_mainGUIPosition.x, m_mainGUIPosition.y, m_mainGUISize.x,
                                                m_mainGUISize.y, m_blurRadius, m_glowStrength, m_glowAlpha,
                                                themeGlowColor);
            Shader2DUtil::Get().OnDrawRoundBlur(context, m_mainGUIPosition.x, m_mainGUIPosition.y, m_mainGUISize.x,
                                                m_mainGUISize.y, m_blurRadius, bgRGBA, m_blurStrength, m_blurOpacity);

            DrawSidebar(context, m_mainGUIPosition, m_mainGUISize.y);
            context.WindowsPos = m_mainGUIPosition;
            context.WindowsSize = m_mainGUISize;

            ImGui::SetCursorPos(ImVec2(m_sidebarWidth + 0, 0));
            ImGui::BeginGroup();
            {
                ImGui::PushStyleColor(ImGuiCol_Text, m_clientColors.font.GetU32());

                if (m_activeTab >= 0 && m_activeTab < m_categories.size())
                {
                    m_categories[m_activeTab]->Render(context, m_clientColors);
                }

                ImGui::PopStyleColor();
            }
            ImGui::EndGroup();

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
            {
                ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
                if (dragDelta.x != 0 || dragDelta.y != 0)
                {
                    ImGui::SetWindowPos(ImVec2(m_mainGUIPosition.x + dragDelta.x, m_mainGUIPosition.y + dragDelta.y));
                    ImGui::ResetMouseDragDelta(0);
                }
            }
        }
        ImGui::End();

        if (m_showAboutWindow)
        {
            DrawAboutWindow(context);
        }
    }

    TextureData* GetLogoTexturePtr() { return m_LogoTexture; }

private:
    void ToggleAboutWindow()
    {
        m_showAboutWindow = !m_showAboutWindow;
        if (m_showAboutWindow)
        {
            float aboutWidth = 250.0f;
            float gap = 20.0f;
            float targetX = m_mainGUIPosition.x - aboutWidth - gap;
            float targetY = m_mainGUIPosition.y;
            m_aboutWindowPos = ImVec2(targetX, targetY);
        }
    }

    void DrawSidebar(DX12DrawContext& context, ImVec2 pos, float height)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 sidebarU32 = m_clientColors.secondaryBackground.GetU32();

        RGBA4C sidebarGradient = RGBA4C(sidebarU32);
        Shader2DUtil::Get().OnDrawRoundedRect(context, pos.x - 1, pos.y - 1, m_sidebarWidth + 2, height + 2,
                                              m_blurRadius, sidebarGradient);

        float iconTopMargin = 10.0f;
        ImVec2 logoSize = ImVec2(45, 35);
        ImVec2 logoPos = ImVec2(pos.x + (m_sidebarWidth - logoSize.x) / 2.0f, pos.y + iconTopMargin);

        ImGui::SetCursorScreenPos(ImVec2(logoPos.x - 5, logoPos.y - 5));
        if (ImGui::InvisibleButton(xorstr_("##LogoBtn"), ImVec2(logoSize.x + 10, logoSize.y + 10)))
        {
            ToggleAboutWindow();
        }
        //drawList->AddText(ImGui::GetFont(), 24.0f, logoPos, m_clientColors.font.GetU32(), logoText);
        DrawSpecificImage(context, logoPos.x, logoPos.y, logoSize.x, logoSize.y);
        float separatorY = logoPos.y + logoSize.y + 15.0f;
        drawList->AddLine(ImVec2(pos.x + 25, separatorY), ImVec2(pos.x + m_sidebarWidth - 25, separatorY),
                          RGBA::GRAY().WithAlpha(200).ToU32(), 1.0f);

        float startY = separatorY + 20.0f;
        float itemHeight = 40.0f;
        float dt = ImGui::GetIO().DeltaTime;

        float targetOffset = m_activeTab * itemHeight;
        m_tabIndicatorAnim.Update(targetOffset, dt);
        float currentAnimY = startY + m_tabIndicatorAnim.value;

        drawList->AddRectFilled(ImVec2(pos.x, currentAnimY + 5), ImVec2(pos.x + 3, currentAnimY + itemHeight - 5),
                                m_clientColors.font.GetU32());

        for (int i = 0; i < m_categories.size(); ++i)
        {
            float currentY = startY + (i * itemHeight);
            bool isSelected = (m_activeTab == i);
            ImU32 targetColor = isSelected ? m_clientColors.font.GetU32() : m_clientColors.font.GetU32WithAlpha(0.5f);
            if (i < m_tabTextAnims.size()) m_tabTextAnims[i].Update(targetColor, dt);
            ImU32 finalTextColor = m_tabTextAnims[i].GetU32();

            std::string name = m_categories[i]->GetName();
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            ImVec2 textPos = ImVec2(pos.x + (m_sidebarWidth - textSize.x) / 2.0f,
                                    currentY + (itemHeight - textSize.y) / 2.0f);
            drawList->AddText(textPos, finalTextColor, name.c_str());

            ImGui::SetCursorScreenPos(ImVec2(pos.x, currentY));
            std::string btnLabel = xorstr_("##Tab") + std::to_string(i);
            if (ImGui::InvisibleButton(btnLabel.c_str(), ImVec2(m_sidebarWidth, itemHeight))) m_activeTab = i;
        }
    }

    void CustomSlider(const char* label, float* v, float v_min, float v_max, const char* format)
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
            if (ImGui::InvisibleButton(xorstr_("##ValueBtn"), ImVec2(textSize.x > 30 ? textSize.x : 30, height))) *isEditing =
                true;
            if (ImGui::IsItemHovered())
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddLine(ImVec2(valuePos.x, valuePos.y + textSize.y),
                            ImVec2(valuePos.x + textSize.x, valuePos.y + textSize.y), m_clientColors.font.GetU32());
            }
        }

        ImGui::PopID();

        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + height + 14.0f));
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }

    void DrawAboutWindow(DX12DrawContext& context)
    {
        float width = 250.0f;
        float height = 310.0f;

        ImGui::SetNextWindowPos(m_aboutWindowPos);
        ImGui::SetNextWindowSize(ImVec2(width, height));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin(xorstr_("About Archon"), &m_showAboutWindow, flags))
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetWindowPos();

            ImU32 bgCol = m_clientColors.background.GetU32();
            ImU32 secBgCol = m_clientColors.secondaryBackground.GetU32();
            ImU32 brightBgCol = m_clientColors.brighterBackground.GetU32();
            ImU32 fontCol = m_clientColors.font.GetU32();

            RGBA secBgRGBA = RGBA(secBgCol);
            RGBA4C shadowGradient = RGBA4C(secBgRGBA);
            Shader2DUtil::Get().OnDrawRoundGlow(context, pos.x, pos.y, width, height, 5.0f, m_glowStrength, m_glowAlpha,
                                                shadowGradient);

            RGBA bgRGBA = RGBA(bgCol);
            Shader2DUtil::Get().OnDrawRoundBlur(context, pos.x, pos.y, width, height, 5.0f, bgRGBA, m_blurStrength,
                                                m_blurOpacity);

            float headerHeight = 30.0f;
            RGBA brightBgRGBA = RGBA(brightBgCol);
            RGBA4C headerGradient = RGBA4C(brightBgRGBA);

            Shader2DUtil::Get().OnDrawRoundedRect(context, pos.x, pos.y, width, headerHeight, 5.0f, headerGradient);
            drawList->AddRectFilled(ImVec2(pos.x, pos.y + headerHeight / 2.0f),
                                    ImVec2(pos.x + width, pos.y + headerHeight), brightBgCol);

            drawList->AddText(ImVec2(pos.x + 10, pos.y + 6), fontCol, xorstr_("About Archon"));

            const char* closeIcon = xorstr_("X");
            ImVec2 closeSize = ImGui::CalcTextSize(closeIcon);
            float closeBtnDim = 20.0f;
            ImVec2 closePos = ImVec2(pos.x + width - closeBtnDim - 5, pos.y + (headerHeight - closeBtnDim) / 2.0f);

            ImGui::SetCursorScreenPos(closePos);
            if (ImGui::InvisibleButton(xorstr_("##CloseAbout"), ImVec2(closeBtnDim, closeBtnDim)))
            {
                m_showAboutWindow = false;
            }
            ImU32 closeColor = ImGui::IsItemHovered() ? RGBA(ImColor(255, 100, 100)).ToU32() : fontCol;
            ImVec2 textPos = ImVec2(closePos.x + (closeBtnDim - closeSize.x) / 2.0f,
                                    closePos.y + (closeBtnDim - closeSize.y) / 2.0f);
            drawList->AddText(textPos, closeColor, closeIcon);

            ImRect headerRect(pos.x, pos.y, pos.x + width - closeBtnDim - 10, pos.y + headerHeight);
            static bool isDragging = false;
            if (ImGui::IsMouseDown(0) && ImGui::IsMouseHoveringRect(headerRect.Min, headerRect.Max) && !
                ImGui::IsItemHovered()) isDragging = true;
            if (ImGui::IsMouseReleased(0)) isDragging = false;
            if (isDragging && ImGui::IsMouseDragging(0))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                m_aboutWindowPos.x += delta.x;
                m_aboutWindowPos.y += delta.y;
            }

            float contentStartY = pos.y + headerHeight + 10.0f;
            const char* clientName = xorstr_("GUI Settings");
            ImVec2 nameSize = ImGui::CalcTextSize(clientName);
            drawList->AddText(ImVec2(pos.x + (width - nameSize.x) / 2.0f, contentStartY), fontCol, clientName);

            float sepY = contentStartY + nameSize.y + 10.0f;
            drawList->AddLine(ImVec2(pos.x, sepY), ImVec2(pos.x + width, sepY), RGBA(fontCol).WithAlpha(20).ToU32());

            float labelY = sepY + 15.0f;
            drawList->AddText(ImVec2(pos.x + 15, labelY), fontCol, xorstr_("UIStyle"));

            float btnSize = 20.0f;
            float btnSpacing = 10.0f;
            float btnStartX = pos.x + width - 20.0f - btnSize;

            for (int i = 0; i < m_primaryThemes.size(); ++i)
            {
                const auto& item = m_primaryThemes[i];
                float currentBtnX = btnStartX - (i * (btnSize + btnSpacing));
                ImVec2 btnPos(currentBtnX, labelY);

                ImGui::SetCursorScreenPos(btnPos);
                std::string btnId = xorstr_("##ThemeSel") + std::to_string(i);
                if (ImGui::InvisibleButton(btnId.c_str(), ImVec2(btnSize, btnSize))) m_currentThemeIdx = i;

                bool isHovered = ImGui::IsItemHovered();
                bool isSelected = (m_currentThemeIdx == i);

                if (isSelected || isHovered)
                {
                    ImU32 accentColor = ImColor(80, 140, 255, (int)(255 * (isSelected ? 1.0f : 0.5f)));
                    drawList->AddRectFilled(ImVec2(btnPos.x - 2, btnPos.y - 2),
                                            ImVec2(btnPos.x + btnSize + 2, btnPos.y + btnSize + 2), accentColor, 10.f);
                }
                drawList->AddRectFilled(btnPos, ImVec2(btnPos.x + btnSize, btnPos.y + btnSize),
                                        item.brighterBackgroundColor, 10.f);
            }

            float settingsY = labelY + 7;


            ImGui::SetCursorScreenPos(ImVec2(pos.x + 15, settingsY + 25.0f));
            
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            
            ImGui::PushStyleColor(ImGuiCol_Text, fontCol);

            CustomSlider(xorstr_("Radius"), &m_blurRadius, 0.0f, 20.0f, xorstr_("%.1f"));
            CustomSlider(xorstr_("Strength"), &m_blurStrength, 1.0f, 20.0f, xorstr_("%.1f"));
            CustomSlider(xorstr_("Opacity"), &m_blurOpacity, 0.0f, 1.0f, xorstr_("%.2f"));
            CustomSlider(xorstr_("Glow"), &m_glowStrength, 0.0f, 20.0f, xorstr_("%.1f"));
            CustomSlider(xorstr_("Alpha"), &m_glowAlpha, 0.0f, 1.0f, xorstr_("%.2f"));
            auto clickGui = ModuleManager::Get().GetModule<ClickGUI>();
            if (clickGui)
            {
                clickGui->currentTheme->value = (float) m_currentThemeIdx;
                clickGui->blurRadius->value = m_blurRadius;
                clickGui->blurStrength->value = m_blurStrength;
                clickGui->blurOpacity->value = m_blurOpacity;
                clickGui->glowStrength->value = m_glowStrength;
                clickGui->glowAlpha->value = m_glowAlpha;
            }
            ImGui::PopStyleColor();

            float bottomSepY = pos.y + height - 25.0f;
            drawList->AddLine(ImVec2(pos.x, bottomSepY), ImVec2(pos.x + width, bottomSepY),
                              RGBA(fontCol).WithAlpha(20).ToU32());
        }
        ImGui::End();
    }
    
    void DrawSpecificImage(DX12DrawContext context, float x, float y, float w, float h) 
    {
        RGBA c0 = Render2DUtil::GetColor(0);
        RGBA c90 = Render2DUtil::GetColor(90);
        RGBA c180 = Render2DUtil::GetColor(180);
        RGBA c270 = Render2DUtil::GetColor(270);
        
        RGBA4C R4C = RGBA4C(RGBA(28, 41, 85));
        
        
        Shader2DUtil::Get().OnDrawStencilImage(context, x, y, w, h, R4C, m_LogoTexture->gpuTextureHandle);
        /*
        ImGui::GetWindowDrawList()->AddImage(
            m_LogoTexture.SrvGpuHandle.ptr,
            ImVec2(x, y), 
            ImVec2(x + w, y + h),
            ImVec2(0, 0), 
            ImVec2(1, 1), 
            IM_COL32(255, 255, 255, 255) 
        );
        */
    }

private:
    ImVec2 m_mainGUIPosition;
    ImVec2 m_mainGUISize;
    ImVec2 m_aboutWindowPos;
    float m_blurRadius;
    float m_blurStrength;
    float m_blurOpacity;
    float m_glowStrength;
    float m_glowAlpha;

    bool m_isOpen;
    bool m_needResetPos;

    float m_sidebarWidth;
    int m_activeTab;

    std::vector<std::shared_ptr<ArchonCategory>> m_categories;

    bool m_showAboutWindow;
    int m_currentThemeIdx;
    std::vector<PrimaryTheme> m_primaryThemes;
    ClientColors m_clientColors;

    FloatAnimation m_tabIndicatorAnim;
    std::vector<ColorAnimation> m_tabTextAnims;
};
