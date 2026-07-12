#include "pch.h"
#include "FontManager.h"
#include "FontManager.h"


#include "../../Utils/RawData/rawData_icons_font.h"
#include "../../Utils/RawData/rawData_icons_font_2.h"
#include "../../Utils/RawData/rawData_sf_bold_font.h"

FontManager& FontManager::Get()
{
    static FontManager instance;
    return instance;
}

void FontManager::Initialize(ImGuiIO& io)
{
    VMProtectBeginVirtualization("Initializing Font");

    m_MainFont = io.Fonts->AddFontFromFileTTF(xorstr_("c:\\windows\\fonts\\msyh.ttc"), 18.0f, NULL,
                                              io.Fonts->GetGlyphRangesChineseFull());

    if (m_MainFont == nullptr)
    {
        m_MainFont = io.Fonts->AddFontDefault();
    }
    m_MainBoldFont = io.Fonts->AddFontFromFileTTF(xorstr_("c:\\windows\\fonts\\msyhbd.ttc"), 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    static const ImWchar icons_ranges[] = {0x0020, 0x00FF, 0};

    ImFontConfig icons_config;
    icons_config.FontDataOwnedByAtlas = false;

    m_IconFont = io.Fonts->AddFontFromMemoryTTF(
        rawData_icons_font,
        rawData_icons_font_len,
        18.0f,
        &icons_config,
        icons_ranges
    );

    m_IconFont2 = io.Fonts->AddFontFromMemoryTTF(
        rawData_icons_font_2,
        rawData_icons_font_2_len,
        18.0f,
        &icons_config,
        icons_ranges
    );


    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;

    m_sf_BoldFont = io.Fonts->AddFontFromMemoryTTF(
        rawData_sf_bold_font,
        rawData_sf_bold_font_len,
        18.0f,
        &font_cfg
    );
    VMProtectEnd();
}

static void RotateDrawListVertices(ImDrawList* dl, int vtx_start, int vtx_end, const ImVec2& center, float angle_rad)
{
    const float s = std::sinf(angle_rad);
    const float c = std::cosf(angle_rad);

    for (int i = vtx_start; i < vtx_end; i++)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        
        float x = v.pos.x - center.x;
        float y = v.pos.y - center.y;
        
        float rx = x * c - y * s;
        float ry = x * s + y * c;
        
        v.pos.x = rx + center.x;
        v.pos.y = ry + center.y;
    }
}

void FontManager::DrawIconRotated(ImDrawList* dl, ImVec2 center, const char* icon, float fontSize, float angleRadians, ImU32 color)
{
    if (!m_IconFont2 || !dl || !icon) return;
    
    ImVec2 textSize = m_IconFont2->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, icon);
    ImVec2 renderPos(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f);
    
    const int vtx_start = dl->VtxBuffer.Size;
    
    dl->AddText(m_IconFont2, fontSize, renderPos, color, icon);
    
    const int vtx_end = dl->VtxBuffer.Size;
    
    RotateDrawListVertices(dl, vtx_start, vtx_end, center, angleRadians);
}

void FontManager::DrawIconCentered(ImDrawList* dl, ImVec2 centerPos, const char* icon, float fontSize, ImU32 color)
{
    if (!m_IconFont) return;

    ImVec2 textSize = m_IconFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, icon);

    ImVec2 renderPos;
    renderPos.x = centerPos.x - textSize.x * 0.5f;
    renderPos.y = centerPos.y - textSize.y * 0.5f;

    dl->AddText(m_IconFont, fontSize, renderPos, color, icon);
}

void FontManager::DrawIconInRect(ImDrawList* dl, ImVec2 rectMin, ImVec2 rectMax, const char* icon, float fontSize,
                                 ImU32 color)
{
    float width = rectMax.x - rectMin.x;
    float height = rectMax.y - rectMin.y;

    ImVec2 center(rectMin.x + width * 0.5f, rectMin.y + height * 0.5f);
    DrawIconCentered(dl, center, icon, fontSize, color);
}

void FontManager::DrawTextW(ImDrawList* dl, ImVec2 pos, const char* text, float fontSize, ImU32 color)
{
    if (m_MainFont)
    {
        dl->AddText(m_MainFont, fontSize, pos, color, text);
    }
    else
    {
        dl->AddText(pos, color, text);
    }
}
