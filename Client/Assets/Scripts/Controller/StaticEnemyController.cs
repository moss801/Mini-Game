using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class StaticEnemyController : EnemyController
{
    [SerializeField] private float detectAngle;
    [SerializeField] private float rotationSpeed;
    
    private float currentAngle;

    private Vector3 currentEular;
    private Quaternion initQuat;
    private bool isLeft;

    protected override void Start()
    {
        base.Start();
        currentAngle = 0;
        initQuat = gameObject.transform.rotation;
        currentEular = initQuat.eulerAngles;
        isLeft = false;
    }

    protected void Update()
    {
        if (isAuthority)
        {
            if (isLeft)
            {
                currentAngle += rotationSpeed;
                if (currentAngle > detectAngle)
                {
                    isLeft = false;
                }
            }
            else
            {
                currentAngle -= rotationSpeed;
                if (currentAngle < -detectAngle)
                {
                    isLeft = true;
                }
            }
            currentEular.y = initQuat.eulerAngles.y + currentAngle;
            gameObject.transform.rotation = Quaternion.Euler(currentEular);
        }
        else
        {
            gameObject.transform.position = serverPos;
            gameObject.transform.rotation =
                Quaternion.Euler(Vector3.Lerp(gameObject.transform.rotation.eulerAngles, serverRot, Time.deltaTime));
        }
         
    }
}
