#include "rpcHandle.h"
#include "sceneMgr.h"
#include "../../codec/codec.h"
#include "../../utils/utils.h"
const int TRAPTIME = 3;
const int SLOWTIME = 5;
const int LIGHTONTIME = 10;
const int LIGHTOFFTIME = 10;
const int IMMUNITYTIME = 1;
const int TRAPDAMAGE = 1;

static char s_send_buff[1024 * 64];
RPCHandle::RPCHandle(SynInfo* sceneInfo,int roomId) {
	SceneInfo = sceneInfo;
	RoomId = roomId;
	InitTimerObject();
	timerHandle = TimerMgr::GetInstance().AddTimer(1,-1,this,&RPCHandle::Tick,false);
	timerTickUpdateChase = TimerMgr::GetInstance().AddTimer(1,-1,this,&RPCHandle::UpdateMonsterTargetPos,true);
	CollisionCheckList.clear();
}
RPCHandle::~RPCHandle() {
	TimerMgr::GetInstance().RemoveTimer(timerHandle);
	TimerMgr::GetInstance().RemoveTimer(timerTickUpdateChase);
	SceneInfo = nullptr;
	for(auto it = CollisionCheckList.begin();it!=CollisionCheckList.end();++it)
	{
		delete (*it);
	}
}

void RPCHandle::HandleRPCReq(uv_tcp_t* client, RPC* req)
{
	int characterId = SceneMgr::GetInstance().GetCharacterNetid(client);
	if(req->cmd() == THROW_WEAPON)
	{
		if(SceneInfo->characterList[characterId]->isArmed)
		{
			SceneInfo->characterList[characterId]->isArmed = false;
			SceneMgr::GetInstance().SetDirty(RoomId,0);
		}
		return;
	}
	SceneObjectType type;
	switch (req->cmd())
	{
		case BOX_STATUE_OPEN:
			type = BOX;
			break;
		case LIGHT_STATUE_ON:
			type = LIGHT;
			break;
		case LIGHT_STATUE_OFF:
			type = LIGHT;
			break;
		case DOOR_STATUE_OPEN:
			type = DOOR;
			break;
		case DOOR_STATUE_CLOSE:
			type = DOOR;
			break;
	}
	if(GetCollisionResult(PLAYER,characterId,type,req->param(0))!=2)
	{
		return;
	}
	CharacterAnimSynMsg msg;
	msg.set_netid(characterId);
	RPC rpc;
	rpc.add_param(req->param(0));
	switch (req->cmd()) {
		case BOX_STATUE_OPEN:
			type = BOX;
			rpc.set_cmd(RPC_CMD::BOX_STATUE_OPEN);
			if(!GetObjectActive(BOX,req->param(0)))
			{
				SceneInfo->boxList[req->param(0)].isActive = true;
				SceneInfo->characterList[characterId]->isArmed = true;
				SceneMgr::GetInstance().SetDirty(RoomId, 0);
				msg.set_animttype(CharacterAnimType::OPENBOX);
				SendAnimSyn(msg);
				SendRpc(rpc);
			}
			else {
				int len = encode(s_send_buff, SERVER_CMD::SERVER_RPC_MSG, rpc.SerializeAsString().c_str(), rpc.ByteSize());
				sendData((uv_stream_t*)(client), s_send_buff, len);
			}
			break;
		case LIGHT_STATUE_ON:
			AddOrResetTimer(req->param(0),TIMER_LIGHT,LIGHTONTIME);
			msg.set_animttype(CharacterAnimType::TURNONLIGHT);
			SendAnimSyn(msg);
			if(!SceneInfo->lightList[req->param(0)].isActive)
			{
				SceneInfo->lightList[req->param(0)].isActive = true;
				rpc.set_cmd(RPC_CMD::LIGHT_STATUE_ON);
				SendRpc(rpc);
			}
			break;
		case LIGHT_STATUE_OFF:
			AddOrResetTimer(req->param(0),TIMER_LIGHT,LIGHTOFFTIME);
			msg.set_animttype(CharacterAnimType::TURNOFFLIGHT);
			SendAnimSyn(msg);
			if(SceneInfo->lightList[req->param(0)].isActive)
			{
				SceneInfo->lightList[req->param(0)].isActive = false;
				rpc.set_cmd(RPC_CMD::LIGHT_STATUE_OFF);
				SendRpc(rpc);
			}
			break;
		case DOOR_STATUE_OPEN:
			rpc.set_cmd(RPC_CMD::DOOR_STATUE_OPEN);
			if(!GetObjectActive(DOOR,req->param(0)))
			{
				SceneInfo->doorList[req->param(0)].isActive = true;
				msg.set_animttype(CharacterAnimType::OPENDOOR);
				SendAnimSyn(msg);
				SendRpc(rpc);
			}
			else {
				int len = encode(s_send_buff, SERVER_CMD::SERVER_RPC_MSG, rpc.SerializeAsString().c_str(), rpc.ByteSize());
				sendData((uv_stream_t*)(client), s_send_buff, len);
			}
			break;
		case DOOR_STATUE_CLOSE:
			rpc.set_cmd(RPC_CMD::DOOR_STATUE_CLOSE);
			if(GetObjectActive(DOOR,req->param(0)))
			{
				SceneInfo->doorList[req->param(0)].isActive = false;
				msg.set_animttype(CharacterAnimType::CLOSEDOOR);
				SendAnimSyn(msg);
				SendRpc(rpc);
			}
			else {
				int len = encode(s_send_buff, SERVER_CMD::SERVER_RPC_MSG, rpc.SerializeAsString().c_str(), rpc.ByteSize());
				sendData((uv_stream_t*)(client), s_send_buff, len);
			}
			break;
	}
}

