#pragma once
#include <stdint.h>
namespace PD {
    class GameTimer {
    public:
        GameTimer();


        // Total time since Reset() without stoped
        float TotalTime() const;
        float DeltaTime() const;
        void Reset();
        void Start();
        void Stop();
        void Tick();

    private:
        double mSecondsPerCount;

        // in seconds
        double mDeltaTime;

        // in counters
        int64_t mBaseTime;
        int64_t mPausedTime;
        int64_t mStopTime;
        int64_t mPrevTime;
        int64_t mCurrTime;
        bool mStopped;
    };


}// namespace PD