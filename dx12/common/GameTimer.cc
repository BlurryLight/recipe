#include "GameTimer.h"
#include <algorithm>
#include <windows.h>
#include <winnt.h>

namespace PD {

    GameTimer::GameTimer()
        : mSecondsPerCount(0.0), mDeltaTime(-1), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0),
          mStopped(false) {
        int64_t countsPerSec{};
        QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
        mSecondsPerCount = 1.0 / (double)countsPerSec;
    }

     float GameTimer::TotalTime() const {

         if (!mStopped) {
             return (float(mCurrTime - mBaseTime - mPausedTime)) * (float) mSecondsPerCount;
         } else {

             return (float(mStopTime - mBaseTime - mPausedTime)) * (float) mSecondsPerCount;
         }
     }


     float GameTimer::DeltaTime() const { return (float) mDeltaTime; }

     void GameTimer::Reset() {

         int64_t curTime{};
         QueryPerformanceCounter((LARGE_INTEGER *) &curTime);
         mBaseTime = curTime;
         mPrevTime = curTime;
         mStopTime = 0;
         mStopped = false;
    }

    void GameTimer::Start() {
        int64_t startTime{};
        QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
        if(mStopped)
        {
            mPausedTime += (startTime - mStopTime);
            mPrevTime = startTime;
            mStopTime = 0;
            mStopped = false;
        }
    
    }

    void GameTimer::Stop() {
    
        if(!mStopped)
        {
        
            int64_t curTime{};
            QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
            mStopTime = curTime;
            mStopped = true;
        }
    }

    void GameTimer::Tick() {
        if(mStopped)
        {
            mDeltaTime = 0;
            return;
        }

        int64_t curTime{};
        QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
        mCurrTime = curTime;
        mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
        mPrevTime = mCurrTime;

        // in multi-processer system it may be negative
        mDeltaTime = std::max(mDeltaTime,0.);
    }

} // namespace PD