void RPCHandle::HandleCollisionCheckReq(uv_tcp_t* client, CollisionReq* req)
{
	CollisionInfo* collisionInfo = new CollisionInfo();
	collisionInfo->idA = req->ida();
	collisionInfo->typeA = req->typea();
	collisionInfo->idB = req->idb();
	collisionInfo->typeB = req->typeb();
	collisionInfo->res = -1;
	CollisionCheckList.push_back(collisionInfo);
}

void RPCHandle::Tick()
{
	CheckCollisionList();
	HandleTimerObject();
}

int RPCHandle::CheckIsValid(vec3 posA, vec3 posB, float minRange, float maxRange)
{
	float distance = pow(posA.x - posB.x, 2) + pow(posA.y - posB.y, 2) + pow(posA.z - posB.z, 2);
	if(distance>maxRange)
	{
		return 0;
	}else if(distance<=minRange)
	{
		return 2;
	}else
	{
		return 1;
	}
}


bool RPCHandle::HandleCollisionImmediate(CollisionInfo* collisionInfo)
{
	if((collisionInfo->typeA==GARGOYLE||collisionInfo->typeA==GHOST)&&collisionInfo->typeB==BULLET)
	{
		if(!SceneInfo->bulletList[collisionInfo->idB].isActive)
		{
			return true;
		}
		SceneInfo->bulletList[collisionInfo->idB].isActive = false;
		SceneInfo->monsterList[collisionInfo->idA].hp -= SceneMgr::GetInstance().GetWeaponDamage(0);
		if(SceneInfo->monsterList[collisionInfo->idA].hp <= 0)
		{
			SceneInfo->monsterList[collisionInfo->idA].hp = 0;
			if(monsterChaseMap.find(collisionInfo->idA)!=monsterChaseMap.end())
			{
				monsterChaseMap.erase(collisionInfo->idA);
			}
			if(!SceneInfo->characterList[SceneInfo->bulletList[collisionInfo->idB].netId]->isSuccess)
			{
				SceneInfo->characterList[SceneInfo->bulletList[collisionInfo->idB].netId]->score += SceneMgr::GetInstance().GetMonsterAttribute(collisionInfo->typeA)->killPoint;
				SceneInfo->characterList[SceneInfo->bulletList[collisionInfo->idB].netId]->beatCount++;
				SceneMgr::GetInstance().SetDirty(RoomId,0);
			}
		}else
		{
			monsterChaseMap[collisionInfo->idA] = SceneInfo->bulletList[collisionInfo->idB].netId;
		}
		SceneMgr::GetInstance().SetDirty(RoomId,1);
		SceneMgr::GetInstance().SetDirty(RoomId,3);
		return true;
	}else if(collisionInfo->typeB==GARGOYLE||collisionInfo->typeB==GHOST)
	{
		if(collisionInfo->typeA == BULLET)
		{
			if(!SceneInfo->bulletList[collisionInfo->idA].isActive)
			{
				return true;
			}
			SceneInfo->bulletList[collisionInfo->idA].isActive = false;
			SceneInfo->monsterList[collisionInfo->idB].hp -= SceneMgr::GetInstance().GetWeaponDamage(0);
			if(SceneInfo->monsterList[collisionInfo->idB].hp <= 0)
			{
				if(monsterChaseMap.find(collisionInfo->idB)!=monsterChaseMap.end())
				{
					monsterChaseMap.erase(collisionInfo->idB);
				}
				SceneInfo->monsterList[collisionInfo->idB].hp = 0;
				SceneInfo->characterList[SceneInfo->bulletList[collisionInfo->idA].netId]->score += SceneMgr::GetInstance().GetMonsterAttribute(collisionInfo->typeB)->killPoint;
				SceneInfo->characterList[SceneInfo->bulletList[collisionInfo->idA].netId]->beatCount++;
				SceneMgr::GetInstance().SetDirty(RoomId,0);
			}else
			{
				monsterChaseMap[collisionInfo->idB] = SceneInfo->bulletList[collisionInfo->idA].netId;
			}
			SceneMgr::GetInstance().SetDirty(RoomId,1);
			SceneMgr::GetInstance().SetDirty(RoomId,3);
			return true;
		}else if(collisionInfo->typeA == PLAYER && !SceneInfo->characterList[collisionInfo->idA]->isImmunity)
		{
			int damage = SceneMgr::GetInstance().GetMonsterAttribute(collisionInfo->typeB)->damage;
			if(SceneInfo->characterList[collisionInfo->idA]->hp > damage)
			{
				SceneInfo->characterList[collisionInfo->idA]->hp -= damage;
				SceneInfo->characterList[collisionInfo->idA]->isImmunity = true;
				AddOrResetTimer(collisionInfo->idA,TIMER_IMMUNITY,IMMUNITYTIME);
			}  else
			{
				SceneInfo->characterList[collisionInfo->idA]->hp = 0;
				SceneInfo->characterList[collisionInfo->idA]->isDeath = true;
				SceneMgr::GetInstance().PlayerDeath(RoomId, SceneInfo->characterList[collisionInfo->idA]->netId);
			}
			SceneMgr::GetInstance().SetDirty(RoomId,0);
		}
	}else if(collisionInfo->typeA == PLAYER)
	{
		if(collisionInfo->typeB == FINISHLINE)
		{
			SceneMgr::GetInstance().PlayerSuccess(RoomId,collisionInfo->idA);
		}else if(collisionInfo->typeB == SLIME)
		{
			if (SceneInfo->characterList[collisionInfo->idA]->maxSpeed != MAXSPEED * 0.5) {
				SceneInfo->characterList[collisionInfo->idA]->maxSpeed = MAXSPEED * 0.5;
				SceneMgr::GetInstance().SetDirty(RoomId, 0);
			}
			AddOrResetTimer(collisionInfo->idA,TIMER_SPEEDDEBUFF,SLOWTIME);
		}else if(collisionInfo->typeB == TRAP
			&& SceneInfo->trapList[collisionInfo->idB].isActive
			&& !SceneInfo->characterList[collisionInfo->idA]->isImmunity)
		{
			SceneInfo->characterList[collisionInfo->idA]->hp -= TRAPDAMAGE;
			SceneInfo->characterList[collisionInfo->idA]->isImmunity = true;
			AddOrResetTimer(collisionInfo->idA,TIMER_IMMUNITY,IMMUNITYTIME);
			SceneMgr::GetInstance().SetDirty(RoomId,0);
			if (SceneInfo->characterList[collisionInfo->idA]->hp <= 0)
			{
				SceneInfo->characterList[collisionInfo->idA]->isDeath = true;
				SceneMgr::GetInstance().PlayerDeath(RoomId, SceneInfo->characterList[collisionInfo->idA]->netId);
			}
		}
	}
	return false;
}

