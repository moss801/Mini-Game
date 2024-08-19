#pragma once
#include "../utils/baseData.h"
#include <functional>

typedef struct TimerInfo
{
    float timer;
    float interval;
    int remainTimes;
    std::function<void()> func;
    bool timerBySecond;
    bool isActive;
    TimerInfo(float initInterval, int totalTimes, std::function<void()> execfunc, bool bySecond)
        :timer(initInterval),interval(initInterval),remainTimes(totalTimes),func(execfunc), timerBySecond(bySecond),isActive(true)
    {}
};
class TimerMgr
{
public:
    static TimerMgr& GetInstance();

    //Only use in main Tick;
    void Tick();

    template<typename T>
    int AddTimer(float initInterval, int totalTimes, T *object,void (T::*func)() , bool bySecond);
    void RemoveTimer(int id);
    float GetDeltaTime();
    long GetServerTime();
    
private:
    TimerMgr();
    ~TimerMgr();
    const TimerMgr &operator=(const TimerMgr &timerMgr) = delete;
    TimerMgr(const TimerMgr &timerMgr) = delete;

    void HandleAllTimer();

    std::unordered_map<int,TimerInfo*> secondTimerMap;
    std::unordered_map<int,TimerInfo*> frameTimerMap;
    int index;
    float deltaTime;
    long lastTickTime;
};
template <typename T>
int TimerMgr::AddTimer(float initInterval, int totalTimes, T* object, void(T::* func)(), bool bySecond)
{
    std::function<void()> execFunc = std::bind(func,object);
    TimerInfo* timerInfo = new TimerInfo(initInterval,totalTimes,execFunc,bySecond);
    if(bySecond)
    {
        secondTimerMap[index] = timerInfo;
    }else
    {
        frameTimerMap[index] = timerInfo;
    }
    ++index;
    return index-1;
}


    



