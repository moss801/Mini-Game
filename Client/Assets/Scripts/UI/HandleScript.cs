using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class HandleScript : MonoBehaviour
{
    public Image handle;
    public Image bg;
    public Canvas canvas;
    public float detectRadius;
    public bool isAlwayShow;
    
    private bool isDrag;
    private Vector2 startPos;
    private Vector2 centerPos;
    private InputReceiver inputReceiver;
    private RectTransform handleAnchor;
    private RectTransform bgAnchor;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        inputReceiver = GameObject.Find("GameManager").GetComponent<InputReceiver>();
        handleAnchor = handle.GetComponent<RectTransform>();
        bgAnchor = bg.GetComponent<RectTransform>();
        centerPos = handleAnchor.anchorMin*new Vector2(Screen.width,Screen.height);
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    private void Update()
    {
        CheckKeyInput();
        if (inputReceiver.IsLeftButtonDown() && !isDrag )
        {
            if (inputReceiver.GetMousePosition().x<Screen.width/2&&inputReceiver.GetMousePosition().y<Screen.height/2)
            {
                canvas.enabled = true;
                StartDrag();
            }
        }
        if (isDrag)
        {
            Drag();
        }
        if (!inputReceiver.IsLeftButtonDown() && isDrag)
        {
            ResetTouch();
            isDrag = false;
            handleAnchor.anchoredPosition = new Vector2();
            if (!isAlwayShow)
            {
                canvas.enabled = false;
            }
        }
    }

    /// <summary>
    /// 开始拖动.
    /// </summary>
    public void StartDrag()
    {
        isDrag = true;
        startPos = inputReceiver.GetMousePosition();
        Vector2 newLocation = startPos/new Vector2(Screen.width, Screen.height);
        bgAnchor.anchorMax = newLocation;
        bgAnchor.anchorMin = newLocation;
        handleAnchor.anchorMax = newLocation;
        handleAnchor.anchorMin = newLocation;
    }

    /// <summary>
    /// 拖动处理.
    /// </summary>
    private void Drag()
    {
        ResetTouch();
        Vector2 deltaPos = inputReceiver.GetMousePosition() - startPos;
        if (deltaPos.magnitude > detectRadius)
        {
            deltaPos = deltaPos.normalized * detectRadius;
        }
        handleAnchor.anchoredPosition = deltaPos;
        inputReceiver.SetJoyStick(deltaPos/detectRadius);
    }

    /// <summary>
    /// 重置位置.
    /// </summary>
    private void ResetTouch()
    {
        inputReceiver.SetJoyStick(Vector2.zero);
    }
    
    /// <summary>
    /// 同步键盘输入.
    /// </summary>
    private void CheckKeyInput()
    {
        handleAnchor.anchoredPosition = inputReceiver.GetMoveDirection() * detectRadius;
    }
}
