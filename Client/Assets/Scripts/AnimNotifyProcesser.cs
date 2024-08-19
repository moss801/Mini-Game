using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AnimNotifyProcesser : MonoBehaviour
{
    private AudioSource audioSource;
    private Animator characterAnimator;
    
    /// <summary>
    /// Start is called before the first frame update.
    /// </summary>
    private void Start()
    {
        audioSource = GetComponent<AudioSource>();
        characterAnimator = gameObject.GetComponent<Animator>();
    }

    /// <summary>
    /// 左脚落地触发.
    /// </summary>
    public void LeftFootDown()
    {
        PlayStepSound();
    }
    
    /// <summary>
    /// 右脚落地触发.
    /// </summary>
    public void RightFootDown()
    {
        PlayStepSound();
    }

    /// <summary>
    /// 播放脚步音效.
    /// </summary>
    public void PlayStepSound()
    {
        double time = AudioSettings.dspTime;
        audioSource.time = 0.2f;
        audioSource.PlayScheduled(time);
        audioSource.SetScheduledEndTime(time+0.3f);
    }

    public void ResetStatue()
    {
        characterAnimator.SetBool("beHit",false);
        characterAnimator.SetBool("openBox", false);
        characterAnimator.SetBool("switchLightOn", false);
        characterAnimator.SetBool("switchLightOff", false);
        characterAnimator.SetBool("isAttack",false);
        gameObject.GetComponentInParent<PlayerController>().isInteract = false;
    }
}
