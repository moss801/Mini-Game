using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class FinishUIScript : MonoBehaviour
{
    [SerializeField] private float autoExitTime;
    [SerializeField] private Sprite successImage;
    [SerializeField] private Sprite failedImage;
    [SerializeField] private Image backImage;
    [SerializeField] private TextMeshProUGUI resultText;
    [SerializeField] private TextMeshProUGUI userNameText;
    [SerializeField] private TextMeshProUGUI rankText;
    [SerializeField] private TextMeshProUGUI killText;
    [SerializeField] private TextMeshProUGUI scoreText;
    [SerializeField] private TextMeshProUGUI timeText;
    [SerializeField] private TextMeshProUGUI countDownText;
    private float timer;
    
    public void InitResult(bool success, int score, int time, string username, int rank,int killNum)
    {
        timer = autoExitTime;
        if (success)
        {
            backImage.sprite = successImage;
            resultText.text = "胜利！";
        }
        else
        {
            backImage.sprite = failedImage;
            resultText.text = "失败！";
        }
        userNameText.text = username;
        rankText.text = rank.ToString();
        scoreText.text = score.ToString();
        timeText.text = time.ToString();
        killText.text = killNum.ToString();
        countDownText.text = ((int)timer).ToString();
        InvokeRepeating("CountDown",0,1);
    }

    private void CountDown()
    {
        timer -= 1;
        countDownText.text = ((int)timer).ToString();
        if (timer <= 0)
        {
            CancelInvoke("CountDown");
            BackToMainMenu();
        }
    }
    
    public void BackToMainMenu()
    {
        if (timer > 0)
        {
            CancelInvoke("CountDown");
        }
        GameProcessManager gameProcessManager = GameObject.Find("GameManager").GetComponent<GameProcessManager>();
        gameProcessManager.BackToMenu();
    }
}
