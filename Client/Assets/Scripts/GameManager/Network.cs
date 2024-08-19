using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Threading;
using Google.Protobuf;
using TCCamp;
using UnityEngine;

public class Network : MonoBehaviour
{

    [SerializeField] private string serverIp;
    [SerializeField] private int serverPort;
    [SerializeField] private int maxRetryTime;
    [SerializeField] private int maxTolerance;

    private static Network _Instance;
    public static  Network instance => _Instance;

    private static Mutex mutex = new Mutex();

    private TcpClient client;

    private NetworkStream stream;

    private Thread sendThread;
    private Thread recvThread;

    private byte[] buffer;

    private static bool isOpen;

    private bool isReadingHead;

    private bool needReceive;

    private Queue<int> lengthQueue;

    private ConcurrentQueue<KeyValuePair<CLIENT_CMD, IMessage>> sendQueue;
    
    private static int counter;

    private static bool isSendThreadEnable;

    private void TryConnect()
    {
        if (isOpen)
        {
            return;
        }
        try
        {
            ConnectToServer();
            stream = client.GetStream();
        }
        catch (Exception e)
        {
            Debug.Log(e);
            return;
        }
        counter = 0;
        isOpen = true;
        EventMgr.instance.DispatchEvent("NetRecover");
        Debug.Log("Net is Ok");
        recvThread = new Thread(RecvMsg);
        recvThread.IsBackground = true;
        recvThread.Start();
    }
    private void Awake() {
        
        _Instance = this;

        //...实现
        
        isOpen = false;
        isSendThreadEnable = false;
        buffer = Array.Empty<byte>();
        lengthQueue = new Queue<int>();
        sendQueue = new ConcurrentQueue<KeyValuePair<CLIENT_CMD, IMessage>>();
    }

    private void OnDestroy()
    {
        Close();
    }

