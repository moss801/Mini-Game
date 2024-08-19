using System.Collections;
using System.Collections.Generic;
using System.IO;
using Google.Protobuf;
using TCCamp;
using UnityEditor;
using UnityEngine;

public class Item
{
    public int id;
    public int price;
    public string desc;
    public string name;
    public string iconName;
}
public class ItemMgr : MonoBehaviour
{
    public string iconPath;

    private Dictionary<int,Item> itemConfigDict;
    
    private Network network;
    
    // Start is called before the first frame update
    private void Start()
    {
        EventMgr.instance.AddListener("Network",OnRecv);
        EventMgr.instance.AddListener("LoginDone",OnLoginDone);
        itemConfigDict = new Dictionary<int, Item>();
    }

    private void OnLoginDone(params object[] args)
    {
        network = GameObject.Find("GameManager").GetComponent<Network>();
        network.SendMsg(CLIENT_CMD.ClientItemconfigReq,null);
    }
    
    private void OnRecv(params object[] args)
    {
        if ((int)args[0] == (int)SERVER_CMD.ServerItemconfigRsp)
        {
            ItemConfigs msg = new ItemConfigs();
            msg.MergeFrom((byte[])args[1]);
            for (int i = 0; i < msg.Item.Count; i++)
            {
                ItemConfig itemConfig = msg.Item[i];
                Item item = new Item();
                item.id = itemConfig.Id;
                item.price = itemConfig.Price;
                item.desc = itemConfig.Introduce;
                item.name = itemConfig.Name;
                item.iconName = itemConfig.IconName;
                itemConfigDict.Add(item.id,item);
            }
            EventMgr.instance.RemoveListener("Network",OnRecv);
        }
    }

    public Item GetItemById(int id)
    {
        if (id <= itemConfigDict.Count && id >= 0)
        {
            return itemConfigDict[id];
        }
        return null;
    }

    public Sprite GetIconById(int id)
    {
        Sprite icon = Resources.Load<Sprite>(Path.Combine(iconPath, itemConfigDict[id].iconName));
        return icon;
    }
}
