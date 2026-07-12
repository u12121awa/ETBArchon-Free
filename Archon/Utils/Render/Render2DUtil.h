#pragma once
#include <algorithm>

#include "../../imgui/imgui.h"


class MathUtil
{
public:
    static float Clamp(float val, float min, float max) { return (val < min) ? min : (val > max) ? max : val; }
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
};


struct RGBA
{
    DWORD R;
    DWORD G;
    DWORD B;
    DWORD A;

    RGBA() : R(0), G(0), B(0), A(255)
    {
    }

    RGBA(DWORD r, DWORD g, DWORD b, DWORD a = 255)
        : R(r), G(g), B(b), A(a)
    {
    }

    RGBA(ImColor ImColor)
        : R((DWORD)(ImColor.Value.x * 255.f)), G((DWORD)(ImColor.Value.y * 255.f)), B((DWORD)(ImColor.Value.z * 255.f)),
          A((DWORD)(ImColor.Value.w * 255.f))
    {
    }

    RGBA(float r, float g, float b, float a = 255)
        : R((DWORD)r), G((DWORD)g), B((DWORD)b), A((DWORD)a)
    {
    }

    RGBA(int r, int g, int b, int a = 255)
        : R((DWORD)r), G((DWORD)g), B((DWORD)b), A((DWORD)a)
    {
    }

    
    RGBA(unsigned r, unsigned g, unsigned b, unsigned a)
        : R((DWORD)r), G((DWORD)g), B((DWORD)b), A((DWORD)a)
    {
    }

    RGBA(float col[4])
        : R((DWORD)col[0]), G((DWORD)col[1]), B((DWORD)col[2]), A((DWORD)col[3])
    {
    }

    static RGBA WHITE()
    {
        return RGBA(255, 255, 255);
    }

    static RGBA RED()
    {
        return RGBA(255, 0, 0);
    }

    static RGBA BLACK()
    {
        return RGBA(0, 0, 0);
    }

    static RGBA YELLOW()
    {
        return RGBA(255, 255, 0);
    }

    static RGBA GRAY()
    {
        return RGBA(128, 128, 128);
    }

    static RGBA CYAN()
    {
        return RGBA(0, 255, 255);
    }


    ImU32 ToU32() const
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4((float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f,
                                                     (float)A / 255.0f));
    }

    int IR() { return (int)R; }
    int IG() { return (int)G; }
    int IB() { return (int)B; }
    int IA() { return (int)A; }

    float FR() { return (float)R; }
    float FG() { return (float)G; }
    float FB() { return (float)B; }
    float FA() { return (float)A; }

    RGBA WithAlpha(float alpha) const { return RGBA((float)R, (float)G, (float)B, MathUtil::Clamp(alpha, 0.f, 255.f)); }

    static RGBA Lerp(RGBA a, RGBA b, float t)
    {
        return RGBA(
            MathUtil::Lerp(a.R, b.R, t),
            MathUtil::Lerp(a.G, b.G, t),
            MathUtil::Lerp(a.B, b.B, t),
            MathUtil::Lerp(a.A, b.A, t)
        );
    }
};

struct RGBA4C
{
    RGBA C1;
    RGBA C2;
    RGBA C3;
    RGBA C4;

    RGBA4C(RGBA c1, RGBA c2, RGBA c3, RGBA c4)
        : C1(c1), C2(c2), C3(c3), C4(c4)
    {
    }

    RGBA4C(RGBA c)
        : C1(c), C2(c), C3(c), C4(c)
    {
    }

    RGBA4C(ImColor ImColor)
        : C1(RGBA(ImColor)), C2(RGBA(ImColor)), C3(RGBA(ImColor)), C4(RGBA(ImColor))
    {
    }
};

namespace Render2DUtil
{

    inline double Interpolate(double oldValue, double newValue, double interpolationValue)
    {
        return oldValue + (newValue - oldValue) * interpolationValue;
    }

    inline int InterpolateInt(int oldValue, int newValue, double interpolationValue)
    {
        return static_cast<int>(
            Interpolate(
                oldValue,
                newValue,
                interpolationValue
            )
        );
    }
    
    inline float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    inline RGBA InterpolateColorC(const RGBA& c1, const RGBA& c2, float amount)
    {
        amount = Clamp01(amount);

        return RGBA(
            InterpolateInt(c1.R, c2.R, amount),
            InterpolateInt(c1.G, c2.G, amount),
            InterpolateInt(c1.B, c2.B, amount),
            InterpolateInt(c1.A, c2.A, amount)
        );
    }


