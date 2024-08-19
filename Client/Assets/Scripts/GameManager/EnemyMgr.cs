using System.Collections;
using System.Collections.Generic;
using System.Linq;
using Google.Protobuf;
using TCCamp;
using UnityEngine;

public class EnemyMgr : MonoBehaviour
{
    [SerializeField] private GameObject staticEnemy;
    [SerializeField] private GameObject dynamicEnemy;
    [SerializeField] private int staticEnemyNum;
    [SerializeField] private int dynamicEnemyNum;
    [SerializeField] private List<Vector3> staticEnemyPosList;
    [SerializeField] private List<Vector3> dynamicEnemyPosList;
    
    private Dictionary<int, EnemyController> enemyDict;
    private GameProcessManager gameProcessManager;
    private Network network;
    private bool isLoadFromNet;
    private int index;
    private Dictionary<SceneObjectType,MonsterAttributeMsg> enemyNetConfigDict;
    private float uploadTime = 1f;
    private int netId;

    private bool isInit;

    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        enemyDict = new Dictionary<int, EnemyController>();
        gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        network = GameObject.Find("GameManager").GetComponent<Network>();
        enemyNetConfigDict = new Dictionary<SceneObjectType,MonsterAttributeMsg>();
        EventMgr.instance.AddListener("LoginDone",OnLoginDone);
        index = 0;
        isLoadFromNet = false;
        isInit = false;
    }

    /// <summary>
    /// 根据配置动态生成敌人.
    /// </summary>
    public void GenerateEnemy()
    {
        if (isInit)
        {
            return;
        }
        if (gameProcessManager.isSingle)
        {
            GenerateEnemy_SingleMode();
            gameProcessManager.SetReady("EnemyMgr");
            isInit = true;
        }
        else
        {
            EventMgr.instance.AddListener("Network",OnRecvEnemyInfo);
            if (!isLoadFromNet)
            {
                network.SendMsg(CLIENT_CMD.ClientMonsterattributeReq,null);
            }
        }
    }

    private void GenerateEnemy_SingleMode()
    {
        if (staticEnemy is null && staticEnemyPosList.Count > 0)
        {
            Debug.LogWarning("StaticEnemyPrefabList is Empty");
            staticEnemyPosList.Clear();
        }
        int i = 0;
        for (i = 0; i < staticEnemyNum; i++)
        {
            GameObject enemy = Instantiate(staticEnemy);
            EnemyController enemyController = enemy.GetComponent<EnemyController>();
            enemyController.Init(i+index,staticEnemyPosList[i%staticEnemyPosList.Count]);
            enemyDict.Add(i+index,enemyController);
        }
        index += i;
        if (dynamicEnemy is null && dynamicEnemyPosList.Count > 0)
        {
            Debug.LogWarning("DynamicEnemyPrefabList is Empty");
            dynamicEnemyPosList.Clear();
        }
        for (i = 0; i < dynamicEnemyNum; i++)
        {
            GameObject enemy = Instantiate(dynamicEnemy);
            EnemyController enemyController = enemy.GetComponent<EnemyController>();
            enemyController.Init(i+index,dynamicEnemyPosList[i%dynamicEnemyPosList.Count]);
            enemyDict.Add(i+index,enemyController);
        }
        index += i;
    }
    private void GenerateEnemy_NetMode(SceneMonsterRsp rsp)
    {
        if (isInit)
        {
            return;
        }
        isInit = true;
        if (staticEnemy is null)
        {
            Debug.LogWarning("StaticEnemyPrefabList is Empty");
            return;
        }
        if (dynamicEnemy is null)
        {
            Debug.LogWarning("DynamicEnemyPrefabList is Empty");
            return;
        }
        int length = rsp.MonsterList.Count;
        for (int i = 0; i < length; i++)
        {
            SceneObjectMsg msg = rsp.MonsterList[i];
            if (msg.Type == SceneObjectType.Ghost)
            {
                GameObject enemy = Instantiate(dynamicEnemy);
                EnemyController enemyController = enemy.GetComponent<EnemyController>();
                enemyController.InitConfig(enemyNetConfigDict[msg.Type].Speed);
                enemyController.Init(
                    msg.Id,
                    new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z),
                    new Vector3(msg.Rotation.X, msg.Rotation.Y, msg.Rotation.Z));
                enemyDict.Add(msg.Id,enemyController);
            }else if (msg.Type == SceneObjectType.Gargoyle)
            {
                GameObject enemy = Instantiate(staticEnemy);
                EnemyController enemyController = enemy.GetComponent<EnemyController>();
                enemyController.InitConfig(enemyNetConfigDict[msg.Type].Speed);
                enemyController.Init(msg.Id,
                    new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z),
                    new Vector3(msg.Rotation.X, msg.Rotation.Y, msg.Rotation.Z));
                enemyDict.Add(msg.Id,enemyController);
            }
        }
    }

    private void LoadEnemyConfig(MonsterAttributeRsp rsp)
    {
        enemyNetConfigDict.Clear();
        int length = rsp.AttributeList.Count;
        for (int i = 0; i < length; i++)
        {
            enemyNetConfigDict.Add(rsp.AttributeList[i].Type, rsp.AttributeList[i]);
        }
        isLoadFromNet = true;
    }
    
    public void RequestRebirth(int id)
    {
        enemyDict[id].Init(id,dynamicEnemyPosList[id%dynamicEnemyPosList.Count]);
    }

    public void ClearAll()
    {
        isInit = false;
        foreach (var enemy in enemyDict)
        {
            Destroy(enemy.Value.gameObject);
        }
        enemyDict.Clear();
        if (!gameProcessManager.isSingle)
        {
            EventMgr.instance.RemoveListener("Network",OnRecvEnemyInfo);
            CancelInvoke("UploadAllEnemy");
        }
    }

    private void OnRecvEnemyInfo(params object[] args)
    {
        if ((int)args[0] == (int)SERVER_CMD.ServerMonsterattributeRsp)
        {
            MonsterAttributeRsp rsp = new MonsterAttributeRsp();
            rsp.MergeFrom((byte[])args[1]);
            LoadEnemyConfig(rsp);
            if (gameProcessManager.IsPreparing()&&!isInit)
            {
                network.SendMsg(CLIENT_CMD.ClientScenedetailReq,null);
            }
        }else if ((int)args[0] == (int)SERVER_CMD.ServerScenemonsterRsp)
        {
            SceneMonsterRsp rsp = new SceneMonsterRsp();
            rsp.MergeFrom((byte[])args[1]);
            GenerateEnemy_NetMode(rsp);
            InvokeRepeating("UploadAllEnemy",0,uploadTime);
            netId = rsp.NetId;
            gameProcessManager.SetReady("EnemyMgr");
        }else if ((int)args[0] == (int)SERVER_CMD.ServerMonstersynRsp)
        {
            MonstersSynMsg rsp = new MonstersSynMsg();
            rsp.MergeFrom((byte[])args[1]);
            UpdateAllEnemy(rsp);
        }
    }

    private void UpdateAllEnemy(MonstersSynMsg rsp)
    {
        int length = rsp.MonsterList.Count;
        for (int i = 0; i < length; i++)
        {
            MonsterSynMsg msg = rsp.MonsterList[i];
            if (enemyDict[msg.Id].GetDeath() && msg.Hp > 0)
            {
                enemyDict[msg.Id].Init(msg.Id,
                    new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z),
                    new Vector3(msg.Rotation.X, msg.Rotation.Y, msg.Rotation.Z));
            }
            enemyDict[msg.Id].UpdateStatue(
                msg.Hp,
                new Vector3(msg.Position.X,msg.Position.Y,msg.Position.Z),
                new Vector3(msg.Rotation.X,msg.Rotation.Y,msg.Rotation.Z),
                new Vector3(msg.TargetPos.X,msg.TargetPos.Y,msg.TargetPos.Z),
                rsp.Netid==netId
            );
        }
    }

    private void UploadAllEnemy()
    {
        MonstersSynMsg req = new MonstersSynMsg();
        foreach (var enemy in enemyDict)
        {
            MonsterSynMsg msg = new MonsterSynMsg();
            msg.Position = new Vec3Msg();
            msg.Rotation = new Vec3Msg();
            msg.Position.X = enemy.Value.gameObject.transform.position.x;
            msg.Position.Y = enemy.Value.gameObject.transform.position.y;
            msg.Position.Z = enemy.Value.gameObject.transform.position.z;
            msg.Rotation.X = enemy.Value.gameObject.transform.rotation.eulerAngles.x;
            msg.Rotation.Y = enemy.Value.gameObject.transform.rotation.eulerAngles.y;
            msg.Rotation.Z = enemy.Value.gameObject.transform.rotation.eulerAngles.z;
            msg.Id = enemy.Key;
            if (enemy.Value.GetDeath())
            {
                msg.Hp = 0;
            }
            else
            {
                msg.Hp = 1;
            }
            req.MonsterList.Add(msg);
        }
        network.SendMsg(CLIENT_CMD.ClientMonstersynReq,req);
    }

    private void OnLoginDone(params object[] args)
    {
        network.SendMsg(CLIENT_CMD.ClientMonsterattributeReq,null);
    }
}
