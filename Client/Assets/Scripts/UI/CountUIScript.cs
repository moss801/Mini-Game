using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.PlayerLoop;

public class CountUIScript : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI countDownText;

    private int count;
    private bool needDestory;

    public void Initialization(int countTime,bool needHide)
    {
        count = countTime;
        needDestory = needHide;
        countDownText.text = count.ToString();
        InvokeRepeating("CountDown",1,1);
    }

    private void CountDown()
    {
        --count;
        countDownText.text = count.ToString();
        if (count == 0)
        {
            CancelInvoke("CountDown");
            if (needDestory)
            {
                GameObject.Find("GameManager").GetComponent<UIManager>().HideCanvas(8);
            }
        }
    } 
}