    inline long long CurrentTimeMillis()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count();
    }

    inline RGBA TwoColorEffect(const RGBA& c1,
                               const RGBA& c2,
                               double speed,
                               double count)
    {
        double time = static_cast<double>(CurrentTimeMillis());

        int angle = static_cast<int>(
            (time / speed + count)
        ) % 360;
        
        angle = (angle >= 180 ? 360 - angle : angle) * 2;

        float t = static_cast<float>(angle) / 360.0f;

        return InterpolateColorC(c1, c2, t);
    }

    
    inline ImColor Astolfo(float yDist, float yTotal, float saturation, float speedt) 
    {
        float speed = 1800.0f;
        
        float time = (float)(GetTickCount64() % (int)speed);
        
        float hue = time + (yTotal - yDist) * speedt;
        
        while (hue > speed) {
            hue -= speed;
        }
        
        hue /= speed;
        
        if (hue > 0.5f) {
            hue = 0.5f - (hue - 0.5f);
        }
        
        hue += 0.5f;
        
        return ImColor::HSV(hue, saturation, 1.0f);
    }
    
    inline ImU32 ToImColor(const RGBA& c)
    {
        return IM_COL32(c.R, c.G, c.B, c.A);
    }

    inline void RectMultiColor(ImDrawList* dl,
                               float x1, float y1, float x2, float y2,
                               const RGBA& cTL, const RGBA& cTR,
                               const RGBA& cBR, const RGBA& cBL)
    {
        dl->AddRectFilledMultiColor(
            ImVec2(x1, y1), ImVec2(x2, y2),
            ToImColor(cTL), ToImColor(cTR),
            ToImColor(cBR), ToImColor(cBL)
        );
    }

    inline void RectMultiColor(ImDrawList* dl,
                               float x1, float y1, float x2, float y2,
                               const RGBA4C& rgba4)
    {
        RectMultiColor(dl, x1, y1, x2, y2, rgba4.C1, rgba4.C2, rgba4.C3, rgba4.C4);
    }


    inline RGBA Rainbow(int speed, int index, float saturation, float brightness, float opacity)
    {
        using namespace std::chrono;

        auto now_ms = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count();


        int angle = static_cast<int>((now_ms / speed + index) % 360);
        float hue = angle / 360.0f;

        float fr, fg, fb;
        ImGui::ColorConvertHSVtoRGB(hue, saturation, brightness, fr, fg, fb);

        float a = opacity * 255.0f;
        a = std::clamp(a, 0.f, 255.f);

        return RGBA(fr * 255, fg * 255, fb * 255, a);
    }

    inline RGBA SkyRainbow(int speed, int index,
                           float saturation = 0.5f,
                           float brightness = 1.0f,
                           float opacity = 1.0f)
    {
        using namespace std::chrono;

        if (speed == 0) speed = 1;

        auto now_ms = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count();

        long long angleRaw = (now_ms / speed) + static_cast<long long>(index);
        int angle = static_cast<int>(angleRaw % 360);
        if (angle < 0) angle += 360;

        float base = angle / 360.0f;

        float hueEffective = (base < 0.5f) ? (1.0f - base) : base;

        float fr, fg, fb;
        ImGui::ColorConvertHSVtoRGB(hueEffective, saturation, brightness, fr, fg, fb);
        

        float a = opacity * 255.0f;
        a = std::clamp(a, 0.f, 255.f);

        return RGBA(fr * 255, fg * 255, fb * 255, a);
    }

    inline RGBA GetColor(int count)
    {
        return SkyRainbow(18, count);
    }


    inline void DrawHudRectSky(ImDrawList* dl,
                               float posX, float posY,
                               float endPosX, float endPosY,
                               float thickness = 2.0f)
    {
        float x1 = floorf(posX);
        float y1 = floorf(posY);
        float x2 = floorf(endPosX);
        float y2 = floorf(endPosY);


        float t = floorf(thickness < 1.0f ? 1.0f : thickness);

        RGBA c0 = GetColor(0);
        RGBA c90 = GetColor(90);
        RGBA c180 = GetColor(180);
        RGBA c270 = GetColor(270);


        RectMultiColor(dl, x1, y1, x2, y1 + t, c180, c90, c90, c180);


        RectMultiColor(dl, x1, y2 - t, x2, y2, c0, c180, c180, c0);


        RectMultiColor(dl, x1, y1 + t, x1 + t, y2 - t, c270, c0, c0, c270);


        RectMultiColor(dl, x2 - t, y1 + t, x2, y2 - t, c90, c270, c270, c90);
    }


    inline void DrawHudRect(ImDrawList* dl, float posX, float posY,
                            float endPosX, float endPosY,
                            RGBA color, float thickness = 1.5f)
    {
        float x1 = floorf(posX);
        float y1 = floorf(posY);
        float x2 = floorf(endPosX);
        float y2 = floorf(endPosY);


        float t = floorf(thickness < 1.0f ? 1.0f : thickness);

        RGBA c0 = color;
        RGBA c90 = color;
        RGBA c180 = color;
        RGBA c270 = color;


        RectMultiColor(dl, x1, y1, x2, y1 + t, c180, c90, c90, c180);


        RectMultiColor(dl, x1, y2 - t, x2, y2, c0, c180, c180, c0);


        RectMultiColor(dl, x1, y1 + t, x1 + t, y2 - t, c270, c0, c0, c270);


        RectMultiColor(dl, x2 - t, y1 + t, x2, y2 - t, c90, c270, c270, c90);
    }
}
