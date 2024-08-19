using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Events;

public class TimerMgr : MonoBehaviour
{

    private Dictionary<int,Timer> timerDict;

    private int[] timerIdArray;

    private int indexNum;
    
    public static TimerMgr instance = null;

    private bool markDarty;
    
    /// <summary>
    /// 启动定时器（按秒）.
    /// </summary>
    /// <param name="interval">时间间隔.</param>
    /// <param name="doCount">循环次数（-1为无限）.</param>
    /// <param name="action">执行的函数.</param>
    /// <returns>定时器句柄.</returns>
    public int StartTimerBySecond(float interval, int doCount, UnityAction<int> action) {
        //...实现
        Timer newTimer = new Timer(TimerType.BySecond, indexNum, interval, doCount, action);
        timerDict.Add(indexNum,newTimer);
        timerIdArray = timerDict.Keys.ToArray();
        ++indexNum;
        return indexNum - 1;
    }

    /// <summary>
    /// 启动定时器（按帧）.
    /// </summary>
    /// <param name="interval">时间间隔.</param>
    /// <param name="doCount">循环次数（-1为无限）.</param>
    /// <param name="action">执行的函数.</param>
    /// <returns>定时器句柄.</returns>
    public int StartTimerByFrame(uint interval, int doCount, UnityAction<int> action) {
        //...实现
        Timer newTimer = new Timer(TimerType.ByFrame, indexNum, interval, doCount, action);
        timerDict.Add(indexNum,newTimer);
        timerIdArray = timerDict.Keys.ToArray();
        ++indexNum;
        return indexNum - 1;
    }

    /// <summary>
    /// 停止定时器.
    /// </summary>
    /// <param name="timerID">定时器句柄.</param>
    public void StopTimer(int timerID) {
        //...实现
        timerDict[timerID].markDelete = true;
    }
    
    /// <summary>
    /// Awake is called when an enabled script instance is being loaded.
    /// </summary>
    private void Awake() {
        instance = this;
        
       //...实现

       timerDict = new Dictionary<int, Timer>();
       indexNum = 0;
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    private void Update() {
        //...实现
        if (timerIdArray is null)
        {
            return;
        }
        for (int i = 0; i < timerIdArray.Length; i++)
        {
            ProcessTimer(timerDict[timerIdArray[i]]);
        }
        if (markDarty)
        {
            timerIdArray = timerDict.Keys.ToArray();
            markDarty = false;
        }
    }

    /// <summary>
    /// 处理定时器.
    /// </summary>
    /// <param name="timer">定时器.</param>
    private void ProcessTimer(Timer timer)
    {
        if (timer.markDelete)
        {
            RemoveTimer(timer.id);
            return;
        }
        if (timer.type == TimerType.ByFrame)
        {
            timer.remainTime--;
        }
        else
        {
            timer.remainTime-=Time.deltaTime;
        }
        if (timer.remainTime <= 0)
        {
            timer.remainTime = timer.interval;
            timer.action.Invoke(0);
            if (!timer.isAlwaysLoop)
            {
                timer.remainLoop--;
                if (timer.remainLoop == 0)
                {
                    RemoveTimer(timer.id);
                }
            }
        }
    }
    
    /// <summary>
    /// 从列表中移除定时器.
    /// </summary>
    /// <param name="id">定时器句柄.</param>
    private void RemoveTimer(int id)
    {
        timerDict.Remove(id);
        markDarty = true;
    }
}
