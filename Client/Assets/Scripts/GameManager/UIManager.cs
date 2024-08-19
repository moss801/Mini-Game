using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Rendering.Universal;


public class UIManager : MonoBehaviour
{
    [SerializeField] private List<GameObject> canvasList;
    [SerializeField] private GameObject UICamera;
    
    private Dictionary<int, GameObject> canvaDict;
    private Dictionary<int, int> id2OrderDict;
    private Canvas mainCanvas;
    private int currentShowNum;
    private int maxNum;
    private int newId;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Awake()
    {
        canvaDict = new Dictionary<int, GameObject>();
        id2OrderDict = new Dictionary<int, int>();
        currentShowNum = 0;
        maxNum = 1;
        newId = canvasList.Count;
        UICamera = Instantiate(UICamera);
        mainCanvas = UICamera.GetComponentInChildren<Canvas>();
        InvokeCanvas(0);
        GameObject.Find("Main Camera").GetComponent<UniversalAdditionalCameraData>().cameraStack.Add(UICamera.GetComponentInChildren<Camera>());
    }

    /// <summary>
    /// 激活/置顶画布.
    /// </summary>
    /// <param name="id">画布ID.</param>
    public void InvokeCanvas(int id)
    {
        if (id < 0 || (!id2OrderDict.ContainsKey(id) && id >= canvasList.Count))
        {
            return;
        }

        if (!id2OrderDict.ContainsKey(id))
        {
            canvaDict.Add(id,Instantiate(canvasList[id],mainCanvas.transform));
            id2OrderDict.Add(id,-1);
        }
        if (id2OrderDict[id] == -1)
        {
            canvaDict[id].SetActive(true);
            currentShowNum++;
        }
        if (id2OrderDict[id] < maxNum-1)
        {
            Canvas canvas = canvaDict[id].GetComponentInChildren<Canvas>();
            canvas.overrideSorting = true;
            canvas.sortingOrder = maxNum;
            id2OrderDict[id] = maxNum;
            maxNum++;
        }
    }

    /// <summary>
    /// 隐藏画布.
    /// </summary>
    /// <param name="id">画布ID.</param>
    public void HideCanvas(int id)
    {
        if (!id2OrderDict.ContainsKey(id))
        {
            return;
        }

        if (id2OrderDict[id] == -1)
        {
            return;
        }
        if (id2OrderDict[id]+1 == maxNum)
        {
            maxNum--;
        }
        canvaDict[id].SetActive(false);
        id2OrderDict[id] = -1;
        currentShowNum--;
        if (currentShowNum == 0)
        {
            maxNum = 1;
        }
    }

    /// <summary>
    /// 隐藏全部.
    /// </summary>
    public void HideAll()
    {
        foreach (var canvaId in id2OrderDict.Keys.ToArray())
        {
            HideCanvas(canvaId);
        }
        maxNum = 1;
        currentShowNum = 0;
    }

    /// <summary>
    /// 判断界面是否打开.
    /// </summary>
    /// <param name="id">画布ID.</param>
    /// <returns>界面是否打开</returns>
    public bool IsOpen(int id)
    {
        if (!id2OrderDict.ContainsKey(id)) return false;
        return id2OrderDict[id] != -1;
    }

    public int AddUI(GameObject UIPrefab)
    {
        Canvas mainCanvas = UICamera.GetComponentInChildren<Canvas>();
        canvaDict.Add(newId,Instantiate(UIPrefab,mainCanvas.transform));
        id2OrderDict.Add(newId,-1);
        InvokeCanvas(newId);
        newId++;
        return newId-1;
    }

    public void RemoveUI(int id)
    {
        HideCanvas(id);
        canvaDict.Remove(id);
        id2OrderDict.Remove(id);
    }

    public GameObject GetUI(int id)
    {
        if (canvaDict.ContainsKey(id))
        {
            return canvaDict[id];
        }
        return null;
    }
}


