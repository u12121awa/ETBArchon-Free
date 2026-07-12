#pragma once

#include <cmath>
#include <chrono>
#include <algorithm>
#include <numbers>


enum class Easing
{
    LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC,
    EASE_IN_OUT_CUBIC,
    EASE_IN_QUART,
    EASE_OUT_QUART,
    EASE_IN_OUT_QUART,
    EASE_IN_QUINT,
    EASE_OUT_QUINT,
    EASE_IN_OUT_QUINT,
    EASE_IN_SINE,
    EASE_OUT_SINE,
    EASE_IN_OUT_SINE,
    EASE_IN_EXPO,
    EASE_OUT_EXPO,
    EASE_IN_OUT_EXPO,
    EASE_IN_CIRC,
    EASE_OUT_CIRC,
    EASE_IN_OUT_CIRC,
    SIGMOID,
    EASE_OUT_ELASTIC,
    EASE_IN_BACK
};

class Animation
{
private:
    Easing easing;
    long long duration;
    long long startTime;

    float startValue;
    float destinationValue;
    float value;
    bool finished;
    
    long long getCurrentTimeMs() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
    }
    
    double pow(double base, double exp) const { return std::pow(base, exp); }
    double sin(double rad) const { return std::sin(rad); }
    double cos(double rad) const { return std::cos(rad); }
    double sqrt(double x) const { return std::sqrt(x); }
    double exp(double x) const { return std::exp(x); }


#ifdef __cpp_lib_math_constants
    static constexpr double PI = std::numbers::pi;
#else
    static constexpr double PI = 3.14159265358979323846;
#endif
    
    double applyEasing(double x)
    {
        switch (easing)
        {
        case Easing::LINEAR: return x;
        case Easing::EASE_IN_QUAD: return x * x;
        case Easing::EASE_OUT_QUAD: return x * (2 - x);
        case Easing::EASE_IN_OUT_QUAD: return x < 0.5 ? 2 * x * x : -1 + (4 - 2 * x) * x;
        case Easing::EASE_IN_CUBIC: return x * x * x;
        case Easing::EASE_OUT_CUBIC: return (--x) * x * x + 1;
        case Easing::EASE_IN_OUT_CUBIC: return x < 0.5 ? 4 * x * x * x : (x - 1) * (2 * x - 2) * (2 * x - 2) + 1;
        case Easing::EASE_IN_QUART: return x * x * x * x;
        case Easing::EASE_OUT_QUART: return 1 - (--x) * x * x * x;
        case Easing::EASE_IN_OUT_QUART: return x < 0.5 ? 8 * x * x * x * x : 1 - 8 * (--x) * x * x * x;
        case Easing::EASE_IN_QUINT: return x * x * x * x * x;
        case Easing::EASE_OUT_QUINT: return 1 + (--x) * x * x * x * x;
        case Easing::EASE_IN_OUT_QUINT: return x < 0.5 ? 16 * x * x * x * x * x : 1 + 16 * (--x) * x * x * x * x;
        case Easing::EASE_IN_SINE: return 1 - cos(x * PI / 2);
        case Easing::EASE_OUT_SINE: return sin(x * PI / 2);
        case Easing::EASE_IN_OUT_SINE: return 1 - cos(PI * x / 2);
        case Easing::EASE_IN_EXPO: return x == 0 ? 0 : pow(2, 10 * x - 10);
        case Easing::EASE_OUT_EXPO: return x == 1 ? 1 : 1 - pow(2, -10 * x);
        case Easing::EASE_IN_OUT_EXPO: return x == 0
                                                  ? 0
                                                  : x == 1
                                                  ? 1
                                                  : x < 0.5
                                                  ? pow(2, 20 * x - 10) / 2
                                                  : (2 - pow(2, -20 * x + 10)) / 2;
        case Easing::EASE_IN_CIRC: return 1 - sqrt(1 - x * x);
        case Easing::EASE_OUT_CIRC: return sqrt(1 - (--x) * x);
        case Easing::EASE_IN_OUT_CIRC: return x < 0.5
                                                  ? (1 - sqrt(1 - 4 * x * x)) / 2
                                                  : (sqrt(1 - 4 * (x - 1) * x) + 1) / 2;
        case Easing::SIGMOID: return 1 / (1 + exp(-x));
        case Easing::EASE_OUT_ELASTIC: return x == 0
                                                  ? 0
                                                  : x == 1
                                                  ? 1
                                                  : pow(2, -10 * x) * sin((x * 10 - 0.75) * ((2 * PI) / 3)) * 0.5 + 1;
        case Easing::EASE_IN_BACK: return (1.70158 + 1) * x * x * x - 1.70158 * x * x;
        default: return x;
        }
    }

public:
    Animation(Easing easing, long long duration, float startValue)
        : easing(easing), duration(duration), startValue(startValue)
    {
        this->startTime = getCurrentTimeMs();
        this->value = startValue;
        this->destinationValue = startValue;
        this->finished = false;
    }

    void SetDuration(long long duration)
    {
        this->duration = duration;
    }
    
    Animation(Easing easing, long long duration)
        : Animation(easing, duration, 0.0f)
    {
    }
    
    void run(float newDestinationValue)
    {
        long long millis = getCurrentTimeMs();
        
        if (std::abs(newDestinationValue - this->destinationValue) > 0.0001f)
        {
            this->destinationValue = newDestinationValue;
            this->reset();
        }
        else
        {
            bool timeExpired = (millis - this->duration) > this->startTime;
            if (timeExpired)
            {
                this->finished = true;
                this->value = newDestinationValue;
                return;
            }
        }
        
        double progress = (double)(millis - this->startTime) / (double)this->duration;
        if (progress > 1.0) progress = 1.0;
        if (progress < 0.0) progress = 0.0;
        
        double result = applyEasing(progress);
        
        if (this->value > newDestinationValue)
        {
            this->value = (float)(this->startValue - (this->startValue - newDestinationValue) * result);
        }
        else
        {
            this->value = (float)(this->startValue + (newDestinationValue - this->startValue) * result);
        }
    }
    
    double getProgress() const
    {
        long long millis = getCurrentTimeMs();
        return (double)(millis - this->startTime) / (double)this->duration;
    }
    
    void reset()
    {
        this->startTime = getCurrentTimeMs();
        this->startValue = this->value;
        this->finished = false;
    }

    float getValue() const
    {
        return value;
    }

    void setValue(float val)
    {
        this->value = val;
    }

    bool isFinished() const
    {
        return finished;
    }
};
