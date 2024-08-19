using System;
using System.Collections;
using System.Collections.Generic;
using TCCamp;
using UnityEngine;

public class GunController : MonoBehaviour
{
    [SerializeField] private float coolDown;
    [SerializeField] private GameObject bulletSpawnSlot;
    [SerializeField] private GameObject muzzleObject;

    private ParticleSystem fireParticleSystem;
    private BulletPool bulletPool;
    private float timer;
    // Start is called before the first frame update
    void Start()
    {
        bulletPool = GameObject.Find("GameManager").GetComponent<BulletPool>();
        timer = coolDown;
        fireParticleSystem = muzzleObject.GetComponent<ParticleSystem>();
        fireParticleSystem.Stop();
    }

    private void Update()
    {
        timer += Time.deltaTime;
    }

    public void fire(PlayerController player)
    {
        if (timer > coolDown)
        {
            timer = 0;
            BulletController bulletController = bulletPool.GetBullet();
            bulletController.Init(player.meshGameObject.transform.forward.normalized, bulletSpawnSlot.transform.position, player);
        }
    }

    public void fire(PlayerController player, Network network)
    {
        AttackReq req = new AttackReq();
        req.Rotation = new Vec3Msg();
        req.Position = new Vec3Msg();
        req.Position.X = bulletSpawnSlot.transform.position.x;
        req.Position.Y = bulletSpawnSlot.transform.position.y;
        req.Position.Z = bulletSpawnSlot.transform.position.z;
        Vector3 rotation = player.meshGameObject.transform.forward.normalized;
        req.Rotation.X = rotation.x;
        req.Rotation.Y = rotation.y;
        req.Rotation.Z = rotation.z;
        req.Id = player.netId;
        network.SendMsg(CLIENT_CMD.ClientAttackReq,req);
    }

    public void ShowVFX()
    {
        fireParticleSystem.Simulate(0.0f);
        fireParticleSystem.Play(true);
    }
}
