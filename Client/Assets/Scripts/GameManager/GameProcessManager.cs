using System;
using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using UnityEditor;
using UnityEngine;

public class GameProcessManager : MonoBehaviour
{
    [HideInInspector] public bool isSingle;
    [SerializeField] private float delayStartTime;
    [SerializeField] private int defaultGameTime;
    [SerializeField] private int maxRecoverTime;
    private UIManager UImanager;
    private bool isGame;
    private bool isPreparing;
    private int checkMgrNum;
    private Network network;
    private string userName;
    private int killNum;
    private int score;
    private int gameTime;
    private int rank;
    private bool isWin;
    private int tryRecoverTime;
    private bool isNormalEnd;
    private TimerMgr timerMgr;
    private HashSet<string> needReadyMgrs;
    

    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        UImanager = GameObject.Find("GameManager").GetComponent<UIManager>();
        network = GameObject.Find("GameManager").GetComponent<Network>();
        EventMgr.instance.AddListener("Network",OnRecv);
        EventMgr.instance.AddListener("LoginDone", LoginDone);
        EventMgr.instance.AddListener("Logout", Loginout);
        EventMgr.instance.AddListener("NetError",OnNetError);
        EventMgr.instance.AddListener("NetRecover",OnNetRecover);
        EventMgr.instance.AddListener("NetUnConnect",OnNetUnConnect);
        timerMgr = GameObject.Find("GameManager").GetComponent<TimerMgr>();
        needReadyMgrs = new HashSet<string>();
        isGame = false;
        isSingle = true;
        isNormalEnd = true;
        isPreparing = false;
        killNum = 0;
        userName = "Single";
    }

    private void LoginDone(params object[] args)
    {
        userName = args[0].ToString();
        network.SendMsg(CLIENT_CMD.ClientGamecontinueReq,null);
    }
    
    private void Loginout(params object[] args)
    {
        userName = "Single";
    }

    public string GetUserName()
    {
        return userName;
    }

    public void ChangeScore(int delta)
    {
        if (delta == 0)
        {
            return;
        }
        score += delta;
        EventMgr.instance.DispatchEvent("ScoreChange",score);
    }

    public void ChangeKill()
    {
        ++killNum;
    }
    
    public int GetScore()
    {
        return score;
    }

    public int GetGameTime()
    {
        return gameTime;
    }

    public void SetGameResult(bool isSuccess)
    {
        isWin = isSuccess;
        EndGame();
    }
    
    public void StartGame()
    {
        if (isGame)
        {
            return;
        }
        isPreparing = true;
        UImanager.HideAll();
        UImanager.InvokeCanvas(7);

        isNormalEnd = true;
        score = 0;
        gameTime = defaultGameTime;
        rank = 1;
        isWin = false;
        
        checkMgrNum = 4;
        GameObject gameManager = GameObject.Find("GameManager");
        gameManager.GetComponent<CharacterMgr>().CreateCharacter();
        gameManager.GetComponent<EnemyMgr>().GenerateEnemy();
        gameManager.GetComponent<SceneMgr>().Init();
        gameManager.GetComponent<BulletPool>().InitPool();
        if (!isSingle)
        {
            network.SendMsg(CLIENT_CMD.ClientScenedetailReq,null);
            InvokeRepeating("SendTimeReq",0,1);
            Invoke("CountDownToEndGame",10);
        }
        else
        {
            InvokeRepeating("CalcTime",0,1);
        }
    }

    /// <summary>
    /// 结束游戏.
    /// </summary>
    private void EndGame()
    {
        if (isGame||isPreparing)
        {
            isGame = false;
            UImanager.HideAll();
            UImanager.InvokeCanvas(4);
            if (isSingle)
            {
                score += gameTime * 300; 
                UImanager.GetUI(4).GetComponent<FinishUIScript>().InitResult(isWin,score,defaultGameTime-gameTime,userName,rank,killNum);
                CancelInvoke("CalcTime");
            }
            else if(isNormalEnd)
            {
                UImanager.GetUI(4).GetComponent<FinishUIScript>().InitResult(isWin,score,gameTime,userName,rank,killNum);
                CancelInvoke("SendTimeReq");
            }
            else
            {
                BackToMenu();
                UImanager.InvokeCanvas(6);
                UImanager.GetUI(6).GetComponent<HintUIScript>().InitHint("游戏异常，已退出",Color.gray, 3);
                EventMgr.instance.DispatchEvent("Logout");
            }
            GameObject gameManager = GameObject.Find("GameManager");
            gameManager.GetComponent<InputReceiver>().enabled = false;
            gameManager.GetComponent<CharacterMgr>().ClearAll();
            gameManager.GetComponent<EnemyMgr>().ClearAll();
            gameManager.GetComponent<SceneMgr>().ClearAll();
            gameManager.GetComponent<BulletPool>().ClearAll();
        }
    }

    public void SetReady(string managerName)
    {
        Debug.Log(managerName + " Has Ready!");
        needReadyMgrs.Add(managerName);
        if (checkMgrNum == needReadyMgrs.Count)
        {
            isPreparing = false;
            needReadyMgrs.Clear();
            if (isSingle)
            {
                Invoke("StartImpl",delayStartTime);
            }
            else
            {
                CancelInvoke("CountDownToEndGame");
                Invoke("SendStartSign",delayStartTime);
            }
            UImanager.HideCanvas(7);
            UImanager.InvokeCanvas(8);
            UImanager.GetUI(8).GetComponent<CountUIScript>().Initialization((int)delayStartTime,false);
        }
    }

    private void SendStartSign()
    {
        network.SendMsg(CLIENT_CMD.ClientStartReq,null);
    }

    private void StartImpl()
    {
        isGame = true;
        UImanager.HideAll();
        GameObject.Find("GameManager").GetComponent<InputReceiver>().enabled = true;
        UImanager.InvokeCanvas(2);
        UImanager.InvokeCanvas(3);
    }

    private void GamePause()
    {
        GameObject.Find("GameManager").GetComponent<InputReceiver>().enabled = false;
        UImanager.InvokeCanvas(7);
    }

    private void GameContinue()
    {
        GameObject.Find("GameManager").GetComponent<InputReceiver>().enabled = true;
        UImanager.HideCanvas(7);
    }

    private void OnNetUnConnect(params object[] param)
    {
        if (!isGame||isSingle)
        {
            return;
        }
        GamePause();
        UImanager.InvokeCanvas(6);
        UImanager.GetUI(6).GetComponent<HintUIScript>().InitHint("失去网络连接，重试中...",Color.gray,-1);
    }
    
    private void OnNetError(params object[] param)
    {
        if (!isGame||isSingle)
        {
            return;
        }
        ++tryRecoverTime;
        TryRecoverNet();
        UImanager.InvokeCanvas(6);
        UImanager.GetUI(6).GetComponent<HintUIScript>().InitHint("网络错误，重试中...",Color.gray,-1);
    }

    private void CountDownToEndGame()
    {
        isNormalEnd = false;
        EndGame();
    }

    private void OnNetRecover(params object[] param)
    {
        if (!isGame||isSingle)
        {
            return;
        }
        tryRecoverTime = 0;
        CancelInvoke("TryRecoverNet");
        CancelInvoke("CountDownToEndGame");
        UImanager.HideCanvas(6);
        GameContinue();
    }

    private void TryRecoverNet()
    {
        if (tryRecoverTime > maxRecoverTime)
        {
            EndGame();
        }
        else
        {
            network.SendMsg(CLIENT_CMD.ClientPing,null);
            ++tryRecoverTime;
            Invoke("CountDownToEndGame",3);
            UImanager.InvokeCanvas(6);
            UImanager.GetUI(6).GetComponent<HintUIScript>().InitHint("网络错误，重试失败。即将返回...",Color.gray,-1);
        }
    }
    
    
    private void OnRecv(params object[] param)
    {
        if ((int)param[0] == (int)SERVER_CMD.ServerStartRsp)
        {
            StartImpl();
        }else if ((int)param[0] == (int)SERVER_CMD.ServerGamescoreRsp)
        {
            ScoreRsp rsp = new ScoreRsp();
            rsp.MergeFrom((byte[])param[1]);
            score = rsp.Score;
            EventMgr.instance.DispatchEvent("ScoreChange",score);
        }else if ((int)param[0] == (int)SERVER_CMD.ServerEndRsp)
        {
            GameResultRsp rsp = new GameResultRsp();
            rsp.MergeFrom((byte[])param[1]);
            if (!rsp.IsFinished)
            {
                killNum = rsp.BeatCount;
                score = rsp.Score;
                gameTime = rsp.GameTime;
                isWin = rsp.Success;
                rank = rsp.RoomRank;
                EndGame();
            }
            else
            {
                EventMgr.instance.DispatchEvent("HasFinish");
            }
            
        }else if ((int)param[0] == (int)SERVER_CMD.ServerGametimeRsp)
        {
            GameTimeRsp rsp = new GameTimeRsp();
            rsp.MergeFrom((byte[])param[1]);
            gameTime = rsp.Time;
        }else if ((int)param[0] == (int)SERVER_CMD.ServerGamecontinueRsp)
        {
            GameContinueRsp rsp = new GameContinueRsp();
            rsp.MergeFrom((byte[])param[1]);
            if (rsp.NeedContinue)
            {
                isSingle = false;
                StartGame();
            }
        }
    }

    private void SendTimeReq()
    {
        network.SendMsg(CLIENT_CMD.ClientGametimeReq,null);
    }

    private void CalcTime()
    {
        --gameTime;
        if (gameTime == 0)
        {
            EndGame();
        }
    }

    /// <summary>
    /// 获取游戏是否正在进行.
    /// </summary>
    /// <returns>游戏是否正在运行.</returns>
    public bool isGaming()
    {
        return isGame;
    }

    public bool IsPreparing()
    {
        return isPreparing;
    }

    public void BackToMenu()
    {
        UImanager.HideAll();
        UImanager.InvokeCanvas(0);
    }

    public void ExitGame()
    {
        Network network = GameObject.Find("GameManager").GetComponent<Network>();
        network.Close();
#if UNITY_EDITOR
        EditorApplication.isPlaying = false;
#else
        Application.Quit();
#endif
    }
}
