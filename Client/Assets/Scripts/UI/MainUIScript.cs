using System;
using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using TMPro;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.UI;

public class MainUIScript : MonoBehaviour
{
    [SerializeField] private GameObject mainPanel;
    [SerializeField] private GameObject scenePanel;
    [SerializeField] private GameObject helpPanel;
    [SerializeField] private TextMeshProUGUI userNameText;
    [SerializeField] private TextMeshProUGUI loginButtonText;
    [SerializeField] private GameObject sceneContentUI;
    [SerializeField] private GameObject sceneDetailPrefab;
    [SerializeField] private float sceneDetailDistance;
    [SerializeField] private GameObject rankListContentUI;
    [SerializeField] private GameObject rankListDetailPrefab;
    [SerializeField] private float rankListDetailDistance;
    [SerializeField] private GameObject roomContentUI;
    [SerializeField] private GameObject roomDetailPrefab;
    [SerializeField] private float roomDetailDistance;
    [SerializeField] private Slider maxNumSlider;
    [SerializeField] private TMP_InputField roomNameInput;
    [SerializeField] private GameObject CreateRoomPanel;
    [SerializeField] private TextMeshProUGUI maxNumText;
    [SerializeField] private GameObject sceneBriefPanel;
    [SerializeField] private GameObject sceneInfoPanel;
    [SerializeField] private TextMeshProUGUI sceneNameText;
    
    private UIManager uiManager;
    private Network network;
    private GameProcessManager gameProcessManager;
    private List<GameObject> sceneDetailList;
    private bool isLogin;
    [HideInInspector] public int sceneId;
    private Dictionary<int, string> sceneId2NameDict;

    private void Start()
    {
        isLogin = false;
        uiManager = GameObject.Find("GameManager").GetComponent<UIManager>();
        gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        EventMgr.instance.AddListener("LoginDone",OnLoginDone);
        EventMgr.instance.AddListener("Network",OnSceneInfoRecv);
        EventMgr.instance.AddListener("Logout",Logout);
        network = GameObject.Find("GameManager").GetComponent<Network>();
        sceneId2NameDict = new Dictionary<int, string>();
        sceneId = -1;
    }

    public void Back()
    {
        scenePanel.SetActive(false);
        helpPanel.SetActive(false);
        mainPanel.SetActive(true);
        SetSceneId(-1);
    }

    public void SinglePlay()
    {
        gameProcessManager.isSingle = true;
        gameProcessManager.StartGame();
    }

    public void MultablePlay()
    {
        if (isLogin)
        {
            scenePanel.SetActive(true);
            helpPanel.SetActive(false);
            mainPanel.SetActive(false);
            network.SendMsg(CLIENT_CMD.ClientScenebriefReq,null);
        }
        else
        {
            Login();
        }
    }

    public void OpenHelp()
    {
        scenePanel.SetActive(false);
        helpPanel.SetActive(true);
        mainPanel.SetActive(false);
    }

    public void ExitGame()
    {
        gameProcessManager.ExitGame();
    }

    public void Login()
    {
        if (isLogin)
        {
            EventMgr.instance.DispatchEvent("Logout");
            network.SendMsg(CLIENT_CMD.ClientLogoutReq,null);
        }
        else
        {
            uiManager.InvokeCanvas(1);
        }
    }

    private void Logout(params object[] args)
    {
        isLogin = false;
        userNameText.text = "请登录";
        loginButtonText.text = "登录";
        Back();
    }

    private void OnLoginDone(params object[] param)
    {
        isLogin = true;
        loginButtonText.text = "退出";
        userNameText.text = param[0].ToString();
    }

    public void CreateRoom()
    {
        if (sceneId == -1)
        {
            uiManager.InvokeCanvas(6);
            GameObject hintUI = uiManager.GetUI(6);
            if (hintUI is not null)
            {
                hintUI.GetComponent<HintUIScript>().InitHint("请先选择副本",Color.gray,3);
            }
            return;
        }
        CreateRoomReq req = new CreateRoomReq();
        req.SceneId = sceneId;
        req.MaxNum = (int)maxNumSlider.value;
        req.RoomName = roomNameInput.text;
        network.SendMsg(CLIENT_CMD.ClientCreateroomReq,req);
    }

    public void OpenCreateRoomUI()
    {
        CreateRoomPanel.SetActive(true);
    }

