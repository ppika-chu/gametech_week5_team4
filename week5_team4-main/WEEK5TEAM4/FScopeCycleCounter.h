#pragma once

#include <windows.h>
#include "Core.h"
#include "FName.h"
#include "TMap.h"
#include "TArray.h"

#define STAT_CONCAT_INNER(A, B) A##B
#define STAT_CONCAT(A, B) STAT_CONCAT_INNER(A, B)
#define ANONYMOUS_VAR(Name) STAT_CONCAT(Name, __LINE__)

#define GET_STAT_ID(NameLiteral) \
    ([]() { static FStatEntry* E = FStatRegistry::Get().FindOrAdd(FName(NameLiteral)); return TStatId{ E }; }())

#define SCOPE_CYCLE_COUNTER(NameLiteral) \
    FScopeCycleCounter ANONYMOUS_VAR(ScopeCounter_)(GET_STAT_ID(NameLiteral))

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

struct FStatEntry
{
    FName   Name;                  //	표시용 이름(예: "Mouse Picking")
    uint64  TotalCycles = 0;     //	실행 시작부터의 누적 사이클
    uint64  CallCount = 0;       //	누적 호출 횟수
    uint64  FrameCycles = 0;     //	이번 프레임에 쌓인 사이클
    uint64  LastFrameCycles = 0; //	직전 프레임 값(화면 표시용)
    uint64  LastCycles = 0;      //	마지막 1회 측정값 (피킹처럼 가끔 일어나는 작업용)
    uint32  FrameCallCount = 0;    // 이번 프레임에 호출된 횟수 (실행 여부 판단용. 시간이 0사이클로 잡혀도 호출은 센다)
    uint32  ActiveFrameStreak = 0; // 연속으로 실행된 프레임 수 (2 이상이면 매 프레임 실행되는 구간으로 본다)
};



class FStatRegistry
{
private:
    TMap<FName, FStatEntry*> Entries;
    // 화면 표시 순서를 고정하기 위한 등록 순서 목록 (TMap은 순서가 보장되지 않는다)
    TArray<FStatEntry*> OrderedEntries;

    FStatRegistry() = default;
    ~FStatRegistry()
    {
        for (const auto& [Name, Entry] : Entries)
        {
            delete(Entry);
        }
    }
    FStatRegistry(const FStatRegistry&) = delete;
    FStatRegistry& operator=(const FStatRegistry&) = delete;

public:

    static FStatRegistry& Get()
    {
        static FStatRegistry Instance;
        return (Instance);
    }


    FStatEntry* FindOrAdd(const FName& Name)
    {
        if (FStatEntry** FindStatEntry = Entries.Find(Name))
        {
            return (*FindStatEntry);
        }

        FStatEntry* NewEntry = new FStatEntry();
        NewEntry->Name = Name;
        Entries.Add(Name, NewEntry);
        OrderedEntries.Add(NewEntry);
        return NewEntry;
    }

    // 프레임 끝에서 호출: 이번 프레임 누적값을 표시용으로 옮기고 다음 프레임을 위해 비운다
    void EndFrame()
    {
        for (FStatEntry* Entry : OrderedEntries)
        {
            // 실행 여부는 시간(FrameCycles)이 아니라 호출 횟수로 판단한다.
            // 100ns 미만으로 끝나는 짧은 구간은 0사이클로 측정될 수 있어서, 시간으로 판단하면 프레임마다 형식이 바뀐다.
            Entry->ActiveFrameStreak = (Entry->FrameCallCount > 0) ? Entry->ActiveFrameStreak + 1 : 0;
            Entry->LastFrameCycles = Entry->FrameCycles;
            Entry->FrameCycles = 0;
            Entry->FrameCallCount = 0;
        }
    }

    const TMap<FName, FStatEntry*>& GetEntries() const
    {
        return (Entries);
    }

    const TArray<FStatEntry*>& GetOrderedEntries() const
    {
        return (OrderedEntries);
    }

};

struct TStatId
{
    FStatEntry* Entry = nullptr;
};

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:

    FScopeCycleCounter(TStatId StatId)
        : StartCycles(FPlatformTime::Cycles64())
        , UsedStatId(StatId)
    { 
    }



    ~FScopeCycleCounter()
    {
        Finish();
    }

    uint64 Finish() {
        if (bFinished)
        {
            return (ResultCycles);
        }

        const uint64 EndCycles = FPlatformTime::Cycles64();
        ResultCycles = EndCycles - StartCycles;

        if (UsedStatId.Entry != nullptr)
        {
            UsedStatId.Entry->TotalCycles += ResultCycles;
            UsedStatId.Entry->FrameCycles += ResultCycles;
            UsedStatId.Entry->LastCycles = ResultCycles;
            UsedStatId.Entry->CallCount++;
            UsedStatId.Entry->FrameCallCount++;
        }

        bFinished = true;
        // FThreadStats::AddMessage(UsedStatId, EStatOperation::Add, CycleDiff);

        return ResultCycles;
    }

private:
    uint64  StartCycles;
    uint64  ResultCycles = 0;
    TStatId UsedStatId;
    bool    bFinished = false;

};