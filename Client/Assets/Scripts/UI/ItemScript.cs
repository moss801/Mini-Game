using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UIElements;

public class ItemScript : MonoBehaviour
{

    public Color defaultColor;
    public Color checkedColor;
    [SerializeField] private UnityEngine.UI.Image backImage;
    [SerializeField] private UnityEngine.UI.Image IconImage;
    [SerializeField] private TextMeshProUGUI NumText;
    
    
    private GameObject itemManager;
    private ItemMgr itemMgr;
    private int index;
    private int id;
    private int num;

    /// <summary>
    /// 勾选物品.
    /// </summary>
    public void Check()
    {
        if (id == -1) return;
        if (itemManager.gameObject.name == "Shop(Clone)")
        {
            itemManager.GetComponent<ShopScript>().SetCheck(index);
        }else if (itemManager.gameObject.name == "Bag(Clone)")
        {
            itemManager.GetComponent<BagScript>().SetCheck(index);
        }
    }

    /// <summary>
    /// 初始化物品框.
    /// </summary>
    /// <param name="itemManager">物品管理器.</param>
    /// <param name="sprite">图标.</param>
    /// <param name="index">物品位置.</param>
    /// <param name="id">物品Id.</param>
    public void InitItem(GameObject itemManager,int index,int id,int num)
    {
        if (itemMgr is null)
        {
            itemMgr = GameObject.Find("GameManager").GetComponent<ItemMgr>();
        }
        this.itemManager = itemManager;
        this.index = index;
        this.id = id;
        this.num = num;
        IconImage.sprite = itemMgr.GetIconById(id);
        GetComponentInChildren<Canvas>().worldCamera = itemManager.GetComponentInChildren<Canvas>().worldCamera;
        backImage.color = defaultColor;
        if (num == -1)
        {
            NumText.text = "";
        }
        else
        {
            NumText.text = num.ToString();
        }
    }

    /// <summary>
    /// 获取物品位置.
    /// </summary>
    /// <returns>物品位置.</returns>
    public int GetIndex()
    {
        return index;
    }

    public void SetCheck(bool isCheck)
    {
        if (isCheck)
        {
            backImage.color = checkedColor;
        }
        else
        {
            backImage.color = defaultColor;
        }
    }

    public void SetNum(int newNum)
    {
        num = newNum;
        NumText.text = num.ToString();
    }

    public int GetNum()
    {
        return num;
    }
}
