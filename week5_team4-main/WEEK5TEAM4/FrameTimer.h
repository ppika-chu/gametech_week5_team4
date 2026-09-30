#pragma once
#include <windows.h>
#include <cfloat>
#include <cstdint>

class FFrameTimer
{
public:
	FFrameTimer(int TargetFPS) : targetFrameTime(1000.0 / TargetFPS), elapsedTime(1000.0 / TargetFPS)
	{
		QueryPerformanceFrequency(&Frequency);
	}

	void StartFrame()
	{
		deltaTime = (float)(elapsedTime * 0.001);
		if (deltaTime > 0.1f) deltaTime = 0.1f;

		QueryPerformanceCounter(&StartTime);
	}

	void EndFrame()
	{	
		do
		{
			Sleep(0);

			QueryPerformanceCounter(&EndTime);
			elapsedTime = (EndTime.QuadPart - StartTime.QuadPart) * 1000.0 / Frequency.QuadPart;

		} while (elapsedTime < targetFrameTime);

		AccumulateIntervalStats();
	}

	float GetDeltaTime() const { return deltaTime; }
	float GetFPS() const { return elapsedTime > 0.0 ? (float)(1000.0 / elapsedTime) : 0.f; }
	float GetFrameTimeMs() const { return static_cast<float>(elapsedTime); } // 클램프 안 된 프로파일링용

	// 구간 평균: StatsIntervalMs 동안의 (프레임 수 / 경과 시간). 구간이 끝날 때만 갱신된다.
	float GetIntervalAverageFPS() const { return intervalAverageFPS; }
	float GetIntervalAverageFrameMs() const { return intervalAverageFrameMs; }
	float GetIntervalMinFrameMs() const { return intervalMinFrameMs; }
	float GetIntervalMaxFrameMs() const { return intervalMaxFrameMs; }

private:
	void AccumulateIntervalStats()
	{
		accumulatedMs += elapsedTime;
		++accumulatedFrames;
		if (elapsedTime < pendingMinMs) pendingMinMs = elapsedTime;
		if (elapsedTime > pendingMaxMs) pendingMaxMs = elapsedTime;

		if (accumulatedMs < StatsIntervalMs)
		{
			return;
		}

		// FPS 값끼리 평균내지 않고, 구간의 프레임 수 / 구간 시간으로 계산한다
		intervalAverageFrameMs = (float)(accumulatedMs / accumulatedFrames);
		intervalAverageFPS = (float)(accumulatedFrames * 1000.0 / accumulatedMs);
		intervalMinFrameMs = (float)pendingMinMs;
		intervalMaxFrameMs = (float)pendingMaxMs;

		accumulatedMs = 0.0;
		accumulatedFrames = 0;
		pendingMinMs = DBL_MAX;
		pendingMaxMs = 0.0;
	}

	static constexpr double StatsIntervalMs = 500.0;

	double targetFrameTime;
	double elapsedTime;
	float deltaTime = 0.f;
	LARGE_INTEGER Frequency, StartTime, EndTime;

	double accumulatedMs = 0.0;
	uint32_t accumulatedFrames = 0;
	double pendingMinMs = DBL_MAX;
	double pendingMaxMs = 0.0;

	float intervalAverageFPS = 0.f;
	float intervalAverageFrameMs = 0.f;
	float intervalMinFrameMs = 0.f;
	float intervalMaxFrameMs = 0.f;
};
