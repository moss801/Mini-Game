using System;
using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

public class InputReceiver : MonoBehaviour
{

    private Vector3 moveForwardInput;
    private Vector2 viewChangeInput;
    private Vector2 joyStickPosition;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        moveForwardInput = Vector3.zero;
        viewChangeInput = Vector2.zero;
        joyStickPosition = Vector2.zero;
        Cursor.lockState = CursorLockMode.Confined;
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    private void Update()
    {
        SetMoveDirection();
        SetViewChange();
        CheckExit();
    }

    /// <summary>
    /// 获取移动方向.
    /// </summary>
    /// <returns>移动方向.</returns>
    public Vector3 GetMoveDirection()
    {
        return moveForwardInput;
    }

    /// <summary>
    /// 计算移动方向.
    /// </summary>
    private void SetMoveDirection()
    {
        moveForwardInput = joyStickPosition;
        if (Input.GetKey(KeyCode.A))
        {
            moveForwardInput.x = -1;
        }
        if(Input.GetKey(KeyCode.S))
        {
            moveForwardInput.y = -1;
        }

        if (Input.GetKey(KeyCode.W))
        {
            moveForwardInput.y = 1;
        }

        if (Input.GetKey(KeyCode.D))
        {
            moveForwardInput.x = 1;
        }
    }

    /// <summary>
    /// 获取相机视角变化量.
    /// </summary>
    /// <returns>相机视角变化量.</returns>
    public Vector2 GetViewChange()
    {
        return viewChangeInput;
    }

    /// <summary>
    /// 计算相机视角变化量.
    /// </summary>
    private void SetViewChange()
    {
        viewChangeInput = new Vector2(-Input.GetAxis("Mouse Y"), Input.GetAxis("Mouse X"));
    }

    /// <summary>
    /// 判断是否与门进行交互.
    /// </summary>
    /// <returns>是否与门进行交互.</returns>
    public bool IsOpenDoor()
    {
        return IsLeftButtonDown();
    }

    /// <summary>
    /// 判断是否继续游戏.
    /// </summary>
    /// <returns>是否继续游戏.</returns>
    public bool IsContinueGame()
    {
        return Input.GetKey(KeyCode.R);
    }

    public bool IsAttack()
    {
        return Input.GetKeyUp(KeyCode.T);
    }

    /// <summary>
    /// 判断是否切换背包界面.
    /// </summary>
    /// <returns>是否切换背包界面.</returns>
    public bool IsSwitchUIBag()
    {
        return Input.GetKeyUp(KeyCode.Alpha1);
    }
    
    /// <summary>
    /// 判断是否切换商城界面.
    /// </summary>
    /// <returns>是否切换商城界面.</returns>
    public bool IsSwitchUIShop()
    {
        return Input.GetKeyUp(KeyCode.Alpha2);
    }
    
    /// <summary>
    /// 判断是否切换摇杆界面.
    /// </summary>
    /// <returns>是否切换摇杆界面.</returns>
    public bool IsSwitchUIJoyStick()
    {
        return Input.GetKeyUp(KeyCode.Alpha3);
    }

    /// <summary>
    /// 设置摇杆位置.
    /// </summary>
    /// <param name="newPosition">摇杆位置.</param>
    public void SetJoyStick(Vector2 newPosition)
    {
        joyStickPosition = newPosition;
    }

    public bool IsInteract()
    {
        return Input.GetKeyDown(KeyCode.Space);
    }

    public bool IsThrow()
    {
        return Input.GetKeyDown(KeyCode.G);
    }
    
    /// <summary>
    /// 检测退出游戏.
    /// </summary>
    public void CheckExit()
    {
        if (Input.GetKeyUp(KeyCode.Escape))
        {
            GameObject.Find("GameManager").GetComponent<GameProcessManager>().ExitGame();
        }
    }

    /// <summary>
    /// 判断鼠标左键是否按下.
    /// </summary>
    /// <returns>鼠标左键是否按下.</returns>
    public bool IsLeftButtonDown()
    {
        return Input.GetMouseButton(0);
    }
    
    /// <summary>
    /// 判断鼠标右键是否按下.
    /// </summary>
    /// <returns>鼠标右键是否按下.</returns>
    public bool IsRightButtonDown()
    {
        return Input.GetMouseButton(1);
    }

    /// <summary>
    /// 获取鼠标位置.
    /// </summary>
    /// <returns>鼠标位置.</returns>
    public Vector2 GetMousePosition()
    {
        return Input.mousePosition;
    }
}
