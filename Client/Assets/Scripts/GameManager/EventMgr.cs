using System;
using System.Collections.Generic;


public class EventMgr {
    
    private class ListenerInfo
    {
        public string eventName;
        public int priority;
        public EventMgr.EventDelegate listener;

        /// <summary>
        /// 监听事件信息构造函数.
        /// </summary>
        /// <param name="priority">事件优先级.</param>
        /// <param name="listener">事件委托.</param>
        /// <returns>监听事件信息类实例.</returns>
        public ListenerInfo(string eventName, int priority, EventMgr.EventDelegate listener)
        {
            this.eventName = eventName;
            this.priority = priority;
            this.listener = listener;
        }
    }

    private class WaitInfo
    {
        public string eventName;
        public object[] param;

        /// <summary>
        /// 等待队列事件信息构造函数.
        /// </summary>
        /// <param name="eventName">事件名.</param>
        /// <param name="param">参数.</param>
        /// <returns>等待队列事件信息实例.</returns>
        public WaitInfo(string eventName,params object[] param)
        {
            this.eventName = eventName;
            this.param = param;
        }
    }
    
    private static EventMgr s_instance;

    public static EventMgr instance => s_instance ??= new EventMgr();

    public delegate void EventDelegate(params object[] args);

    private Dictionary<string, List<ListenerInfo>> eventDict;

    private Dictionary<EventDelegate,ListenerInfo> event2IdDict;

    private List<WaitInfo> waitList;

    private List<ListenerInfo> waitToRemoveList;

    private bool isInit;

    /// <summary>
    /// 事件管理器构造函数.
    /// </summary>
    private EventMgr()
    {
        eventDict = new Dictionary<string, List<ListenerInfo>>();
        event2IdDict = new Dictionary<EventDelegate, ListenerInfo>();
        waitList = new List<WaitInfo>();
        waitToRemoveList = new List<ListenerInfo>();
        isInit = false;
    }
    
    /// <summary>
    /// 添加监听.
    /// </summary>
    /// <param name="eventName">事件名.</param>
    /// <param name="listener">事件委托.</param>
    /// <param name="priority">优先级.</param>
    public void AddListener(string eventName, EventDelegate listener, int priority = 0) {

        //....实现
        if (!isInit)
        {
            TimerMgr.instance.StartTimerByFrame(1, -1, Update);
            isInit = true;
        }
        
        ListenerInfo newInfo = new ListenerInfo(eventName, priority, listener);
        event2IdDict.TryAdd(listener,newInfo);

        if (!eventDict.ContainsKey(eventName))
        {
            eventDict.Add(eventName,new List<ListenerInfo>());
        }
        eventDict[eventName].Add(newInfo);
        eventDict[eventName].Sort((ListenerInfo x,ListenerInfo y) => x.priority.CompareTo(y.priority));
        
    }

    /// <summary>
    /// 移除监听.
    /// </summary>
    /// <param name="eventName">事件名.</param>
    /// <param name="listener">事件委托.</param>
    public void RemoveListener(string eventName, EventDelegate listener)
    {
        ListenerInfo listenerInfo = new ListenerInfo(eventName,0,listener);
        waitToRemoveList.Add(listenerInfo);
    }
    
    /// <summary>
    /// 移除监听实现.
    /// </summary>
    private void RemoveListenerImpl() {
        //....实现
        for (int i = 0; i < waitToRemoveList.Count; i++)
        {
            ListenerInfo listenerInfo = waitToRemoveList[i];
            if (!event2IdDict.ContainsKey(listenerInfo.listener)||!eventDict.ContainsKey(listenerInfo.eventName))
            {
                continue;
            }
            for (int j = 0; j < eventDict[listenerInfo.eventName].Count; j++)
            {
                if (eventDict[listenerInfo.eventName][j] == event2IdDict[listenerInfo.listener])
                {
                    eventDict[listenerInfo.eventName].RemoveAt(j);
                    break;
                }
            }
        }
        waitToRemoveList.Clear();
    }

    /// <summary>
    /// 分发事件.
    /// </summary>
    /// <param name="eventName">事件名.</param>
    /// <param name="args">参数.</param>
    public void DispatchEvent(string eventName, params object[] args) {
       //....实现
       WaitInfo waitInfo = new WaitInfo(eventName,args);
       waitList.Add(waitInfo);
    }

    /// <summary>
    /// 分发事件(立即).
    /// </summary>
    /// <param name="eventName">事件名.</param>
    /// <param name="args">参数.</param>
    public void DispatchEventImmediately(string eventName, params object[] args) {
        //....实现
        if (eventDict.ContainsKey(eventName))
        {
            for (int i = 0; i < eventDict[eventName].Count; i++)
            {
                eventDict[eventName][i].listener.Invoke(args);
            }
        }
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    public void Update(int t) {
        //....实现
        RemoveListenerImpl();
        for (int i = 0; i < waitList.Count; i++)
        {
            DispatchEventImmediately(waitList[i].eventName,waitList[i].param);
        }
        waitList.Clear();
    }
}