    public void CancelCreateRoomUI()
    {
        CreateRoomPanel.SetActive(false);
    }

    public void OnSliderValueChange()
    {
        maxNumText.text = maxNumSlider.value.ToString();
    }
    
    private void OnSceneInfoRecv(params object[] param)
    {
        if ((int)param[0] == (int)SERVER_CMD.ServerScenebriefRsp)
        {
            sceneId2NameDict.Clear();
            SceneBriefRsp rsp = new SceneBriefRsp();
            rsp.MergeFrom((byte[])param[1]);
            for (int i = 0; i < rsp.SceneBrief.Count; i++)
            {
                SceneBriefMsg msg = rsp.SceneBrief[i];
                GameObject newSceneBrief = Instantiate(sceneDetailPrefab,sceneContentUI.transform);
                newSceneBrief.transform.localPosition = new Vector2(i * sceneDetailDistance,0);
                sceneId2NameDict[msg.SceneId] = msg.SceneName;
                newSceneBrief.GetComponent<SceneDetailScript>().InitDetail(msg.SceneId,msg.Difficult,msg.GhostCount,msg.GargoyleCount,msg.SceneName);
            }
        }else if((int)param[0] == (int)SERVER_CMD.ServerRanklistRsp)
        {
            RankListRsp rsp = new RankListRsp();
            rsp.MergeFrom((byte[])param[1]);
            for (int i = rsp.RankList.Count - 1; i >= 0; --i)
            {
                RankListMsg msg = rsp.RankList[i];
                GameObject newRankListDetail = Instantiate(rankListDetailPrefab, rankListContentUI.transform);
                newRankListDetail.transform.localPosition = new Vector2(0, -i * rankListDetailDistance);
                newRankListDetail.GetComponent<RankDetailScript>().InitDetail(i+1,msg.Name,msg.Score,msg.Time);
            }
        }else if((int)param[0] == (int)SERVER_CMD.ServerRoomRsp)
        {
            RoomInfoRsp rsp = new RoomInfoRsp();
            rsp.MergeFrom((byte[])param[1]);
            for (int i = 0; i < rsp.RoomInfoList.Count; ++i)
            {
                RoomInfoMsg msg = rsp.RoomInfoList[i];
                GameObject newRoomDetail = Instantiate(roomDetailPrefab, roomContentUI.transform);
                newRoomDetail.transform.localPosition = new Vector2(0, -i * roomDetailDistance);
                newRoomDetail.GetComponent<RoomScript>().InitDetail(msg.RoomId,msg.RoomName);
            }
        }else if ((int)param[0] == (int)SERVER_CMD.ServerEnterroomRsp)
        {
            EnterRoomRsp rsp = new EnterRoomRsp();
            rsp.MergeFrom((byte[])param[1]);
            if (rsp.ResCode == 1)
            {
                uiManager.InvokeCanvas(5);
                GameObject matchUI = uiManager.GetUI(5);
                if (matchUI is not null)
                {
                    matchUI.GetComponent<RoomScript>().InitMatch(rsp.RoomInfo.MaxNum,rsp.RoomInfo.RoomName);
                }
            }
            else
            {
                uiManager.InvokeCanvas(6);
                GameObject hintUI = uiManager.GetUI(6);
                if (hintUI is not null)
                {
                    hintUI.GetComponent<HintUIScript>().InitHint(rsp.ResStr,Color.gray,3);
                }
            }
        }
    }

    public void SetSceneId(int sceneId)
    {
        this.sceneId = sceneId;
        if (sceneId != -1)
        {
            sceneBriefPanel.GetComponent<RectTransform>().anchorMax = new Vector2(0.5f,1);
            
            sceneInfoPanel.SetActive(true);
            sceneNameText.text = "<rotate=90>" + sceneId2NameDict[sceneId];
        }
        else
        {
            sceneBriefPanel.GetComponent<RectTransform>().anchorMax = new Vector2(1,1);
            sceneInfoPanel.SetActive(false);
        }
        
        int childCount = roomContentUI.transform.childCount;
        while (childCount > 0)
        {
            Destroy(roomContentUI.transform.GetChild(0).gameObject);
            --childCount;
        }
        childCount = rankListContentUI.transform.childCount;
        while (childCount > 0)
        {
            Destroy(rankListContentUI.transform.GetChild(0).gameObject);
            --childCount;
        }
    }
}
