#pragma once
#include <chrono>

class TimerHandler {
private:
    std::chrono::high_resolution_clock::time_point m_StartTime;

public:
    TimerHandler() {
        reset();
    }
    
    void reset() {
        m_StartTime = std::chrono::high_resolution_clock::now();
    }
    
    long long getPassedTimeMs() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_StartTime);
        return duration.count();
    }
    
    long long getTimeMs() const {
        return getPassedTimeMs();
    }
    
    bool passedS(double s) const {
        return getPassedTimeMs() >= static_cast<long long>(s * 1000.0);
    }
    
    bool passedMs(long long ms) const {
        return getPassedTimeMs() >= ms;
    }
    
    bool passedMs(int ms) const {
        return getPassedTimeMs() >= static_cast<long long>(ms);
    }
    
    bool passed(double ms) const {
        return getPassedTimeMs() >= static_cast<long long>(ms);
    }
    
    bool every(long long ms) {
        if (passedMs(ms)) {
            reset();
            return true;
        }
        return false;
    }
    
    void setMs(long long ms) {
        m_StartTime = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(ms);
    }
};