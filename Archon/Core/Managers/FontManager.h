#pragma once

#include <string>

class FontManager
{
public:
    static FontManager& Get();
    
    void Initialize(ImGuiIO& io);
    
    ImFont* GetMainFont() const { return m_MainFont; }
    ImFont* GetMainBoldFont() const { return m_MainBoldFont; }
    ImFont* GetIconFont() const { return m_IconFont; }
    ImFont* GetIconFont2() const { return m_IconFont2; }
    ImFont* GetSFBoldFont() const { return m_sf_BoldFont; }

    void DrawIconRotated(ImDrawList* dl, ImVec2 center, const char* icon, float fontSize, float angleRadians, ImU32 color);
    void DrawIconCentered(ImDrawList* dl, ImVec2 centerPos, const char*, float fontSize, ImU32 color);
    void DrawIconInRect(ImDrawList* dl, ImVec2 rectMin, ImVec2 rectMax, const char* icon, float fontSize, ImU32 color);
    void DrawTextW(ImDrawList* dl, ImVec2 pos, const char* text, float fontSize, ImU32 color);


private:
    FontManager() = default;
    ~FontManager() = default;
    
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    ImFont* m_MainFont = nullptr;
    ImFont* m_MainBoldFont = nullptr;
    ImFont* m_IconFont = nullptr;
    ImFont* m_IconFont2 = nullptr;
    ImFont* m_sf_BoldFont = nullptr;
};