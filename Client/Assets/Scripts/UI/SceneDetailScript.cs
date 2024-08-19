using System.Collections;
using System.Collections.Generic;
using TCCamp;
using TMPro;
using UnityEngine;

public class SceneDetailScript : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI nameText;
    [SerializeField] private TextMeshProUGUI difficultText;
    [SerializeField] private TextMeshProUGUI ghostText;
    [SerializeField] private TextMeshProUGUI gargoyleText;
    
    private int sceneId;

    public void InitDetail(int sceneId, int difficult, int ghostNum, int gargoyleNum, string name)
    {
        nameText.text = name;
        this.sceneId = sceneId;
        difficultText.text = "难度： " + difficult.ToString();
        ghostText.text = "幽灵： " + ghostNum.ToString();
        gargoyleText.text = "石像鬼： " + gargoyleNum.ToString();
    }
    public void SelectScene()
    {
        gameObject.GetComponentInParent<MainUIScript>().SetSceneId(sceneId);
        Network network = GameObject.Find("GameManager").GetComponent<Network>();
        RoomInfoReq roomReq = new RoomInfoReq();
        roomReq.SceneId = sceneId;
        network.SendMsg(CLIENT_CMD.ClientRoomReq,roomReq);
        RankListReq rankReq = new RankListReq();
        rankReq.SceneId = sceneId;
        network.SendMsg(CLIENT_CMD.ClientRanklistReq,rankReq);
    }
}
