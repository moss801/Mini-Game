using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class InteractBox : InteractItemBase
{
    [SerializeField] private GameObject item;
    [SerializeField] private GameObject boxClose;
    [SerializeField] private GameObject boxOpen;

    private bool isOpen;
    private GameObject operatorObject;
    
    // Start is called before the first frame update
    private void Start()
    {
        interactType = InteractItemType.Box;
        Recover();
    }

    public void Recover()
    {
        boxClose.SetActive(true);
        boxOpen.SetActive(false);
        isOpen = false;
    }

    public override void InteractMethod()
    {
        if (operatorObject is null || isOpen)
        {
            return;
        }
        boxOpen.SetActive(true);
        boxClose.SetActive(false);
        isOpen = true;
        if (!GameObject.Find("GameManager").GetComponent<GameProcessManager>().isSingle)
        {
            return;
        }
        if (operatorObject is not null)
        {
            operatorObject.GetComponent<PlayerController>().SetArmed(true);
        }
    }

    private void OnTriggerEnter(Collider other)
    {
        if (other.gameObject.CompareTag("Player"))
        {
            operatorObject = other.gameObject;
        }
    }

    private void OnTriggerExit(Collider other)
    {
        if (operatorObject == other.gameObject)
        {
            operatorObject = null;
        }
    }

    public bool GetBoxOpen()
    {
        return isOpen;
    }
}
