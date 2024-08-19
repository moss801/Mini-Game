using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class RankDetailScript : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI orderText;
    [SerializeField] private TextMeshProUGUI nameText;
    [SerializeField] private TextMeshProUGUI scoreText;
    [SerializeField] private TextMeshProUGUI timeText;
    
    public void InitDetail(int order, string name, int score,int time)
    {
        orderText.text = order.ToString();
        nameText.text = name;
        scoreText.text = score.ToString() + "分";
        timeText.text = time.ToString() + "秒";
    }
}
