using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;

public class DynamicEnemyController : EnemyController
{
    public int hangoutRange = 5;
    public int maxDetectTime = 10;
    public int waitTime;
    protected NavMeshAgent navMeshAgent;
    protected bool canEnterWait;

    private const float MINMISTAKE = 0.2f; 
    protected override void Start()
    {
        base.Start();
        canEnterWait = true;
        InitAgent();
        Hangout();
    }

    // Update is called once per frame
    void Update()
    {
        if (isLocalControl)
        {
            if (lastMurder is not null && !lastMurder.isDeath)
            {
                navMeshAgent.SetDestination(lastMurder.transform.position);
                return;
            }
            if (navMeshAgent.remainingDistance < 0.5f&&canEnterWait)
            {
                Invoke("Hangout",waitTime);
                canEnterWait = false;
            }
        }
        else
        {
            if (isAuthority)
            {
                if ((targetPos - gameObject.transform.position).magnitude < MINMISTAKE)
                {
                    gameObject.transform.position = targetPos;
                }
                else
                {
                    navMeshAgent.SetDestination(targetPos);
                }
            }
            else
            {
                gameObject.transform.position = Vector3.Lerp(gameObject.transform.position,serverPos,Time.deltaTime*2);
                gameObject.transform.rotation = Quaternion.Euler(Vector3.Lerp(gameObject.transform.rotation.eulerAngles,serverRot,Time.deltaTime*2));
            }
        }
    }
    
    /// <summary>
    /// 初始化寻路智能体.
    /// </summary>
    protected void InitAgent()
    {
        /*bool isSetPos = false;
        for (int i = 0; i < maxDetectTime; i++)
        {
            Vector3 targetPos = Random.insideUnitSphere * hangoutRange;
            targetPos.y = 0;
            NavMeshHit hit;
            if (NavMesh.SamplePosition(targetPos, out hit, 1, 1))
            {
                gameObject.transform.position = hit.position;
                isSetPos = true;
                break;
            }
        }
        if (!isSetPos)
        {
            Debug.LogWarning("NavMeshAgentFailed");
            return;
        }*/
        navMeshAgent = gameObject.AddComponent<NavMeshAgent>();
        navMeshAgent.speed = maxSpeed;
    }
    
    /// <summary>
    /// AI-闲逛行为.
    /// </summary>
    protected void Hangout()
    {
        canEnterWait = true;
        for (int i = 0; i < maxDetectTime; i++)
        {
            Vector3 targetPos = Random.insideUnitSphere * hangoutRange;
            NavMeshHit hit;
            if (NavMesh.SamplePosition(targetPos, out hit, hangoutRange, 1))
            {
                navMeshAgent.SetDestination(hit.position);
                return;
            }
        }
    }
}
