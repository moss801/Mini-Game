using System;
using System.Collections;
using System.Collections.Generic;
using System.Xml.Schema;
using TCCamp;
using UnityEngine;
using Random = UnityEngine.Random;

public class EnemyController : MonoBehaviour
{
    public int Hp;
    public int damage;
    public int value;
    public GameObject meshGameObject;
    [SerializeField] protected float rebirthTime;
    [HideInInspector] public int id;
    public SceneObjectType type;
    
    protected Animator EnemyAnimator;
    protected EnemyMgr enemyMgr;
    protected PlayerController lastMurder;
    protected Material dissolve;
    protected int currentHp;
    protected float currentEffect;
    protected bool isDeath;
    protected bool isLocalControl;
    protected bool isAuthority;
    [SerializeField] protected float maxSpeed;
    protected Vector3 targetPos;
    protected Vector3 serverPos;
    protected Vector3 serverRot;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    protected virtual void Start()
    {
        enemyMgr = GameObject.Find("GameManager").GetComponent<EnemyMgr>();
        LoadEnemyMesh();
        isAuthority = false;
    }

    public void InitConfig(float speed)
    {
        maxSpeed = speed;
    }

    public void Init(int id, Vector3 position)
    {
        this.id = id;
        gameObject.transform.position = position;
        serverPos = position;
        lastMurder = null;
        isDeath = false;
        currentEffect = 0;
        currentHp = Hp;
        if (dissolve)
        {
            dissolve.SetFloat("_DissolveAmount",currentEffect);
        }
        isLocalControl = GameObject.Find("GameManager").GetComponent<GameProcessManager>().isSingle;
        gameObject.SetActive(true);
        if (Hp == 0)
        {
            Death();
        }
    }
    
    public void Init(int id, Vector3 position,Vector3 rotation)
    {
        gameObject.transform.rotation = Quaternion.Euler(rotation);
        serverRot = rotation;
        Init(id,position);
    }
    
    /// <summary>
    /// 加载敌人模型.
    /// </summary>
    public void LoadEnemyMesh()
    {
        //meshGameObject = Instantiate(meshGameObject,gameObject.transform);
        EnemyAnimator = meshGameObject.GetComponent<Animator>();
        SkinnedMeshRenderer skinnedMeshRenderer = GetComponentInChildren<SkinnedMeshRenderer>();
        if (skinnedMeshRenderer.materials.Length > 0 )
        {
            dissolve = skinnedMeshRenderer.materials[0];
            dissolve.SetFloat("_DissolveAmount",0);
        }
    }

    

    public void beHit(int damage, PlayerController player)
    {
        if (!isLocalControl)
        {
            return;
        }
        if (currentHp == -1)
        {
            return;
        }

        lastMurder = player;
        currentHp -= damage;
        if (currentHp <= 0)
        {
            Death();
        }
    }
    
    public void Death()
    {
        if (!isDeath)
        {
            currentHp = 0;
            isDeath = true;
            if (isLocalControl && lastMurder is not null)
            {
                GameProcessManager gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
                gameProcessManager.ChangeScore(value);
                gameProcessManager.ChangeKill();
            }
            InvokeRepeating("DeathEffect",0,0.1f);
        }
    }

    private void DeathEffect()
    {
        currentEffect += 0.1f;
        if (currentEffect >= 1)
        {
            CancelInvoke("DeathEffect");
            gameObject.SetActive(false);
            if (isLocalControl)
            {
                Invoke("Rebirth",rebirthTime);
            }
        }
        dissolve.SetFloat("_DissolveAmount",currentEffect);
    }

    private void Rebirth()
    {
        enemyMgr.RequestRebirth(id);
    }

    public void UpdateStatue(int Hp,Vector3 pos, Vector3 rot,Vector3 tarPos,bool isAuthor)
    {
        isAuthority = isAuthor;
        targetPos = tarPos;
        serverPos = pos;
        serverRot = rot;
        currentHp = Hp;
        if (Hp == 0)
        {
            Death();
        }
    }

    public bool GetDeath()
    {
        return isDeath;
    }
}
