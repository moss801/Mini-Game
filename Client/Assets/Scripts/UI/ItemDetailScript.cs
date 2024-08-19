using System;
using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class ItemDetailScript : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI titleText;
    [SerializeField] private TextMeshProUGUI nameText;
    [SerializeField] private Image icon;
    [SerializeField] private TextMeshProUGUI descText;
    [SerializeField] private TextMeshProUGUI priceText;
    [SerializeField] private Slider numSelecter;
    [SerializeField] private TextMeshProUGUI hintText;
    
    private int price;
    private int id;
    private bool isAdd;
    private int canvasId;
    private Network network;
    
    public void Init(int id, bool isAdd, int canvasId)
    {
        ItemMgr itemMgr = GameObject.Find("GameManager").GetComponent<ItemMgr>();
        network = GameObject.Find("GameManager").GetComponent<Network>();
        this.id = id;
        Item item = itemMgr.GetItemById(id);
        if (item is null)
        {
            return;
        }
        nameText.text = item.name;
        descText.text = item.desc;
        priceText.text = "0";
        icon.sprite = itemMgr.GetIconById(id);
        numSelecter.value = 0;
        numSelecter.minValue = 0;
        numSelecter.maxValue = 100;
        price = item.price;
        this.isAdd = isAdd;
        this.canvasId = canvasId;
        if (this.isAdd)
        {
            titleText.text = "买";
        }
        else
        {
            titleText.text = "丢弃";
        }
        EventMgr.instance.AddListener("Network",OnRecv);
    }

    public void Onclick()
    {
        if (isAdd)
        {
            BuyReq req = new BuyReq();
            req.Id = id;
            req.Num = (int)numSelecter.value;
            network.SendMsg(CLIENT_CMD.ClientBuyitemReq,req);
        }
        else
        {
            DeleteItemReq req  = new DeleteItemReq();
            req.Id = id;
            req.Num = (int)numSelecter.value;
            network.SendMsg(CLIENT_CMD.ClientDeleteitemReq,req);
        }
    }

    public void OnValueChange()
    {
        int num = (int)numSelecter.value;
        if (isAdd)
        {
            priceText.text = "数量: " + num.ToString() + ", 总价: " + (num * price).ToString();
        }
        else
        {
            priceText.text = "数量: " + num.ToString();
        }
    }

    public void Close()
    {
        EventMgr.instance.RemoveListener("Network",OnRecv);
        GameObject.Find("GameManager").GetComponent<UIManager>().RemoveUI(canvasId);
    }

    public void OnRecv(params object[] args)
    {
        int cmd = (int)args[0];
        if (cmd == (int)SERVER_CMD.ServerBuyitemRsp)
        {
            StatueCodeRsp rsp = new StatueCodeRsp();
            rsp.MergeFrom((byte[])args[1]);
            if (rsp.Code == StatueCode.StatueSuccess || rsp.Code == StatueCode.StatueBuyItemnotenough)
            {
                network.SendMsg(CLIENT_CMD.ClientGetshopitemsReq,null);
                network.SendMsg(CLIENT_CMD.ClientGetbagitemsReq,null);
            }

            switch (rsp.Code)
            {
                case StatueCode.StatueSuccess:
                {
                    hintText.text = "购买成功";
                    break;
                }
                case StatueCode.StatueBuyItemnotenough:
                {
                    hintText.text = "道具数量不足";
                    break;
                }
                case StatueCode.StatueMoneyNotenough:
                {
                    hintText.text = "金币不足";
                    break;
                }
            }
            Debug.Log(rsp);
        }else if (cmd == (int)SERVER_CMD.ServerDeleteitemRsp)
        {
            StatueCodeRsp rsp = new StatueCodeRsp();
            rsp.MergeFrom((byte[])args[1]);
            if (rsp.Code == StatueCode.StatueSuccess)
            {
                network.SendMsg(CLIENT_CMD.ClientGetbagitemsReq,null);
            }
            switch (rsp.Code)
            {
                case StatueCode.StatueSuccess:
                {
                    hintText.text = "丢弃成功";
                    break;
                }
                case StatueCode.StatueDeleteItemnotenough:
                {
                    hintText.text = "道具数量不足";
                    break;
                }
            }
            Debug.Log(rsp);
        }
    }
}
