using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class InteractDoor : InteractItemBase
{
    public GameObject doorMesh;
    
    private bool isOpen;
    private bool isExeAnim;
    private Vector3 currentForward;
    private Vector3 finalForward;
    private const float minAngle = 1f;
    private Vector3 openRotation;
    private Vector3 closeRotation;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        doorMesh = Instantiate(doorMesh, gameObject.transform);
        doorMesh.transform.localPosition = new Vector3(-1, 0, 0);
        isOpen = false;
        isExeAnim = false;
        openRotation = gameObject.transform.right;
        closeRotation = gameObject.transform.forward;
        currentForward = gameObject.transform.forward;
        finalForward = gameObject.transform.forward;
        interactType = InteractItemType.Door;
    }

    /// <summary>
    /// 触发门交互.
    /// </summary>
    public override void InteractMethod()
    {
        if (isOpen)
        {
            finalForward = closeRotation;
        }
        else
        {
            finalForward = openRotation;
        }
        isOpen = !isOpen;
        isExeAnim = true;
        InvokeRepeating("DoorAnim",0,Time.deltaTime);
    }

    public void OpenDoor()
    {
        if (isOpen)
        {
            return;
        }
        isOpen = true;
        finalForward = openRotation;
        if (!isExeAnim)
        {
            InvokeRepeating("DoorAnim",0,Time.deltaTime);
        }
    }
    
    public void CloseDoor()
    {
        if (!isOpen)
        {
            return;
        }
        isOpen = false;
        finalForward = closeRotation;
        if (!isExeAnim)
        {
            InvokeRepeating("DoorAnim",0,Time.deltaTime);
        }
    }

    /// <summary>
    /// 播放门动画（插值模拟开关门）.
    /// </summary>
    public void DoorAnim()
    {
        currentForward = Vector3.Lerp(currentForward,finalForward,Time.deltaTime);
        gameObject.transform.forward = currentForward;
        if (Vector3.Angle(currentForward,finalForward)<=minAngle)
        {
            currentForward = finalForward;
            gameObject.transform.forward = currentForward;
            CancelInvoke("DoorAnim");
            isExeAnim = false;
        }
    }

    public bool GetDoorOpen()
    {
        return isOpen;
    }
}
