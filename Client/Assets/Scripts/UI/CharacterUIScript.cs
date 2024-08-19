using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class CharacterUIScript : MonoBehaviour
{

    [SerializeField] private Slider hpBar;
    [SerializeField] private TextMeshProUGUI scoreText;
    [SerializeField] private TextMeshProUGUI timeText;
    [SerializeField] private TextMeshProUGUI userNameText;
    [SerializeField] private TextMeshProUGUI finishHintText;
    [SerializeField] public Button attackButton;
    [SerializeField] public Button throwButton;
    [SerializeField] public Button shopButton;
    [SerializeField] public Button bagButton;
    
    private PlayerController player;
    private bool isInit;
    private GameProcessManager gameProcessManager;
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        EventMgr.instance.AddListener("HpChange",SetHpBar);
        EventMgr.instance.AddListener("ScoreChange",SetScore);
        if (gameProcessManager is null)
        {
            gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        }
        EventMgr.instance.AddListener("HasFinish",ShowFinishHint);
    }
    
    public void OpenShop()
    {
        GameObject.Find("GameManager").GetComponent<UIManager>().InvokeCanvas(9);
    }
    
    public void OpenBag()
    {
        GameObject.Find("GameManager").GetComponent<UIManager>().InvokeCanvas(10);
    }
    
    private void SetScore(params object[] args)
    {
        scoreText.text = ((int)args[0]).ToString();
    }

    private void SetTime()
    {
        int seconds = gameProcessManager.GetGameTime();
        if (seconds < 10)
        {
            timeText.color = Color.red;
        }
        else
        {
            timeText.color = Color.white;
        }
        timeText.text = (seconds / 60).ToString() + " : " + (seconds % 60).ToString();
    }
    
    private void OnEnable()
    {
        isInit = false;
        player = GameObject.Find("GameManager").GetComponent<CharacterMgr>().GetLocalPlayer();
        if (player is not null)
        {
            isInit = true;
            hpBar.value = player.Hp;
            if (gameProcessManager is null)
            {
                gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
            }

            if (gameProcessManager.isSingle)
            {
                bagButton.gameObject.SetActive(false);
                shopButton.gameObject.SetActive(false);
            }
            else
            {
                bagButton.gameObject.SetActive(true);
                shopButton.gameObject.SetActive(true);
            }
            SetScore(gameProcessManager.GetScore());
            userNameText.text = gameProcessManager.GetUserName();
            InvokeRepeating("SetTime",0,1);
        }
    }

    private void OnDisable()
    {
        CancelInvoke("SetTime");
        finishHintText.gameObject.SetActive(false);
    }

    public void Attack()
    {
        if (isInit && !player.isDeath && player.GetArmed())
        {
            player.Attack();
        }
    }

    public void Throw()
    {
        if (isInit && !player.isDeath)
        {
            player.Throw();
        }
    }

    public void Interact()
    {
        if (isInit && !player.isDeath)
        {
            player.InteractImpl();
        }
    }

    public void SetHpBar(params object[] args)
    {
        hpBar.value = (int)args[0];
    }

    private void ShowFinishHint(params object[] args)
    {
        finishHintText.gameObject.SetActive(true);
    }
}
