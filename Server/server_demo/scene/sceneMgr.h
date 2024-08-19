#pragma once
#include <functional>
#include "../../utils/baseData.h"
#include "rpcHandle.h"

using namespace std;
class RoomClear;

class SceneMgr {
public:
	static SceneMgr& GetInstance();
	SceneMgr();
	~SceneMgr();

	void Quit(uv_tcp_t* client);
	void OnRecvSceneInfoReq(uv_tcp_t* client);
	void OnRecvSceneBriefReq(uv_tcp_t* client);
	void OnRecvEnemyConfigReq(uv_tcp_t* client);
	void OnRecvRoomReq(uv_tcp_t* client, const RoomInfoReq* req);
	void OnRecvRoomCreateReq(uv_tcp_t* client, const CreateRoomReq* req);
	void OnRecvRoomEnterReq(uv_tcp_t* client, const EnterRoomReq* req);
	void OnRecvRankListReq(uv_tcp_t* client, const RankListReq* req);
	void OnRecvStartReq(uv_tcp_t* client);
	void OnRecvMonsterSynReq(uv_tcp_t* client, const MonstersSynMsg* syn);
	void OnRecvGameTimeReq(uv_tcp_t* client);
	void OnRecvGameScoreReq(uv_tcp_t* client);
	void OnRecvGameContinueReq(uv_tcp_t* client);
	void OnRecvAttackReq(uv_tcp_t* client, const AttackReq* req);
	void OnRecvBulletsSynReq(uv_tcp_t* client, const BulletsSynMsg* req);
	void OnRecvRPCReq(uv_tcp_t* client, RPC* req);
	void OnRecvCollisionReq(uv_tcp_t* client, CollisionReq* req);
	void SaveDisconnectPlayer(uv_tcp_t* client);
	void LoadDisconnectPlayer(uv_tcp_t* client, int roomId);
	void LoadNewPlayer(uv_tcp_t* client, int roomId);
	void UpdateStatus();
	SynInfo* GetSceneInfo(uv_tcp_t* client);
	int GetCharacterNetid(uv_tcp_t* client);
	vec3 GetFinishPos(int roomId);
	list<uv_tcp_t*> GetClientInRoom(int roomId);
	int GetWeaponDamage(int weaponId);
	MonsterAttribute* GetMonsterAttribute(SceneObjectType type);
	// type = 0 player; 
	// type = 1 monster;
	// type = 2 sceneObject;
	// type = 3 bullet;
	void SetDirty(int roomId, int type);
	void PlayerSuccess(int roomId, int netId);
	void PlayerDeath(int roomId, int netId);
	void GameOver(int roomId);

private:
	void LoadBriefInfo();
	void LoadEnemyConfig();
	void LoadRankList();
	void SaveRankList();
	void LoadSceneInfo(const int roomId);
	void SendRoomReady(const int roomId);
	void RoomNumberChange(const int roomId);
	void UpdateMonstList(RoomInfo* room, const MonstersSynMsg* syn);
	void GameStart(int roomId);
	void CloseRoom(int roomId);
	int GenRandom(int max);
	float distance(vec3 a, vec3 b);



	SceneBriefRsp sceneBriefRsp;
	MonsterAttributeRsp enemyConfigRsp;
	random_device rd; 
	mt19937 gen;

public:
	// roomId - RoomInfo
	map<int, RoomInfo*> roomsInfo;
	// sceneId - RoomInfoList
	map<int, list<RoomInfo*>> scene2roomList;
	// client - roomId
	map<uv_tcp_t*, int> player2room;
	// roomId - clientList
	map<int, list<uv_tcp_t*>> room2players;
	// playerid - roomid
	map<string, int> disconnectPlayers;
	map<SceneObjectType, MonsterAttribute> monstersAttribute;
	vector<int> timerPool;
	vector<RoomClear*> clearPool;
	vector<int> bulletPool;
	// score
	vector<multimap<int, RankList>> rankLists;
	int LevelCount;
	int roomIndex;
	int bulletIndex;
};

class RoomClear {
public:
	RoomClear(int id) : roomId(id) {}

	void GameOver();

	int roomId;
};