void RPCHandle::CheckCollisionList()
{
	for(auto it = CollisionCheckList.begin();it!= CollisionCheckList.end();)
	{
		// 2.5 4
		int checkRes = -1;
		if ((*it)->typeA == BULLET || (*it)->typeB == BULLET) {
			checkRes = CheckIsValid(GetObjectPos((*it)->typeA, (*it)->idA), GetObjectPos((*it)->typeB, (*it)->idB), 100, 150);
		}
		else {
			checkRes = CheckIsValid(GetObjectPos((*it)->typeA, (*it)->idA), GetObjectPos((*it)->typeB, (*it)->idB), 3, 4);
		}
		if(checkRes == 0)
		{
			delete (*it);
			it = CollisionCheckList.erase(it);
		}else
		{
			(*it)->res = checkRes;
			if(checkRes == 2)
			{
				if (HandleCollisionImmediate((*it))) {
					it = CollisionCheckList.erase(it);
				}
				else {
					++it;
				}
			}
			else {
				++it;
			}
		}
	}
}

void RPCHandle::SendRpc(RPC rsp)
{
	list<uv_tcp_t*> clients = SceneMgr::GetInstance().GetClientInRoom(RoomId);
	int len = encode(s_send_buff, SERVER_CMD::SERVER_RPC_MSG, rsp.SerializeAsString().c_str(), rsp.ByteSize());
	for(auto it = clients.begin();it!=clients.end();++it)
	{
		sendData((uv_stream_t*)(*it), s_send_buff, len);
	}
}

