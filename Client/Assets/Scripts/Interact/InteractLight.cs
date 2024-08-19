using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class InteractLight : InteractItemBase
{
    public float onTime;
    public float offTime;
    private bool isCountDownLightOn;
    private bool isCountDownLightOff;
    
    [SerializeField] private Light controlLight;
    private void Start()
    {
        isCountDownLightOn = false;
        isCountDownLightOff = false;
        LightOn();
        interactType = InteractItemType.Light;
    }

    public bool GetLightStatue()
    {
        if (controlLight is null)
        {
            return false;
        }
        return controlLight.enabled;
    }

    public override void InteractMethod()
    {
        if (GetLightStatue())
        {
            LightOff();
        }
        else
        {
            LightOn();
        }
    }

    private void OnTriggerStay(Collider other)
    {
        if (GetLightStatue())
        {
            if (other.gameObject.CompareTag("Enemy"))
            {
                other.gameObject.GetComponent<EnemyController>().Death();
            }
        }
    }

    // ReSharper disable Unity.PerformanceAnalysis
    public void LightOff()
    {
        if (isCountDownLightOn)
        {
            CancelInvoke("LightOn");
            isCountDownLightOn = false;
        }

        if (isCountDownLightOff)
        {
            CancelInvoke("LightOff");
            isCountDownLightOff = false;
        }
        controlLight.enabled = false;
        isCountDownLightOn = true;
        Invoke("LightOn",offTime);
    }

    // ReSharper disable Unity.PerformanceAnalysis
    public void LightOn()
    {
        if (isCountDownLightOn)
        {
            CancelInvoke("LightOn");
            isCountDownLightOn = false;
        }
        if (isCountDownLightOff)
        {
            CancelInvoke("LightOff");
            isCountDownLightOff = false;
        }
        controlLight.enabled = true;
        isCountDownLightOff = true;
        Invoke("LightOff",onTime);
    }
}
