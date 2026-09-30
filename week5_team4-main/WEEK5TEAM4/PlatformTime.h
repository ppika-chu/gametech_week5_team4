#pragma once

#include <windows.h>
#include "Core.h"

// 고정밀 하드웨어 타이머(QueryPerformanceCounter) 래퍼.
// 엔진의 CPU 시간 측정은 모두 이 클래스를 통해 읽는다. (FFrameTimer, FScopeCycleCounter)
class FWindowsPlatformTime
{
public:
    inline static double GSecondsPerCycle; // 0
    inline static bool bInitialized; // false

    inline static void InitTiming() {
        if (!bInitialized)
        {
            bInitialized = true;

            double Frequency = (double)GetFrequency();
            if (Frequency <= 0.0)
            {
                Frequency = 1.0;
            }

            GSecondsPerCycle = 1.0 / Frequency;
        }
    }

    inline static float GetSecondsPerCycle() {
        if (!bInitialized)
        {
            InitTiming();
        }
        return (float)GSecondsPerCycle;
    }

    inline static uint64 GetFrequency() {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        return Frequency.QuadPart;
    }

    inline static double ToMilliseconds(uint64 CycleDiff) {
        double Ms = static_cast<double>(CycleDiff)
            * GetSecondsPerCycle()
            * 1000.0;

        return Ms;
    }

    inline static uint64 Cycles64() {
        LARGE_INTEGER CycleCount;
        QueryPerformanceCounter(&CycleCount);
        return (uint64)CycleCount.QuadPart;
    }
};

typedef FWindowsPlatformTime FPlatformTime;
