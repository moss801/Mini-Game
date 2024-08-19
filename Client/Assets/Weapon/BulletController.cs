using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using TCCamp;
using UnityEngine;

public class BulletController : MonoBehaviour
{
    [SerializeField] private GameObject mesh;
    [SerializeField] private GameObject hitVFX;
    [HideInInspector] public int Id;
    [HideInInspector] public Vector3 serverPos;
    
    private int damage;
    private float speed;
    [SerializeField] private Vector3 direction;
    private BulletPool pool;
    private PlayerController belongPlayer;
    private ParticleSystem hitparticleSystem;
    private bool waitRecycle;

    private void Start()
    {
        pool = GameObject.Find("GameManager").GetComponent<BulletPool>();
        speed = pool.GetSpeed();
        damage = pool.GetDamage();
    }

    public void ResetDirect(Vector3 rot)
    {
        direction = rot;
    }
    
    public void SetServerPosition(Vector3 pos)
    {
        serverPos = pos;
    }

    public void Init(Vector3 rotation, Vector3 position, PlayerController belong=null)
    {
        waitRecycle = false;
        belongPlayer = belong;
        gameObject.transform.position = position;
        //gameObject.transform.rotation = Quaternion.Euler(rotation);
        direction = rotation;
        mesh.transform.forward = direction;
        hitVFX.transform.forward = direction;
        if (hitparticleSystem is null)
        {
            hitparticleSystem = hitVFX.GetComponent<ParticleSystem>();
            
        }
        hitparticleSystem.Stop();
        gameObject.SetActive(true);
    }

    // Update is called once per frame
    private void Update()
    {
        if (pool.isNetMode)
        {
            //gameObject.transform.position = serverPos;
            serverPos += direction.normalized*(speed*Time.deltaTime);
            gameObject.transform.position = Vector3.Lerp(gameObject.transform.position, serverPos, Time.deltaTime);
        }
        else
        {
            gameObject.transform.position += direction.normalized*(speed*Time.deltaTime);
        }

        if (!waitRecycle)
        {
            CheckHit();
        }
    }

    private bool CheckHit()
    {
        RaycastHit raycastHit;
        if (Physics.Raycast(transform.position, direction, out raycastHit))
        {
            if (raycastHit.distance < speed * Time.deltaTime)
            {
                OnHit(raycastHit.collider);
                return true;
            }
        }
        return false;
    }
    
    private void OnHit(Collider other)
    {
        if (other.gameObject.CompareTag("Enemy"))
        {
            waitRecycle = true;
            EnemyController enemyController = other.gameObject.GetComponent<EnemyController>();
            if (pool.isNetMode)
            {
                CollisionReq req = new CollisionReq();
                req.TypeA = SceneObjectType.Bullet;
                req.IdA = Id;
                req.TypeB = enemyController.type;
                req.IdB = enemyController.id;
                pool.GetNetWork().SendMsg(CLIENT_CMD.ClientCollisioncheckReq,req);
            }
            else
            {
                enemyController.beHit(damage,belongPlayer);
                Invoke("Recycle",0.5f);
            }
            hitparticleSystem.Simulate(0.0f);
            hitparticleSystem.Play();
        }
        else if(other.gameObject.CompareTag("Door")||other.gameObject.CompareTag("StaticMesh")||other.gameObject.CompareTag("Box"))
        {
            waitRecycle = true;
            BulletsSynMsg msgs = new BulletsSynMsg();
            BulletSynMsg msg = new BulletSynMsg();
            msg.Id = Id;
            msg.IsActive = false;
            msgs.BulletList.Add(msg);
            pool.GetNetWork().SendMsg(CLIENT_CMD.ClientBulletsynReq,msgs);
            hitparticleSystem.Simulate(0.0f);
            hitparticleSystem.Play();
            Invoke("Recycle",0.5f);
        }
    }

    private void OnEnable()
    {
        if (hitparticleSystem is null)
        {
            hitparticleSystem = hitVFX.GetComponent<ParticleSystem>();
        }
        hitparticleSystem.Simulate(0.0f);
        hitparticleSystem.Stop();
    }

    private void Recycle()
    {
        gameObject.SetActive(false);
        pool.ReturnBullet(this);
    }
}
