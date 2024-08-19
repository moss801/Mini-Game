using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class HintUIScript : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI hintText;
    [SerializeField] private Image backImage;
    public void InitHint(string hint,Color bgcolor,float hideTime)
    {
        this.hintText.text = hint;
        backImage.color = bgcolor;
        if (hideTime > 0)
        {
            Invoke("HideSelf",hideTime);
        }
        
    }

    private void HideSelf()
    {
        GameObject.Find("GameManager").GetComponent<UIManager>().HideCanvas(6);
    }
}
