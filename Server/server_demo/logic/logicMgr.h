#pragma once
#include "../../utils/baseData.h"
#include "player/player.h"

using namespace std;

class LogicMgr
{

public:

	LogicMgr();

	void init(uv_tcp_t* client, PlayerMgr* playerMgr);

	void quit(uv_tcp_t* client, PlayerMgr* playerMgr);

	void receive_statue_update(uv_tcp_t* client, CharacterStatueReq* statueReq);

	void broad_statue();

	void clearAll();

public:

	set<int> dirtyRoom;
	//vector<uv_tcp_t*> clientPool;
	//unordered_map<uv_tcp_t*, CharacterStatue*> client2Character;
};

