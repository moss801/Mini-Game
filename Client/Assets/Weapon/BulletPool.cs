using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using UnityEngine;

public class BulletPool : MonoBehaviour
{
    [SerializeField] private float speed;
    [SerializeField] private int damage;
    [SerializeField] private int defaultPoolSize;
    [SerializeField] private GameObject bullet;
    
    private List<bool> usedStatueList;
    private List<BulletController> bulletPool;
    private Dictionary<int, int> bulletIdDict;
    private Network network;
    [HideInInspector] public bool isNetMode;
    
    // Start is called before the first frame update
    private void Start()
    {
        usedStatueList = new List<bool>();
        bulletIdDict = new Dictionary<int, int>();
        bulletPool = new List<BulletController>();
        network = GameObject.Find("GameManager").GetComponent<Network>();
    }

    public void InitPool()
    {
        GameProcessManager gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        isNetMode = !gameProcessManager.isSingle;
        if (!isNetMode)
        {
            for (int i = 0; i < defaultPoolSize; i++)
            {
                GameObject newBullet = Instantiate(bullet);
                newBullet.SetActive(false);
                BulletController bulletController = newBullet.GetComponent<BulletController>();
                bulletController.Id = i;
                usedStatueList.Add(true);
                bulletPool.Add(bulletController);
                bulletIdDict[i] = i;
                gameProcessManager.SetReady("BulletPool");
            }
        }
        else
        {
            EventMgr.instance.AddListener("Network",OnRecvBulletSyn);
            gameProcessManager.SetReady("BulletPool");
        }
    }

    public BulletController GetBullet()
    {
        for (int i = 0; i < usedStatueList.Count; i++)
        {
            if (usedStatueList[i])
            {
                usedStatueList[i] = false;
                return bulletPool[i];
            }
        }
        GameObject newBullet = Instantiate(bullet);
        BulletController bulletController = newBullet.GetComponent<BulletController>();
        bulletController.Id = usedStatueList.Count;
        bulletIdDict[bulletController.Id] = usedStatueList.Count;
        usedStatueList.Add(false);
        bulletPool.Add(bulletController);
        return newBullet.GetComponent<BulletController>();
    }

    private void OnRecvBulletSyn(params object[] args)
    {
        if ((int)args[0] == (int)SERVER_CMD.ServerBulletsynRsp)
        {
            BulletsSynMsg msgs = new BulletsSynMsg();
            msgs.MergeFrom((byte[])args[1]);
            int length = msgs.BulletList.Count;
            for (int i = 0; i < length; i++)
            {
                BulletController bulletController = null;
                BulletSynMsg msg = msgs.BulletList[i];
                if (!bulletIdDict.ContainsKey(msg.Id))
                {
                    bulletIdDict.Add(msg.Id,bulletPool.Count);
                    GameObject newBullet = Instantiate(bullet);
                    bulletController = newBullet.GetComponent<BulletController>();
                    bulletPool.Add(bulletController);
                    usedStatueList.Add(false);
                    bulletController.Init(
                        new Vector3(msg.Rotation.X,msg.Rotation.Y,msg.Rotation.Z),
                        new Vector3(msg.Position.X,msg.Position.Y,msg.Position.Z)
                    );
                    bulletController.SetServerPosition(new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z));
                    bulletController.Id = msg.Id;
                }
                else
                {
                    bulletController = bulletPool[bulletIdDict[msg.Id]];
                    bulletController.SetServerPosition(new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z));
                    bulletController.ResetDirect(new Vector3(msg.Rotation.X,msg.Rotation.Y,msg.Rotation.Z));
                }
                bulletController.gameObject.SetActive(msg.IsActive);
            }
        }
    }

    public void ReturnBullet(BulletController bulletController)
    {
        usedStatueList[bulletIdDict[bulletController.Id]] = true;
    }

    public float GetSpeed()
    {
        return speed;
    }
    public int GetDamage()
    {
        return damage;
    }

    public void ClearAll()
    {
        foreach (var bulletInst in bulletPool)
        {
            Destroy(bulletInst.gameObject);
        }
        usedStatueList.Clear();
        bulletPool.Clear();
        bulletIdDict.Clear();
        if (isNetMode)
        {
            EventMgr.instance.RemoveListener("Network", OnRecvBulletSyn);
        }
    }

    public Network GetNetWork()
    {
        return network;
    }
}
