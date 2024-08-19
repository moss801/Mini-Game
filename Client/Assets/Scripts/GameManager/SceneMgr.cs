using System;
using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using UnityEngine;
using RPC = TCCamp.RPC;

public class SceneMgr : MonoBehaviour
{
    [SerializeField] private GameObject doorPrefab;
    [SerializeField] private GameObject lightPrefab;
    [SerializeField] private GameObject boxPrefab;
    [SerializeField] private GameObject trapSlowPrefab;
    [SerializeField] private GameObject trapHurtPrefab;
    [SerializeField] private GameObject finishLinePrefab;
    
    private GameProcessManager gameProcessManager;
    private Dictionary<SceneObjectType, Dictionary<int, int>> sceneObjectDict;
    private List<GameObject> sceneObjectList;
    private bool isInit;
    private GameObject generateFinishLine;
    private void Start()
    {
        gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        sceneObjectDict = new Dictionary<SceneObjectType, Dictionary<int, int>>();
        sceneObjectList = new List<GameObject>();
        isInit = false;
    }

    public void Init()
    {
        if (gameProcessManager.isSingle)
        {
            RecoverToSingle();
            gameProcessManager.SetReady("SceneMgr");
            isInit = true;
        }
        else
        {
            HideDefault();
            EventMgr.instance.AddListener("Network",OnRecvSceneMessage);
        }
    }

    private void OnRecvSceneMessage(params object[] args)
    {
        if ((int)args[0] == (int)SERVER_CMD.ServerSceneobjectRsp)
        {
            SceneObjectRsp rsp = new SceneObjectRsp();
            rsp.MergeFrom((byte[])args[1]);
            GenerateSceneObject(rsp);
            gameProcessManager.SetReady("SceneMgr");
        }else if ((int)args[0] == (int)SERVER_CMD.ServerRpcMsg)
        {
            RPC rpc = new RPC();
            rpc.MergeFrom((byte[])args[1]);
            UpdateObjectStatue(rpc);
        }
    }

    private void UpdateObjectStatue(RPC rpc)
    {
        switch (rpc.Cmd)
        {
            case RPC_CMD.TrapStatueEnable:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Trap][id]].GetComponent<Trap>().Attack();
                }
                break;
            case RPC_CMD.TrapStatueDisable:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Trap][id]].GetComponent<Trap>().Hide();
                }
                break;
            case RPC_CMD.BoxStatueOpen:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Box][id]].GetComponent<InteractBox>().InteractMethod();
                }
                break;
            case RPC_CMD.LightStatueOn:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Light][id]].GetComponent<InteractLight>().LightOn();
                }
                break;
            case RPC_CMD.LightStatueOff:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Light][id]].GetComponent<InteractLight>().LightOff();
                }
                break;
            case RPC_CMD.DoorStatueOpen:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Door][id]].GetComponent<InteractDoor>().OpenDoor();
                }
                break;
            case RPC_CMD.DoorStatueClose:
                foreach (var id in rpc.Param)
                {
                    sceneObjectList[sceneObjectDict[SceneObjectType.Door][id]].GetComponent<InteractDoor>().CloseDoor();
                }
                break;
        }
    }

    public void ClearAll()
    {
        isInit = false;
        foreach (var sceneObject in sceneObjectList)
        {
            Destroy(sceneObject);
        }
        sceneObjectList.Clear();
        sceneObjectDict.Clear();
        if (generateFinishLine is not null)
        {
            generateFinishLine = null;
        }
        EventMgr.instance.RemoveListener("Network",OnRecvSceneMessage);
    }

    private void HideDefault()
    {
        GameObject[] obj = FindObjectsOfType(typeof(GameObject)) as GameObject[];
        foreach (GameObject child in obj)
        {
            if (child.gameObject.CompareTag("Box")||child.gameObject.CompareTag("Door")||child.gameObject.CompareTag("Light")||child.gameObject.CompareTag("Trap")||child.gameObject.CompareTag("FinishLine"))
            {
                child.gameObject.SetActive(false);
            }
        }
    }
    
    private void RecoverToSingle()
    {
        GameObject[] obj = FindObjectsOfType(typeof(GameObject)) as GameObject[];
        foreach (GameObject child in obj)
        {
            if (child.gameObject.CompareTag("Box"))
            {
                child.gameObject.SetActive(true);
                child.GetComponent<InteractBox>().Recover();
            }
            else if(child.gameObject.CompareTag("Door")||child.gameObject.CompareTag("Light")||child.gameObject.CompareTag("Trap")||child.gameObject.CompareTag("FinishLine"))
            {
                child.gameObject.SetActive(true);
            }
        }
    }

    private void GenerateSceneObject(SceneObjectRsp rsp)
    {
        if (isInit)
        {
            return;
        }

        isInit = true;
        int length = rsp.ObjectList.Count;
        for (int i = 0; i < length; i++)
        {
            SceneObjectMsg msg = rsp.ObjectList[i];
            if (!sceneObjectDict.ContainsKey(msg.Type))
            {
                Dictionary<int, int> newTypeDict = new Dictionary<int, int>();
                sceneObjectDict.Add(msg.Type,newTypeDict);
            }
            sceneObjectDict[msg.Type].Add(msg.Id,sceneObjectList.Count);
            bool isSuccess = true;
            switch (msg.Type)
            {
                case SceneObjectType.Door:
                    sceneObjectList.Add(Instantiate(doorPrefab));
                    sceneObjectList[sceneObjectList.Count - 1].GetComponent<InteractItemBase>().itemId = msg.Id;
                    break;
                case SceneObjectType.Light:
                    sceneObjectList.Add(Instantiate(lightPrefab));
                    sceneObjectList[sceneObjectList.Count - 1].GetComponent<InteractItemBase>().itemId = msg.Id;
                    break;
                case SceneObjectType.Box:
                    sceneObjectList.Add(Instantiate(boxPrefab));
                    sceneObjectList[sceneObjectList.Count - 1].GetComponent<InteractItemBase>().itemId = msg.Id;
                    break;
                case SceneObjectType.Slime:
                    sceneObjectList.Add(Instantiate(trapSlowPrefab));
                    sceneObjectList[sceneObjectList.Count - 1].GetComponent<Trap>().id = msg.Id;
                    break;
                case SceneObjectType.Trap:
                    sceneObjectList.Add(Instantiate(trapHurtPrefab));
                    sceneObjectList[sceneObjectList.Count - 1].GetComponent<Trap>().id = msg.Id;
                    break;
                case SceneObjectType.Finishline:
                    generateFinishLine = Instantiate(finishLinePrefab);
                    generateFinishLine.gameObject.transform.position = new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z);
                    sceneObjectDict[msg.Type].Remove(msg.Id);
                    isSuccess = false;
                    break;
                default:
                    sceneObjectDict[msg.Type].Remove(msg.Id);
                    isSuccess = false;
                    break;
            }

            if (isSuccess)
            {
                sceneObjectList[sceneObjectList.Count - 1].transform.position = new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z);
                sceneObjectList[sceneObjectList.Count - 1].transform.rotation = Quaternion.Euler(msg.Rotation.X, msg.Rotation.Y, msg.Rotation.Z);
            }
        }
        
    }
}