void RPCHandle::SendAnimSyn(CharacterAnimSynMsg msg)
{
	list<uv_tcp_t*> clients = SceneMgr::GetInstance().GetClientInRoom(RoomId);
	int len = encode(s_send_buff, SERVER_CMD::SERVER_CHARACTERANIMSYN_RSP, msg.SerializeAsString().c_str(), msg.ByteSize());
	for(auto it = clients.begin();it!=clients.end();++it)
	{
		sendData((uv_stream_t*)(*it), s_send_buff, len);
	}
}

vec3 RPCHandle::GetObjectPos(SceneObjectType type, int id)
{
	switch (type) {
		case PLAYER:
			return SceneInfo->characterList[id]->position;
			break;
		case BULLET:
			return SceneInfo->bulletList[id].position;
			break;
		case GHOST:
			return SceneInfo->monsterList[id].position;
			break;
		case GARGOYLE:
			return SceneInfo->monsterList[id].position;
			break;
		case DOOR:
			return SceneInfo->doorList[id].position;
			break;
		case LIGHT:
			return SceneInfo->lightList[id].position;
			break;
		case BOX:
			return SceneInfo->boxList[id].position;
			break;
		case SLIME:
			return SceneInfo->slimeList[id].position;
			break;
		case TRAP:
			return SceneInfo->trapList[id].position;
			break;
		case FINISHLINE:
			return SceneMgr::GetInstance().GetFinishPos(RoomId);
			break;
	}
	return vec3();
}

bool RPCHandle::GetObjectActive(SceneObjectType type, int id)
{
	switch (type) {
		case DOOR:
			return SceneInfo->doorList[id].isActive;
			break;
		case LIGHT:
			return SceneInfo->lightList[id].isActive;
			break;
		case BOX:
			return SceneInfo->boxList[id].isActive;
			break;
		case TRAP:
			return SceneInfo->trapList[id].isActive;
			break;
	}
	return false;
}

int RPCHandle::GetCollisionResult(SceneObjectType typeA, int idA, SceneObjectType typeB, int idB)
{
	for (auto it = CollisionCheckList.begin(); it != CollisionCheckList.end(); ++it) {
		CollisionInfo* info = (*it);
		if ((info->idA == idA && info->idB == idB && info->typeA == typeA && info->typeB == typeB)
			|| (info->idA == idB && info->idB == idA && info->typeA == typeB && info->typeB == typeA)) {
			return info->res;
		}
	}
	return -1;
}

