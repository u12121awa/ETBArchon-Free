#pragma once

#include <cmath>
#include <algorithm>

enum class AnimMode
{
    Thunder,
    Mio,
    My,
    Old,
    Normal,
    None
};

class AnimateUtil
{
public:
    static float animate(float current, float endPoint, float speed, AnimMode mode, float deltaTime, float fps)
    {
        switch (mode)
        {
        case AnimMode::Mio: return mio(current, endPoint, speed, deltaTime, fps);
        case AnimMode::Thunder: return thunder(current, endPoint, speed, deltaTime, fps);
        case AnimMode::My: return my(current, endPoint, speed, deltaTime, fps);
        case AnimMode::Old: return old(current, endPoint, speed, deltaTime, fps);
        case AnimMode::Normal: return normal(current, endPoint, speed, deltaTime, fps);
        case AnimMode::None:
        default: return endPoint;
        }
    }
    
    static float animate(float current, float endPoint, float speed, float deltaTime, float fps)
    {
        if (speed >= 1.0f)
        {
            return endPoint;
        }
        return speed == 0.0f ? current : thunder(current, endPoint, speed, deltaTime, fps);
    }

private:
    static float mio(float current, float endPoint, float speed, float deltaTime, float fps)
    {
        float diff = endPoint - current;
        if (std::abs(diff) < 0.001f)
        {
            return endPoint;
        }
        speed /= 10.0f;
        int negative = speed < 0.0f ? -1 : 1;
        if (negative == -1)
        {
            speed *= -1.0f;
        }
        
        float factor = diff * (deltaTime * 50.0f) / (1.0f / speed * (std::min(240.0f, fps) / 240.0f));

        if (diff < 0.0f && factor < diff)
        {
            factor = diff;
        }
        else if (diff > 0.0f && factor >= diff)
        {
            factor = diff;
        }

        return current + factor * (float)negative;
    }

    static float old(float current, float endPoint, float speed, float deltaTime, float fps)
    {
        float diff = endPoint - current;
        if (std::abs(diff) < 0.001f)
        {
            return endPoint;
        }

        int negative = speed < 0.0f ? -1 : 1;
        if (negative == -1)
        {
            speed *= -1.0f;
        }

        float factor = diff * speed;

        if (diff < 0.0f && factor < diff)
        {
            factor = diff;
        }
        else if (diff > 0.0f && factor >= diff)
        {
            factor = diff;
        }

        return current + factor * (float)negative;
    }

    static float my(float current, float endPoint, float speed, float deltaTime, float fps)
    {
        float diff = endPoint - current;
        if (std::abs(diff) < 0.001f)
        {
            return endPoint;
        }

        int negative = speed < 0.0f ? -1 : 1;
        if (negative == -1)
        {
            speed *= -1.0f;
        }
        
        float factor = diff * deltaTime * speed * 10.0f;

        if (diff < 0.0f && factor < diff)
        {
            factor = diff;
        }
        else if (diff > 0.0f && factor >= diff)
        {
            factor = diff;
        }

        return current + factor * (float)negative;
    }

    static float thunder(float current, float endPoint, float speed, float deltaTime, float fps)
    {
        float diff = endPoint - current;
        
        if (std::abs(diff) <= 0.1f)
        {
            return endPoint;
        }
        
        float factor = diff * speed * (deltaTime * 10);
        
        return current + factor;
    }

    static float normal(float current, float endPoint, float speed, float deltaTime, float fps)
    {
        bool shouldContinue = endPoint > current;
        speed *= 10.0f;
        
        float diff = std::abs(endPoint - current);
        return diff <= speed ? endPoint : current + (shouldContinue ? speed : -speed);
    }
};
