using System;
using System.Collections;
using System.Collections.Generic;
using TCCamp;

using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.UI;
using RPC = TCCamp.RPC;

public class PlayerController : MonoBehaviour
{
    [SerializeField] private GameObject weaponPrefab;
    public float initSpeed = 5f;
    [HideInInspector] public float maxSpeed;
    [SerializeField] private GameObject forceFieldObj;
    public GameObject meshGameObject;
    public GameObject cameraGameObject;
    public GameObject rightHandSlot;
    public AudioClip footStepSound;
    public bool IsLocalControl;
    public Slider hpBar;

    public float currentSpeed = 0f;
    public int Hp;
    public Vector3 targetPosition;
    public float targetSpeed;
    
    private Animator characterAnimator;
    private AudioSource audioSource;
    private GameProcessManager gameProcessManager;
    private Network network;
    private InputReceiver inputReceiver;
    private InteractItemBase interactItem;
    [HideInInspector] public int netId;
    private bool isImmunity;
    [HideInInspector] public bool isInteract;
    [HideInInspector] public bool isDeath;
    private bool isArmed;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    public void Awake()
    {
        LoadCharacter();
        GameObject gameManager = GameObject.Find("GameManager");
        gameProcessManager = gameManager.GetComponent<GameProcessManager>();
        network = gameManager.GetComponent<Network>();
        inputReceiver = gameManager.GetComponent<InputReceiver>();
        SetImmunity(false);
        isDeath = false;
        isInteract = false;
        interactItem = null;
        SetArmed(false);
        maxSpeed = initSpeed;
        
    }
    public void Start()
    {
        if (IsLocalControl)
        {
            cameraGameObject = Instantiate(cameraGameObject, gameObject.transform);
            hpBar.transform.localScale = Vector3.zero;
        }
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    private void Update ()
    {
        if (gameProcessManager.isGaming())
        {
            forceFieldObj.SetActive(isImmunity);
            if (IsLocalControl)
            {
                if (!isInteract&&!isDeath)
                {
                    UpdatePosition();
                    UpdateInteract();
                    UpdateAttack();
                    if (inputReceiver.IsThrow())
                    {
                        Throw();
                    }
                }
                
            }
            else
            {
                Vector3 nextPos = Vector3.Lerp(transform.position, targetPosition, 1.5f*Time.deltaTime);
                currentSpeed = (nextPos - transform.position).magnitude/Time.deltaTime;
                if (currentSpeed < 0.3f)
                {
                    currentSpeed = 0;
                }
                transform.position = nextPos;
            }
            characterAnimator.SetFloat("currentSpeed",currentSpeed);
        }
    }
    
    /// <summary>
    /// 接受输入，更新玩家位置.
    /// </summary>
    protected void UpdatePosition()
    {
        bool isMove = false;
        Vector3 direction = Vector3.zero;
        Vector3 input = inputReceiver.GetMoveDirection();
        isMove = (input.magnitude > 0.01f); 
        direction += input.y*cameraGameObject.transform.forward;
        direction += input.x*cameraGameObject.transform.right;
        if (isMove)
        {
            currentSpeed = maxSpeed*input.magnitude;
            direction.y = 0;
            meshGameObject.transform.forward = direction;
            gameObject.transform.Translate(meshGameObject.transform.forward*(currentSpeed*Time.deltaTime));
        }
        else
        {
            currentSpeed = 0;
        }
    }
    
    /// <summary>
    /// 加载角色模型.
    /// </summary>
    public void LoadCharacter()
    {
        //meshGameObject = Resources.Load<GameObject>("JohnLemon");
        //meshGameObject = Instantiate(meshGameObject,gameObject.transform);
        characterAnimator = meshGameObject.GetComponent<Animator>();
        audioSource = meshGameObject.AddComponent<AudioSource>();
        audioSource.clip = footStepSound;
        audioSource.playOnAwake = false;
        meshGameObject.AddComponent<AnimNotifyProcesser>();
    }

    /// <summary>
    /// 接受输入指令，进行角色与物品交互.
    /// </summary>
    private void UpdateInteract()
    {
        if (!inputReceiver.IsInteract())
        {
            return;
        }
        InteractImpl();
    }

    public void InteractImpl()
    {
        if (interactItem is null)
        {
            return;
        }
        ResetStatue();
        RPC rsp = new RPC();
        rsp.Param.Add(interactItem.itemId);
        switch (interactItem.interactType)
        {
            case InteractItemType.Door:
            {
                characterAnimator.SetBool("switchLightOn",true);
                if (((InteractDoor)interactItem).GetDoorOpen())
                {
                    rsp.Cmd = RPC_CMD.DoorStatueClose;
                }else{
                    rsp.Cmd = RPC_CMD.DoorStatueOpen;
                }
                break;
            }
            case InteractItemType.Box:
            {
                if (((InteractBox)interactItem).GetBoxOpen())
                {
                    return;
                }
                characterAnimator.SetBool("openBox",true);
                rsp.Cmd = RPC_CMD.BoxStatueOpen;
                break;
            }
            case InteractItemType.Light:
            {
                if (((InteractLight)interactItem).GetLightStatue())
                {
                    characterAnimator.SetBool("switchLightOff",true);
                    rsp.Cmd = RPC_CMD.LightStatueOff;
                }
                else
                {
                    characterAnimator.SetBool("switchLightOn",true);
                    rsp.Cmd = RPC_CMD.LightStatueOn;
                }
                break;
            }
            default:
            {
                return;
            }
        }

        if (gameProcessManager.isSingle)
        {
            interactItem.InteractMethod();
            isInteract = true;
        }
        else
        {
            SendRPCMessage(rsp);
        }
    }

    private void UpdateAttack()
    {
        if (isArmed && inputReceiver.IsAttack())
        {
            Attack();
        }
    }
    
    public void AddCamera()
    {
        if (IsLocalControl)
        {
            cameraGameObject = Instantiate(cameraGameObject, gameObject.transform);
        }
    }

    private void OnTriggerEnter(Collider other)
    {
        if (!IsLocalControl)
        {
            return;
        } 
        interactItem = other.gameObject.GetComponent<InteractItemBase>();
        if(gameProcessManager.isSingle)
        {
            if (other.gameObject.CompareTag("FinishLine"))
            {
                gameProcessManager.SetGameResult(true);
            }
        }
        else
        {
            CollisionReq req = new CollisionReq();
            req.TypeA = SceneObjectType.Player;
            req.IdA = netId;
            InteractItemBase item = other.gameObject.GetComponent<InteractItemBase>();
            if (item is not null)
            {
                switch (item.interactType)
                {
                    case InteractItemType.Box:
                        req.TypeB = SceneObjectType.Box;
                        break;
                    case InteractItemType.Door:
                        req.TypeB = SceneObjectType.Door;
                        break;
                    case InteractItemType.Light:
                        req.TypeB = SceneObjectType.Light;
                        break;
                }
                req.IdB = item.itemId;
                network.SendMsg(CLIENT_CMD.ClientCollisioncheckReq,req);
            }else if (other.gameObject.CompareTag("Enemy"))
            {
                EnemyController enemyController = other.gameObject.GetComponent<EnemyController>();
                req.TypeB = enemyController.type;
                req.IdB = enemyController.id;
                network.SendMsg(CLIENT_CMD.ClientCollisioncheckReq,req);
            }else if (other.gameObject.CompareTag("FinishLine"))
            {
                req.TypeB = SceneObjectType.Finishline;
                network.SendMsg(CLIENT_CMD.ClientCollisioncheckReq,req);
            }
        }
    }

    private void OnTriggerExit(Collider other)
    {
        interactItem = null;
    }

    private void OnTriggerStay(Collider other)
    {
        if (gameProcessManager.isSingle && IsLocalControl && other.gameObject.CompareTag("Enemy"))
        {
            EnemyController enemyController = other.gameObject.GetComponent<EnemyController>();
            if (enemyController is not null && enemyController.damage>0)
            {
                Hurt(enemyController.damage);
            }
        }
    }

    public void Hurt(int damage)
    {
        if (!gameProcessManager.isSingle||isImmunity||isDeath)
        {
            return;
        }
        SetImmunity(true);
        Hp -= damage;
        EventMgr.instance.DispatchEvent("HpChange",Hp);
        if (Hp <= 0)
        {
            Hp = 0;
            SetImmunity(false);
            Death();
        }
        else
        {
            characterAnimator.SetBool("beHit",true);
            isInteract = true;
            Invoke("CancelImmunity",2);
        }
    }
    
    private void ResetStatue()
    {
        currentSpeed = 0;
        isInteract = false;
        if (characterAnimator is not null)
        {
            characterAnimator.SetFloat("currentSpeed",currentSpeed);
        }
    }
    
    public void Death()
    {
        if (isDeath)
        {
            return;
        }
        ResetStatue();
        isDeath = true;
        characterAnimator.SetBool("isDie",true);
        if (gameProcessManager.isSingle)
        {
            Invoke("CheckNeedEnd",3);
        }
    }

    private void CheckNeedEnd()
    {
        gameProcessManager.SetGameResult(false);
    }

    public void Attack()
    {
        ResetStatue();
        GunController gun = rightHandSlot.transform.GetChild(0).GetComponent<GunController>();
        characterAnimator.SetBool("isAttack",true);
        isInteract = true;
        gun.ShowVFX();
        if (gameProcessManager.isSingle)
        {
            gun.fire(this);
        }
        else
        {
            gun.fire(this,network);
        }
    }

    public void showFireVfx()
    {
        rightHandSlot.transform.GetChild(0).GetComponent<GunController>().ShowVFX();
    }

    public void Pick(GameObject obj)
    {
        if (rightHandSlot.transform.childCount > 0)
        {
            Destroy(rightHandSlot.transform.GetChild(0));
        }
        Instantiate(obj, rightHandSlot.transform);
    }

    public void Throw()
    {
        if (isArmed && rightHandSlot.transform.childCount > 0)
        {
            if (!gameProcessManager.isSingle)
            {
                RPC req = new RPC();
                req.Cmd = RPC_CMD.ThrowWeapon;
                SendRPCMessage(req);
            }
            else
            {
                SetArmed(false);
            }
        }
    }

    public void AddSpeedDebuff(float val,float lastTime)
    {
        CancelInvoke("CancelSpeedDebuff");
        maxSpeed = val * initSpeed;
        Invoke("CancelSpeedDebuff",lastTime);
    }
    
    private void CancelImmunity()
    {
        isImmunity = false;
    }

    private void CancelSpeedDebuff()
    {
        maxSpeed = initSpeed;
    }

    private void SendRPCMessage(RPC req)
    {
        network.SendMsg(CLIENT_CMD.ClientRpcReq, req);
    }

    public void SetArmed(bool isArm)
    {
        if (isArm == isArmed)
        {
            return;
        }
        isArmed = isArm;
        characterAnimator.SetBool("isArmed",isArmed);
        if (isArmed)
        {
            Pick(weaponPrefab);
        }
        else
        {
            Destroy(rightHandSlot.transform.GetChild(0).gameObject);
        }

        if (IsLocalControl)
        {
            UIManager uiManager = GameObject.Find("GameManager").GetComponent<UIManager>();
            if (uiManager.GetUI(2) is null)
            {
                uiManager.InvokeCanvas(2);
                uiManager.HideCanvas(2);
            }
            CharacterUIScript uiScript = uiManager.GetUI(2).GetComponent<CharacterUIScript>();
            uiScript.attackButton.gameObject.SetActive(isArmed);
            uiScript.throwButton.gameObject.SetActive(isArmed);
        }
    }

    public bool GetArmed()
    {
        return isArmed;
    }

    public void SetImmunity(bool enable)
    {
        isImmunity = enable;
    }

    public void Rebirth()
    {
        if (!isDeath)
        {
            return;
        }
        isDeath = false;
        characterAnimator.SetBool("isDie",isDeath);
    }
    
    public Animator GetAnimator()
    {
        return characterAnimator;
    }
}
