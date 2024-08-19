using System;
using System.Collections;
using System.Collections.Generic;
using Google.Protobuf;
using TCCamp;
using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Networking.Types;

public class CharacterMgr : MonoBehaviour
{
    [SerializeField] private GameObject Character;
    [SerializeField] private Vector3 startPosition;

    private const float DETECTMIN = 0.1f;
    
    private Network network;

    private int networkID;

    private PlayerController ControlCharacter;

    private Dictionary<int, PlayerController> CharacterDict;

    private Vector3 lastRotation;

    private Vector3 lastPosition;

    private int lastHp;

    private float lastSpeed;

    private GameProcessManager gameProcessManager;
    
    private bool isInit;
    private void Start()
    {
        CharacterDict = new Dictionary<int, PlayerController>();
        gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        isInit = false;
    }

    public PlayerController GetLocalPlayer()
    {
        return ControlCharacter;
    }
    
    // Start is called before the first frame update
    public void CreateCharacter()
    {
        if (gameProcessManager.isSingle)
        {
            ControlCharacter = Instantiate(Character).GetComponent<PlayerController>();
            ControlCharacter.gameObject.transform.position = startPosition;
            ControlCharacter.IsLocalControl = true;
            gameProcessManager.SetReady("CharacterMgr");
            isInit = true;
        }
        else
        {
            ControlCharacter = null;
            network = GameObject.Find("GameManager").GetComponent<Network>();
            EventMgr.instance.AddListener("Network",OnRecv);
            network.SendMsg(CLIENT_CMD.ClientCharacteraddReq,null);
        }
    }