    /// <summary>
    /// Destroying the attached Behaviour will result in the game or Scene receiving OnDestroy.
    /// </summary>
    public void Close() {
       //...实现
       if (isOpen)
       {
           client.Close();
           stream.Close();
           isOpen = false;
       }
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    private void Update() { 
        //...实现
        mutex.WaitOne();
        while (lengthQueue.Count>0)
        {
            byte[] message = FetchData();
            int cmd = message[0] + (message[1] << 8);
            if (cmd == (int)SERVER_CMD.ServerPong)
            {
                counter = 0;
            }
            else
            {
                EventMgr.instance.DispatchEventImmediately("Network",cmd,message.Skip(2).ToArray());
            }
        }
        mutex.ReleaseMutex();
    }
    
    /// <summary>
    /// 连接服务器.
    /// </summary>
    private void ConnectToServer() {

        //...实现
        client = new TcpClient();
        client.Connect(serverIp,serverPort);
    }

    /// <summary>
    /// 发送消息.
    /// </summary>
    /// <param name="cmd">The first name to join.</param>
    /// <param name="msg">The last name to join.</param>
    public void SendMsg(TCCamp.CLIENT_CMD cmd, IMessage msg)
    {
        sendQueue.Enqueue(new KeyValuePair<CLIENT_CMD, IMessage>(cmd, msg));
        if (!isSendThreadEnable || sendThread.ThreadState == ThreadState.Stopped)
        {
            isSendThreadEnable = true;
            sendThread = new Thread(SendMsgImpl);
            sendThread.IsBackground = true;
            sendThread.Start();
        }
    }

    private void SendMsgImpl(){
        int i = 0;
        while (!isOpen)
        {
            EventMgr.instance.DispatchEvent("NetUnConnect");
            if (i == maxRetryTime)
            {
                Debug.LogWarning("Net is not Open,Retry " + maxRetryTime.ToString() + "times");
                EventMgr.instance.DispatchEvent("NetError");
                return;
            }
            TryConnect();
            ++i;
        }
        KeyValuePair<CLIENT_CMD,IMessage> pair;
        while (isOpen && sendQueue.TryDequeue(out pair))
        {
            CLIENT_CMD cmd = pair.Key;
            IMessage msg = pair.Value;
            //...实现
            byte[] packageBytes;
            if (msg != null)
            {
                byte[] msgBytes = msg.ToByteArray();
                packageBytes = new byte[6+msgBytes.Length];
                packageBytes[2] = (byte)((msgBytes.Length+2) & 0xFF);
                packageBytes[3] = (byte)(((msgBytes.Length+2)>>8) & 0xFF);
                msgBytes.CopyTo(packageBytes,6);
            }
            else
            {
                packageBytes = new byte[6];
                packageBytes[2] = 2;
                packageBytes[3] = 0;
            }
            packageBytes[0] = (byte)'T';
            packageBytes[1] = (byte)'C';
            packageBytes[4] = (byte)((int)cmd & 0xFF);
            packageBytes[5] = (byte)(((int)cmd>>8) & 0xFF);
            try
            {
                stream.Write(packageBytes,0,packageBytes.Length);
            }
            catch (Exception)
            {
                isOpen = false;
                sendQueue.Enqueue(pair);
                stream.Close();
                client.Close();
                isSendThreadEnable = false;
                return;
            }

            if (cmd == CLIENT_CMD.ClientPing)
            {
                ++counter;
                if (counter > maxTolerance)
                {
                    EventMgr.instance.DispatchEvent("NetUnConnect");
                }
            }
            
            if (sendQueue.Count == 0)
            {
                Thread.Sleep(1000);
            }
            else
            {
                Thread.Sleep(10);
            }
        }
        isSendThreadEnable = false;
    }

    /// <summary>
    /// 接受信息.
    /// </summary>
    private void RecvMsg() {

        //...实现
        while (isOpen)
        {
            try
            {
                byte[] head = new byte[4];
                isReadingHead = true;
                needReceive = true;
                ReadBytes(head, 4);
                isReadingHead = false;
                if (needReceive)
                {
                    int length = BytesToInt16(head, 2);
                    byte[] msgbuffer = new byte[length];
                    ReadBytes(msgbuffer, length);
                    mutex.WaitOne();
                    byte[] newBuffer = new byte[length + buffer.Length];
                    buffer.CopyTo(newBuffer, 0);
                    msgbuffer.CopyTo(newBuffer, buffer.Length);
                    buffer = newBuffer;
                    lengthQueue.Enqueue(length);
                    mutex.ReleaseMutex();
                }
                //SendMsg(CLIENT_CMD.ClientPing,null);
                Thread.Sleep(100);
            }
            catch (Exception)
            {
                stream.Close();
                client.Close();
                isOpen = false;
                return;
            }
        }
    }
    
    /// <summary>
    /// 读取数据.
    /// </summary>
    /// <param name="buffer">输出缓冲区.</param>
    /// <param name="count">读取长度.</param>
    //保证从网络读取到足够的大小
    private void ReadBytes(byte[] buffer, int count) {
       //...实现
       int currentLength = 0;
       while (currentLength < count)
       {
           int readLength = stream.Read(buffer, currentLength, count-currentLength);
           if (isReadingHead && readLength == 0)
           {
               needReceive = false;
               break;
           }
           currentLength += readLength;
       }
    }

    /// <summary>
    /// 从缓冲区读取包数据.
    /// </summary>
    /// <returns>包数据.</returns>
    private byte[] FetchData()
    {
        object message = Activator.CreateInstance<object>();
        if (lengthQueue.Count > 0)
        {
            int length = lengthQueue.Dequeue();
            byte[] info = buffer.Take(length).ToArray();
            buffer = buffer.Skip(length).ToArray();
            return info;
        }
        return Array.Empty<byte>();
    }

    public bool GetNetStatue()
    {
        return isOpen;
    }
    
    /// <summary>
    /// 将字节数组中制定位置的两个字节转为整数类型.
    /// </summary>
    /// <param name="src">字节数组.</param>
    /// <param name="offset">偏移.</param>
    /// <returns>指定偏移的数字.</returns>
    public static Int16 BytesToInt16(byte[] src, int offset) {
        return (Int16) ((src[offset] & 0xFF) | ((src[offset +1] & 0xFF) <<8));
    }
}
