using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering.Universal;

public class CameraController : MonoBehaviour
{
    
    public float initVerticalAngle = 10f;
    public float initHorizontalAngle = 0f;
    public float armLength = 1;
    public float rate = 2.5f;
    public float minAngle = -30f;
    public float maxAngle = 30f;
    public const int Circle = 360;
    public Transform root;

    private InputReceiver inputReceiver;
    private Vector2 angle;

    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        angle = Vector2.zero;
        root = new GameObject("root").transform;
        root.parent = gameObject.transform.parent;
        root.localPosition = new Vector3(0,0,0);
        gameObject.transform.parent = root;
        gameObject.transform.localPosition = new Vector3(0, 1f, -2f).normalized  * armLength;
        inputReceiver = GameObject.Find("GameManager").GetComponent<InputReceiver>();
        Invoke("AddUICamera",0.1f);
    }

    /// <summary>
    /// Update is called once per frame.
    /// </summary>
    private void Update()
    {
        ChangeView();
        //AvoidTouch();
    }

    /// <summary>
    /// 相机接受视角变动输入.
    /// </summary>
    protected void ChangeView()
    {
        angle += inputReceiver.GetViewChange() * rate;
        angle.y %= Circle;
        angle.x = Mathf.Clamp(angle.x, minAngle, maxAngle);
        root.localEulerAngles = angle;
        /*Vector3 angle1 = root.localEulerAngles;
        angle1.y = angle.y;
        root.localEulerAngles = angle1;
        angle1 = gameObject.transform.localEulerAngles;
        angle1.x = angle.x;
        gameObject.transform.localEulerAngles = angle1;*/
    }

    /// <summary>
    /// 相机避障，效果不好暂未使用.
    /// </summary>
    protected void AvoidTouch()
    {
        Vector3 direction = gameObject.transform.position - root.parent.position;
        RaycastHit[] hitInfos;
        Ray ray = new Ray(root.parent.position, direction);
        hitInfos = Physics.RaycastAll(ray);
        float minDistance = armLength;
        for (int i = 0; i < hitInfos.Length ; i++)
        {
            if (hitInfos[i].collider.gameObject.name != "Character" && hitInfos[i].distance < minDistance)
            {
                minDistance = hitInfos[i].distance;
            }
        }
        gameObject.transform.localPosition = Vector3.Lerp(gameObject.transform.localPosition, direction.normalized * minDistance, Time.deltaTime*2);
    }

    /// <summary>
    /// 叠加UI相机.
    /// </summary>
    public void AddUICamera()
    {
        GameObject UICamera = GameObject.Find("UICamera(Clone)");
        if (UICamera)
        {
            GetComponent<UniversalAdditionalCameraData>().cameraStack.Add(UICamera.GetComponentInChildren<Camera>());
        }
    }
}
