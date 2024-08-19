using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public enum InteractItemType
{
    Box = 1,
    Door = 2,
    Light = 3
}
public class InteractItemBase : MonoBehaviour
{
    [HideInInspector] public InteractItemType interactType;
    [HideInInspector] public int itemId;
    public virtual void InteractMethod()
    {
        
    }
}
