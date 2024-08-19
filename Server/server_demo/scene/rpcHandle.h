#pragma once
#include "../../utils/baseData.h"
enum ObjectTimerType
{
	TIMER_LIGHT = 0,
	TIMER_TRAP = 1,
	TIMER_IMMUNITY = 2,
	TIMER_SPEEDDEBUFF = 3
};
typedef struct CollisionInfo
{
	SceneObjectType typeA;
	int idA;
	SceneObjectType typeB;
	int idB;
	int res;
}CollisionInfo;
typedef struct ObjectTimerInfo
{
	ObjectTimerType type;
	int id;
	float remainTime;
	ObjectTimerInfo(ObjectTimerType _type, int _id, int _remainTime)
	:type(_type),id(_id),remainTime(_remainTime){};
}ObjectTimerInfo;

class RPCHandle {
public:
	RPCHandle(SynInfo* sceneInfo,int roomId);
	~RPCHandle();

public:

	void HandleRPCReq(uv_tcp_t* client, RPC* req);

	void HandleCollisionCheckReq(uv_tcp_t* client, CollisionReq* req);
	
private:

	void Tick();
	//0 is exceed max, 1 is between,2 is little than min;
	int CheckIsValid(vec3 posA, vec3 posB, float minRange, float maxRange);

	bool HandleCollisionImmediate(CollisionInfo* collisionInfo);

	void CheckCollisionList();

	void SendRpc(RPC rsp);

	void SendAnimSyn(CharacterAnimSynMsg msg);

	vec3 GetObjectPos(SceneObjectType type, int id);

	bool GetObjectActive(SceneObjectType type, int id);

	int GetCollisionResult(SceneObjectType typeA, int idA, SceneObjectType typeB, int idB);

	void InitTimerObject();

	void HandleTimerObject();

	void UpdateMonsterTargetPos();

	void AddOrResetTimer(int id, ObjectTimerType type, float newTime);

	list<CollisionInfo*> CollisionCheckList;

	vector<ObjectTimerInfo> timerObjectList;

	unordered_map<int,int> monsterChaseMap;
	
	SynInfo* SceneInfo;

	int RoomId;

	int timerHandle;

	int timerTickUpdateChase;
	
};