void RPCHandle::InitTimerObject()
{
	for(auto it = SceneInfo->lightList.begin();it!=SceneInfo->lightList.end();++it)
	{
		if(it->second.isActive)
		{
			timerObjectList.emplace_back(TIMER_LIGHT,it->first,LIGHTONTIME);
		}else
		{
			timerObjectList.emplace_back(TIMER_LIGHT,it->first,LIGHTOFFTIME);
		}
	}
	for(auto it = SceneInfo->trapList.begin();it!=SceneInfo->trapList.end();++it)
	{
		timerObjectList.emplace_back(TIMER_TRAP,it->first,TRAPTIME);
	}
}

void RPCHandle::HandleTimerObject()
{
	RPC rpc_lightOn;
	rpc_lightOn.set_cmd(LIGHT_STATUE_ON);
	bool isLightOn = false;
	RPC rpc_lightOff;
	rpc_lightOff.set_cmd(LIGHT_STATUE_OFF);
	bool isLightOff = false;
	RPC rpc_trapEnable;
	rpc_trapEnable.set_cmd(TRAP_STATUE_ENABLE);
	bool isTrapEnable = false;
	RPC rpc_trapDisable;
	rpc_trapDisable.set_cmd(TRAP_STATUE_DISABLE);
	bool isTrapDisable = false;
	float deltaTime = TimerMgr::GetInstance().GetDeltaTime();
	for(auto it = timerObjectList.begin();it!=timerObjectList.end();)
	{
		it->remainTime -= deltaTime;
		if(it->remainTime<=0)
		{
			switch (it->type) {
				case TIMER_LIGHT:
					if(SceneInfo->lightList[it->id].isActive)
					{
						SceneInfo->lightList[it->id].isActive = false;
						it->remainTime = LIGHTOFFTIME;
						isLightOff = true;
						rpc_lightOff.add_param(it->id);
					}else
					{
						SceneInfo->lightList[it->id].isActive = true;
						it->remainTime = LIGHTONTIME;
						isLightOn = true;
						rpc_lightOn.add_param(it->id);
					}
					++it;
					break;
				case TIMER_TRAP:
					if(SceneInfo->trapList[it->id].isActive)
					{
						SceneInfo->trapList[it->id].isActive = false;
						it->remainTime = TRAPTIME;
						isTrapDisable = true;
						rpc_trapDisable.add_param(it->id);
					}else
					{
						SceneInfo->trapList[it->id].isActive = true;
						it->remainTime = TRAPTIME;
						isTrapEnable = true;
						rpc_trapEnable.add_param(it->id);
					}
					++it;
					break;
				case TIMER_IMMUNITY:
					SceneInfo->characterList[it->id]->isImmunity = false;
					SceneMgr::GetInstance().SetDirty(RoomId,0);
					it = timerObjectList.erase(it);
					break;
				case TIMER_SPEEDDEBUFF:
					SceneInfo->characterList[it->id]->maxSpeed = MAXSPEED;
					SceneMgr::GetInstance().SetDirty(RoomId,0);
					it = timerObjectList.erase(it);
					break;
			}
		}else
		{
			++it;
		}
	}
	if(isLightOff)
	{
		SendRpc(rpc_lightOff);
	}
	if(isLightOn)
	{
		SendRpc(rpc_lightOn);
	}
	if(isTrapDisable)
	{
		SendRpc(rpc_trapDisable);
	}
	if(isTrapEnable)
	{
		SendRpc(rpc_trapEnable);
	}
}

void RPCHandle::UpdateMonsterTargetPos()
{
	for(auto it=monsterChaseMap.begin();it!=monsterChaseMap.end();++it)
	{
		SceneInfo->monsterList[it->first].targetpos = SceneInfo->characterList[it->second]->position;
	}
}

void RPCHandle::AddOrResetTimer(int id, ObjectTimerType type, float newTime)
{
	bool isSet = false;
	for(auto it = timerObjectList.begin(); it!= timerObjectList.end();++it)
	{
		if(it->type==type&&it->id==id)
		{
			it->remainTime = newTime;
			isSet = true;
			break;
		}
	}
	if(!isSet)
	{
		timerObjectList.emplace_back(type,id,newTime);
	}
}