    private void OnRecv(params object[] args)
    {
        if ((int)args[0] == (int)SERVER_CMD.ServerCharacteraddRsp)
        {
            if (isInit)
            {
                return;
            }

            isInit = true;
            CharacterStatueMsg msg = new CharacterStatueMsg();
            msg.MergeFrom((byte[])args[1]);
            networkID = msg.NetId;
            ControlCharacter = Instantiate(Character).GetComponent<PlayerController>();
            ControlCharacter.netId = networkID;
            ControlCharacter.IsLocalControl = true;
            lastRotation = new Vector3(msg.Rotation.X, msg.Rotation.Y, msg.Rotation.Z);
            ControlCharacter.meshGameObject.transform.rotation = Quaternion.Euler(lastRotation);
            lastPosition = new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z);
            ControlCharacter.gameObject.transform.position = lastPosition;
            lastHp = msg.Hp;
            ControlCharacter.Hp = lastHp;
            EventMgr.instance.DispatchEvent("HpChange", lastHp);
            lastSpeed = 0;
            ControlCharacter.maxSpeed = msg.MaxSpeed;
            if (msg.Hp==0)
            {
                ControlCharacter.Death();
            }
            else
            {
                ControlCharacter.Rebirth();
            }
            ControlCharacter.SetArmed(msg.IsArmed);
            ControlCharacter.SetImmunity(msg.IsImmunity);
            InvokeRepeating("UpdateStatue",0,0.5f);
            gameProcessManager.SetReady("CharacterMgr");
        }else if ((int)args[0] == (int)SERVER_CMD.ServerBroadcastCharacterstatue)
        {
            NetAsyncMsg msg = new NetAsyncMsg();
            msg.MergeFrom((byte[])args[1]);
            for (int i = 0; i < msg.CharacterStatueList.Count; i++)
            {
                CharacterStatueMsg characterStatueMsg = msg.CharacterStatueList[i];
                PlayerController NetCharacter = null;
                if (characterStatueMsg.NetId == networkID)
                {
                    NetCharacter = ControlCharacter;
                    gameProcessManager.ChangeScore(characterStatueMsg.Score-gameProcessManager.GetScore());
                }
                else if(CharacterDict.ContainsKey(characterStatueMsg.NetId))
                {
                    NetCharacter = CharacterDict[characterStatueMsg.NetId];
                }
                else
                {
                    NetCharacter = Instantiate(Character).GetComponent<PlayerController>();
                    NetCharacter.netId = characterStatueMsg.NetId;
                    CharacterDict.Add(characterStatueMsg.NetId, NetCharacter);
                    ConstraintSource constraintSource = new ConstraintSource();
                    constraintSource.sourceTransform = ControlCharacter.GetComponentInChildren<Camera>().gameObject.transform;
                    constraintSource.weight = 1;
                    NetCharacter.gameObject.GetComponentInChildren<LookAtConstraint>().AddSource(constraintSource);
                }
                if (characterStatueMsg.NetId != networkID)
                {
                    NetCharacter.targetPosition = new Vector3(characterStatueMsg.Position.X, characterStatueMsg.Position.Y, characterStatueMsg.Position.Z);
                    NetCharacter.meshGameObject.transform.eulerAngles = new Vector3(characterStatueMsg.Rotation.X, characterStatueMsg.Rotation.Y, characterStatueMsg.Rotation.Z);
                    NetCharacter.targetSpeed = characterStatueMsg.Speed;
                }
                NetCharacter.maxSpeed = characterStatueMsg.MaxSpeed;
                if (characterStatueMsg.IsDeath)
                {
                    NetCharacter.Death();
                }
                else
                {
                    NetCharacter.Rebirth();
                }
                NetCharacter.SetArmed(characterStatueMsg.IsArmed);
                NetCharacter.SetImmunity(characterStatueMsg.IsImmunity);
                if (NetCharacter.Hp != characterStatueMsg.Hp)
                {
                    if (NetCharacter.Hp > characterStatueMsg.Hp)
                    {
                        NetCharacter.GetAnimator().SetBool("beHit",true);
                    }
                    NetCharacter.Hp = characterStatueMsg.Hp;
                    if (characterStatueMsg.NetId != networkID)
                    {
                        NetCharacter.hpBar.value = NetCharacter.Hp;
                    }
                    else
                    {
                        EventMgr.instance.DispatchEvent("HpChange", NetCharacter.Hp);
                    }
                }
            }
        }else if ((int)args[0] == (int)SERVER_CMD.ServerCharacterremoveRsp)
        {
            RemoveCharacterRsp rsp = new RemoveCharacterRsp();
            rsp.MergeFrom((byte[])args[1]);
            if (CharacterDict.ContainsKey(rsp.NetId))
            {
                Destroy(CharacterDict[rsp.NetId].gameObject);
                CharacterDict.Remove(rsp.NetId);
            }
        }else if ((int)args[0] == (int)SERVER_CMD.ServerCharacteranimsynRsp)
        {
            CharacterAnimSynMsg msg = new CharacterAnimSynMsg();
            msg.MergeFrom((byte[])args[1]);
            if (CharacterDict.ContainsKey(msg.NetId))
            {
                CharacterDict[msg.NetId].isInteract = true;
                switch (msg.AnimtType)
                {
                    case CharacterAnimType.Opendoor:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("switchLightOn",true);
                        break;
                    case CharacterAnimType.Closedoor:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("switchLightOn",true);
                        break;
                    case CharacterAnimType.Turnonlight:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("switchLightOn",true);
                        break;
                    case CharacterAnimType.Turnofflight:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("switchLightOff",true);
                        break;
                    case CharacterAnimType.Openbox:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("openBox",true);
                        break;
                    case CharacterAnimType.Attack:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("isAttack",true);
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().showFireVfx();
                        break;
                    case CharacterAnimType.Hurt:
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().GetAnimator().SetBool("beHit",true);
                        CharacterDict[msg.NetId].GetComponent<PlayerController>().isInteract = true;
                        break;
                    default:
                        CharacterDict[msg.NetId].isInteract = false;
                        break;
                }
            }
        }
    }
    
    private void UpdateStatue()
    {
        if (ControlCharacter is null)
        {
            return;
        }

        bool isDirty = false;

        CharacterStatueReq msg = new CharacterStatueReq();
        msg.Position = new Vec3Msg();
        msg.Position.X = ControlCharacter.gameObject.transform.position.x;
        msg.Position.Y = ControlCharacter.gameObject.transform.position.y;
        msg.Position.Z = ControlCharacter.gameObject.transform.position.z;

        Vector3 vec = new Vector3(msg.Position.X, msg.Position.Y, msg.Position.Z);
        if ((vec - lastPosition).magnitude > DETECTMIN)
        {
            isDirty = true;
            lastPosition = vec;
        }

        msg.Rotation = new Vec3Msg();
        msg.Rotation.X = ControlCharacter.meshGameObject.transform.eulerAngles.x;
        msg.Rotation.Y = ControlCharacter.meshGameObject.transform.eulerAngles.y;
        msg.Rotation.Z = ControlCharacter.meshGameObject.transform.eulerAngles.z;
        vec = ControlCharacter.meshGameObject.transform.eulerAngles;
        if (Mathf.Abs(Vector3.Angle(vec, lastRotation)) > DETECTMIN)
        {
            isDirty = true;
            lastRotation = vec;
        }
        msg.Speed = ControlCharacter.currentSpeed;
        if (Mathf.Abs(ControlCharacter.currentSpeed - lastSpeed) > DETECTMIN)
        {
            isDirty = true;
            lastSpeed = ControlCharacter.currentSpeed;
        }
        msg.NetId = networkID;
        if (isDirty)
        {
            network.SendMsg(CLIENT_CMD.ClientUpdatestatueReq,msg);
        }
    }

    public void ClearAll()
    {
        isInit = false;
        foreach (var character in CharacterDict)
        {
            Destroy(character.Value.gameObject);
        }
        CharacterDict.Clear();
        if (ControlCharacter is not null)
        {
            Destroy(ControlCharacter.gameObject);
            ControlCharacter = null;
        }

        if (!gameProcessManager.isSingle)
        {
            EventMgr.instance.RemoveListener("Network",OnRecv);
            CancelInvoke("UpdateStatue");
        }
    }
}
