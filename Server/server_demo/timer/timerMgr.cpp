#include "timerMgr.h"

#include <ctime>
#include "scene/rpcHandle.h"
#include "scene/sceneMgr.h"
#include "player/player.h"
#include "logic/logicMgr.h"
TimerMgr& TimerMgr::GetInstance()
{
    static TimerMgr timerMgr;
    return timerMgr;
}

void TimerMgr::Tick()
{
    deltaTime = (clock()-lastTickTime)/1000.f;
    lastTickTime = clock();
    HandleAllTimer();
}

void TimerMgr::RemoveTimer(int id)
{
    if(frameTimerMap.find(id)!=frameTimerMap.end())
    {
        frameTimerMap[id]->isActive = false;
    }else if(secondTimerMap.find(id)!=secondTimerMap.end())
    {
        secondTimerMap[id]->isActive = false;
    }
}

float TimerMgr::GetDeltaTime()
{
    return deltaTime;
}

long TimerMgr::GetServerTime()
{
    return lastTickTime;
}

TimerMgr::TimerMgr()
{
    index = 0;
    lastTickTime = clock();
}
TimerMgr::~TimerMgr()
{
    
}


void TimerMgr::HandleAllTimer()
{
    for(auto it = frameTimerMap.begin();it!=frameTimerMap.end();)
    {
        if(it->second->isActive)
        {
            TimerInfo * timeInfo = it->second;
            if(--timeInfo->timer<=0)
            {
                timeInfo->func();
                timeInfo->timer = timeInfo->interval;
                if(timeInfo->remainTimes == -1)
                {
                    ++it;
                    continue;
                }else if(--timeInfo->remainTimes==0)
                {
                    it = frameTimerMap.erase(it);
                }
            }else
            {
                ++it;
            }
        }else
        {
            it = frameTimerMap.erase(it);
        }
    }
    for(auto it = secondTimerMap.begin();it!=secondTimerMap.end();)
    {
        if(it->second->isActive)
        {
            TimerInfo * timeInfo = it->second;
            timeInfo->timer -= deltaTime;
            if(timeInfo->timer<=0)
            {
                timeInfo->func();
                timeInfo->timer = timeInfo->interval;
                if(timeInfo->remainTimes == -1)
                {
                    ++it;
                    continue;
                }else if(--timeInfo->remainTimes==0)
                {
                    it = secondTimerMap.erase(it);
                }
            }else
            {
                ++it;
            }
        }else
        {
            it = secondTimerMap.erase(it);
        }
    }
}


