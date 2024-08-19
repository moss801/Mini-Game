using System;
using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using TMPro;
using UnityEngine;
public class RoomScript : MonoBehaviour
{

    [SerializeField] private TextMeshProUGUI roomNameText;
    [SerializeField] private TextMeshProUGUI currentNumText;
    [SerializeField] private TextMeshProUGUI totalNumText;
    
    private const int coolDownTime = 3;
    private int roomId;
    private bool isCoolDown;
    private bool isGaming;
    
    public void InitMatch(int maxNum, string roomName)
    {
        EventMgr.instance.AddListener("Network",OnRecvRoomInfo);
        currentNumText.text = "1";
        roomNameText.text = roomName;
        totalNumText.text = "/" + maxNum.ToString();
        isGaming = false;
    }

    public void InitDetail(int roomId, string roomName)
    {
        roomNameText.text = roomName;
        this.roomId = roomId;
        isCoolDown = false;
    }
    
    
    public void ExitRoom()
    {
        GetNetworkManager().SendMsg(CLIENT_CMD.ClientExitroomReq,null);
        GameObject.Find("GameManager").GetComponent<UIManager>().HideCanvas(5);
    }

    public void EnterRoom()
    {
        if (isCoolDown)
        {
            return;
        }
        isCoolDown = true;
        EnterRoomReq req = new EnterRoomReq();
        req.RoomId = roomId;
        GetNetworkManager().SendMsg(CLIENT_CMD.ClientEnterroomReq,req);
        Invoke("EndCoolDown",coolDownTime);
    }

    private Network GetNetworkManager()
    {
        Network network = GameObject.Find("GameManager").GetComponent<Network>();
        return network;
    }

    private void EndCoolDown()
    {
        isCoolDown = false;
    }

    private void OnRecvRoomInfo(params object[] param)
    {
        if ((int)param[0] == (int)SERVER_CMD.ServerRoomnumchangeRsp)
        {
            RoomNumChange roomNumChange = new RoomNumChange();
            roomNumChange.MergeFrom((byte[])param[1]);
            currentNumText.text = roomNumChange.CurrentNum.ToString();
        }else if ((int)param[0] == (int)SERVER_CMD.ServerRoomreadyRsp)
        {
            if (isGaming)
            {
                return;
            }
            isGaming = true;
            GameProcessManager gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
            EventMgr.instance.RemoveListener("Network",OnRecvRoomInfo);
            gameProcessManager.isSingle = false;
            gameProcessManager.StartGame();
        }
    }
}
