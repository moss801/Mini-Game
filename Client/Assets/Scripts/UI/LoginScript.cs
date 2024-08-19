using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using Google.Protobuf;
using TCCamp;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class LoginScript : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI titleText;
    
    [SerializeField] private Text hintText;

    [SerializeField] private TextMeshProUGUI buttonText;

    [SerializeField] private TextMeshProUGUI switchButtonText;

    [SerializeField] private TextMeshProUGUI userNameText;

    [SerializeField] private TMP_InputField userNameInput;
    
    [SerializeField] private TMP_InputField playerIDInput;
    
    [SerializeField] private TMP_InputField passwordInput;

    private Network networkManager;
    
    private bool isRegister;

    private string lastLoginUserName;

    private void OnEnable()
    {
        networkManager = GameObject.Find("GameManager").GetComponent<Network>();
        EventMgr.instance.AddListener("Network",ReceiveUserInformation);
        isRegister = true;
        Switch();
        HideHint();
    }
    
    private void OnDisable()
    {
        EventMgr.instance.RemoveListener("Network",ReceiveUserInformation);
    }

    public void OnClick()
    {
        if (isRegister)
        {
            Create();
        }
        else
        {
            Login();
        }
    }
    
    public void Switch()
    {
        isRegister = !isRegister;
        if (isRegister)
        {
            titleText.text = "注册";
            switchButtonText.text = "去登录";
            buttonText.text = "创建";
            userNameText.transform.localScale = Vector3.one;
            userNameInput.transform.localScale = Vector3.one;
        }
        else
        {
            titleText.text = "登录";
            switchButtonText.text = "去创建";
            buttonText.text = "登录";
            userNameText.transform.localScale = Vector3.zero;
            userNameInput.transform.localScale = Vector3.zero;
        }
    }
    
    public void HideHint()
    {
        hintText.text = "";
    }

    private void Login()
    {
        if (playerIDInput.text.Length == 0)
        {
            hintText.text = "用户名为空";
            return;
        }
        if (passwordInput.text.Length < 6)
        {
            hintText.text = "密码错误";
            return;
        }
        lastLoginUserName = playerIDInput.text;
        PlayerLoginReq req = new PlayerLoginReq();
        req.PlayerID = playerIDInput.text;
        req.Password = GetMd5();
        networkManager.SendMsg(CLIENT_CMD.ClientLoginReq,req);
    }

    private void Create()
    {
        if (playerIDInput.text.Length == 0)
        {
            hintText.text = "账户名不能为空";
            return;
        }
        if (passwordInput.text.Length < 6)
        {
            hintText.text = "密码过短";
            return;
        }

        PlayerCreateReq req = new PlayerCreateReq();
        Encoding encoding = Encoding.UTF8;
        req.Name = ByteString.CopyFrom(userNameInput.text,encoding) ;
        req.Password = GetMd5();
        req.PlayerID = playerIDInput.text;
        networkManager.SendMsg(CLIENT_CMD.ClientCreateReq,req);
    }

    private void ReceiveUserInformation(object[] args)
    {
        
        int cmd = (int)args[0];
        if (cmd == (int)SERVER_CMD.ServerCreateRsp)
        {
            byte[] message = (byte[])args[1];
            PlayerCreateRsp rsp = new PlayerCreateRsp();
            rsp.MergeFrom(message);
            Debug.Log(rsp.Reason);
            hintText.text = rsp.Reason;
            if (rsp.Result == 0)
            {
                EventMgr.instance.DispatchEvent("LoginDone",rsp.Name);
                UIManager uiManager = GameObject.Find("GameManager").GetComponent<UIManager>();
                uiManager.HideCanvas(1);
            }
        }else if (cmd == (int)SERVER_CMD.ServerLoginRsp)
        {
            byte[] message = (byte[])args[1];
            PlayerLoginRsp rsp = new PlayerLoginRsp();
            rsp.MergeFrom(message);
            Debug.Log(rsp.Reason);
            hintText.text = rsp.Reason;
            if (rsp.Result == 0)
            {
                EventMgr.instance.DispatchEvent("LoginDone",rsp.PlayerData.Name);
                lastLoginUserName = rsp.PlayerData.Playerid;
                UIManager uiManager = GameObject.Find("GameManager").GetComponent<UIManager>();
                uiManager.HideCanvas(1);
            }
        }
    }
    
    private string GetMd5()
    {
        byte[] bytes = Encoding.UTF8.GetBytes(passwordInput.text); 
        MD5 md5 = MD5.Create();
        string s = BitConverter.ToString(md5.ComputeHash(bytes), 0, bytes.Length);
        return s;
    }

    public void Hide()
    {
        GameObject.Find("GameManager").GetComponent<UIManager>().HideCanvas(1);
    }
}
