using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using Google.Protobuf;
using TCCamp;
using TMPro;
using UnityEngine;

public class BagScript : MonoBehaviour
{
    public GameObject itemPrefab;
    public GameObject ItemDetailUI;
    public int rowNum;

    [SerializeField] private TextMeshProUGUI MoneyText;
    private GameObject contentUI;
    private List<int> Index2IdList;
    private List<ItemScript> ItemList;
    private int checkIndex;
    private UIManager UImanager;
    private Network network;
    
    /// <summary>
    /// This function is called when the object becomes enabled and active.
    /// </summary>
    private void OnEnable()
    {
        network = GameObject.Find("GameManager").GetComponent<Network>();
        EventMgr.instance.AddListener("Network",InitItems);
        network.SendMsg(CLIENT_CMD.ClientGetbagitemsReq,null);
    }
    
    
    private void OnDisable()
    {
        EventMgr.instance.RemoveListener("Network",InitItems);
    }

    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        var visualElements = GetComponentsInChildren<RectTransform>();
        foreach (var visualElement in visualElements)
        {
            if (visualElement.gameObject.name == "Content")
            {
                contentUI = visualElement.gameObject;
                break;
            }
        }

        ItemList = new List<ItemScript>();
        Index2IdList = new List<int>();
        checkIndex = -1;
        UImanager = GameObject.Find("GameManager").GetComponent<UIManager>();
    }
    
    /// <summary>
    /// 移除选中物品.
    /// </summary>
    public void RemoveItem()
    {
        if (checkIndex == -1 || checkIndex >= Index2IdList.Count)
        {
            return;
        }
        int canvasid = UImanager.AddUI(ItemDetailUI);
        UImanager.GetUI(canvasid).GetComponent<ItemDetailScript>().Init(Index2IdList[checkIndex],false,canvasid);
        
    }

    /// <summary>
    /// 初始化背包布局.
    /// </summary>
    private void InitItems(params object[] args)
    {
        int cmd = (int)args[0];
        if (cmd == (int)SERVER_CMD.ServerGetbagitemsRsp)
        {
            checkIndex = -1;
            BagItemRsp rsp = new BagItemRsp();
            rsp.MergeFrom((byte[])args[1]);
            MoneyText.text = "金币: " + rsp.Money.ToString();
            int index = 0;
            for (int i = 0; i < rsp.Item.Count; i++)
            {
                if (rsp.Item[i].Num != 0)
                {
                    GameObject newItem = null;
                    if (index >= ItemList.Count)
                    {
                        newItem = Instantiate(itemPrefab,contentUI.transform);
                        ItemList.Add(newItem.GetComponent<ItemScript>());
                        Index2IdList.Add(rsp.Item[i].Id);
                    }
                    else
                    {
                        newItem = ItemList[index].gameObject;
                        Index2IdList[index] = rsp.Item[i].Id;
                    }
                    Vector2 position = new Vector2(newItem.GetComponent<RectTransform>().sizeDelta.x * (index % rowNum+0.5f),
                        -newItem.GetComponent<RectTransform>().sizeDelta.y * (index / rowNum+0.5f));
                    newItem.transform.localPosition = position;
                    newItem.GetComponent<ItemScript>().InitItem(gameObject,index,rsp.Item[i].Id,rsp.Item[i].Num);
                    index++;
                }
            }
            while (index < ItemList.Count)
            {
                GameObject gameObject = ItemList[index].gameObject;
                ItemList.RemoveAt(index);
                Index2IdList.RemoveAt(index);
                Destroy(gameObject);
            }
        }
    }

    /// <summary>
    /// 选中/取消选择物品.
    /// </summary>
    /// <param name="index">物品编号.</param>
    public void SetCheck(int index)
    {
        if (checkIndex != -1)
        {
            ItemList[checkIndex].SetCheck(false);
        }
        if (index == checkIndex)
        {
            checkIndex = -1;
            return;
        }
        if (ItemList.Count > index && index >= 0)
        {
            ItemList[index].SetCheck(true);
            checkIndex = index;
        }
    }

    /// <summary>
    /// 获取物品选中状态.
    /// </summary>
    /// <param name="index">物品编号.</param>
    /// <returns>返回是否选中.</returns>
    public bool GetCheck(int index)
    {
        return index==checkIndex;
    }

    /// <summary>
    /// 添加物品.
    /// </summary>
    /// <param name="id">物品Id.</param>
    /// <param name="Num">物品Id.</param>
    public void AddItem(int id,int Num)
    {
        
    }
    
    /// <summary>
    /// 关闭背包.
    /// </summary>
    public void Close()
    {
        UImanager.HideCanvas(10);
    }
}
