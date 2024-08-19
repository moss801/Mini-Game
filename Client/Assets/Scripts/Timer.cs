using UnityEngine;
using UnityEngine.Events;
public enum TimerType
{
    BySecond,
    ByFrame
}
public class Timer
{
    
    public UnityAction<int> action;
    public bool isAlwaysLoop;
    public TimerType type;
    public float remainTime;
    public float interval;
    public float remainLoop;
    public int id;
    public bool markDelete;
    
    /// <summary>
    /// 定时器构造函数.
    /// </summary>
    /// <param name="type">定时器类型（帧/秒）.</param>
    /// <param name="id">定时器句柄（-1表示无效）.</param>
    /// <param name="interval">定时间隔.</param>
    /// <param name="doCount">循环次数.</param>
    /// <param name="action">执行动作.</param>
    public Timer(TimerType type, int id, float interval, int doCount, UnityAction<int> action) {
            //...实现
            this.type = type;
            this.id = id;
            this.interval = interval;
            isAlwaysLoop = (doCount <= 0);
            remainLoop = doCount;
            this.action = action;
            markDelete = false;
    }
    
    /// <summary>
    /// 停止计时器.
    /// </summary>
    public void Stop() {
        //...实现
        TimerMgr.instance.StopTimer(id);
    }
    
    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    public void Update(float dt) {
       //...实现
    }
}
