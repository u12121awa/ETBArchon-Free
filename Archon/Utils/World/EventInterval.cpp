#include "pch.h"
#include <Windows.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <numeric>

namespace EventInterval
{
    struct Meter
    {
        long long LastTime = 0;
        long long Frequency = 0;
        std::size_t HistoryCount = 0;
        std::vector<double> Intervals;
    };

    static std::unordered_map<std::string, Meter> Meters;

    static Meter& GetMeter(const char* name, std::size_t historyCount)
    {
        auto it = Meters.find(name);
        if (it == Meters.end())
        {
            Meter m;
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            m.Frequency = freq.QuadPart;
            m.LastTime = 0;
            m.HistoryCount = historyCount;
            auto res = Meters.emplace(name, std::move(m));
            return res.first->second;
        }
        else
        {
            if (historyCount != 0)
                it->second.HistoryCount = historyCount;
            return it->second;
        }
    }

    void OnEvent(const char* name, std::size_t historyCount)
    {
        if (!name || historyCount == 0)
            return;

        Meter& meter = GetMeter(name, historyCount);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        if (meter.LastTime != 0)
        {
            double deltaSeconds = double(now.QuadPart - meter.LastTime) / double(meter.Frequency);
            double deltaMs = deltaSeconds * 1000.0;

            meter.Intervals.push_back(deltaMs);
            if (meter.Intervals.size() > meter.HistoryCount)
                meter.Intervals.erase(meter.Intervals.begin());

            double avg = 0.0;
            if (!meter.Intervals.empty())
            {
                double sum = std::accumulate(meter.Intervals.begin(), meter.Intervals.end(), 0.0);
                avg = sum / double(meter.Intervals.size());
            }

            Log::Info(
                std::string(name) +
                " interval: " + std::to_string(deltaMs) +
                " ms, avg: " + std::to_string(avg) + " ms"
            );
        }

        meter.LastTime = now.QuadPart;
    }
}
