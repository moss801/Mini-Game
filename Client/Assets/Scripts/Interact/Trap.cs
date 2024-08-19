using System;
using System.Collections;
using System.Collections.Generic;
using TCCamp;
using UnityEngine;

public enum TrapType
{
    SpeedDebuff = 1,
    Hurt = 2
}
public class Trap : MonoBehaviour
{
    [SerializeField] private TrapType type;

    [SerializeField] private float cycleTime;

    [SerializeField] private float damage;

    [SerializeField] private float debuffLastTime;

    [SerializeField] private GameObject weapon;

    [HideInInspector] public int id;
    
    private PlayerController player;

    private bool isAttack;

    private bool isCountDownAttack;
    private bool isCountDownHide;
    private bool isNetMode;
    private Network network;
    // Start is called before the first frame update
    private void Start()
    {
        isCountDownAttack = false;
        isCountDownHide = false;
        isNetMode = !GameObject.Find("GameManager").GetComponent<GameProcessManager>().isSingle;
        network = GameObject.Find("GameManager").GetComponent<Network>();
        player = null;
        if (cycleTime <= 0)
        {
            isAttack = true;
        }
        else
        {
            isAttack = false;
            Invoke("Attack",cycleTime);
        }
    }

    private void OnTriggerStay(Collider other)
    {
        if (isAttack)
        {
            if (player is not null)
            {
                switch (type)
                {
                    case TrapType.SpeedDebuff:
                        player.AddSpeedDebuff(damage,debuffLastTime);
                        break;
                    case TrapType.Hurt:
                        player.Hurt((int)damage);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    public void Attack()
    {
        if (isCountDownAttack)
        {
            CancelInvoke("Attack");
            isCountDownAttack = false;
        }

        if (isCountDownHide)
        {
            CancelInvoke("Hide");
            isCountDownHide = false;
        }
        isAttack = true;
        weapon.SetActive(true);
        Invoke("Hide",cycleTime);
        isCountDownHide = true;
    }

    public void Hide()
    {
        if (isCountDownAttack)
        {
            CancelInvoke("Attack");
            isCountDownAttack = false;
        }

        if (isCountDownHide)
        {
            CancelInvoke("Hide");
            isCountDownHide = false;
        }
        isAttack = false;
        weapon.SetActive(false);
        Invoke("Attack",cycleTime);
        isCountDownAttack = true;
    }

    private void OnTriggerEnter(Collider other)
    {
        if (other.gameObject.CompareTag("Player") && other.gameObject.GetComponent<PlayerController>().IsLocalControl)
        {
            if (isNetMode)
            {
                CollisionReq req = new CollisionReq();
                req.TypeA = SceneObjectType.Player;
                req.IdA = other.gameObject.GetComponent<PlayerController>().netId;
                req.IdB = id;
                switch (type)
                {
                    case TrapType.SpeedDebuff:
                        req.TypeB = SceneObjectType.Slime;
                        break;
                    case TrapType.Hurt:
                        req.TypeB = SceneObjectType.Trap;
                        break;
                }
                network.SendMsg(CLIENT_CMD.ClientCollisioncheckReq,req);
            }
            else
            {
                player = other.gameObject.GetComponent<PlayerController>();
            }
        }
    }

    private void OnTriggerExit(Collider other)
    {
        if (player is null)
        {
            return; 
        }
        if (other.gameObject == player.gameObject)
        {
            player = null;
        }
    }
}
