#include "logicMgr.h"
#include <filedb/filedb.h>
#include "../codec/codec.h"
#include "../utils/utils.h"
#include "scene/sceneMgr.h"
static char s_send_buff[1024 * 64];
extern PlayerMgr g_playerMgr;

LogicMgr::LogicMgr() 
{

}

void LogicMgr::init(uv_tcp_t* client, PlayerMgr* playerMgr) 
{
	string playerId =  playerMgr->GetPlayerIDByClient(client);
	int roomId;
	if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
	{
		//找断线
		string playerId = g_playerMgr.GetPlayerIDByClient(client);
		if (SceneMgr::GetInstance().disconnectPlayers.find(playerId) != SceneMgr::GetInstance().disconnectPlayers.end())
		{
			roomId = SceneMgr::GetInstance().disconnectPlayers[playerId];
			SceneMgr::GetInstance().LoadDisconnectPlayer(client, roomId);
		}
		else
		{
			return;
		}
	}
	else
	{
		roomId = SceneMgr::GetInstance().player2room[client];
		SceneMgr::GetInstance().LoadNewPlayer(client, roomId);
	}

	RoomInfo* room = SceneMgr::GetInstance().roomsInfo[roomId];
	if (!room)
	{
		return;
	}

	if (room->god == nullptr)
	{
		room->god = client;
	}

	for (int i = 0; i < room->synInfo->characterList.size(); i++)
	{
		room->synInfo->characterList[i]->markDirty = true;
	}
	dirtyRoom.insert(roomId);
}

void LogicMgr::quit(uv_tcp_t* client, PlayerMgr* playerMgr) 
{
	if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
	{
		return;
	}
	int roomId = SceneMgr::GetInstance().player2room[client];
	RoomInfo* room = SceneMgr::GetInstance().roomsInfo[roomId];

	string playerid = g_playerMgr.GetPlayerIDByClient(client);

	SceneMgr::GetInstance().disconnectPlayers[playerid] = roomId;

	RemoveCharacterRsp msg;
	if (!room)
	{
		return;
	}
	int netId = room->playerid2netId[playerid];
	msg.set_netid(netId);
	int len = encode(s_send_buff, SERVER_CHARACTERREMOVE_RSP, msg.SerializeAsString().c_str(), msg.ByteSize());

	for (auto target : SceneMgr::GetInstance().room2players[roomId])
	{
		if (target == client) 
		{
			continue;
		}
		sendData((uv_stream_t*)target, s_send_buff, len);
	}
}

void LogicMgr::receive_statue_update(uv_tcp_t* client, CharacterStatueReq* statueReq) 
{
	string playerId = g_playerMgr.GetPlayerIDByClient(client);
	if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
	{
		return;
	}
	int roomId = SceneMgr::GetInstance().player2room[client];
	RoomInfo* room = SceneMgr::GetInstance().roomsInfo[roomId];
	if (!room)
	{
		return;
	}
	
	int netId = room->playerid2netId[playerId];

	CharacterStatue* characterStatue = room->synInfo->characterList[netId];
	if (!characterStatue)
	{
		return;
	}
	if (statueReq->netid() != characterStatue->netId) 
	{
		return;
	}

	dirtyRoom.insert(roomId);
	characterStatue->markDirty = true;
	characterStatue->speed = statueReq->speed();
	characterStatue->position.x = statueReq->position().x();
	characterStatue->position.y = statueReq->position().y();
	characterStatue->position.z = statueReq->position().z();
	characterStatue->rotation.x = statueReq->rotation().x();
	characterStatue->rotation.y = statueReq->rotation().y();
	characterStatue->rotation.z = statueReq->rotation().z();
	characterStatue->speed = statueReq->speed();
}

void LogicMgr::broad_statue() 
{
	if (dirtyRoom.empty())
	{
		return;
	}

	for (auto roomId : dirtyRoom)
	{
		if (SceneMgr::GetInstance().roomsInfo.find(roomId) == SceneMgr::GetInstance().roomsInfo.end())
		{
			continue;
		}
		RoomInfo* room = SceneMgr::GetInstance().roomsInfo[roomId];

		// 提前结束
		if (room->isEnd)
		{
			TimerMgr::GetInstance().RemoveTimer(room->gameTimerId);
			SceneMgr::GetInstance().GameOver(roomId);
			return;
		}

		// 角色同步
		NetAsyncMsg netAsyncMsg;
		for (int i = 0; i < room->synInfo->characterList.size(); ++i)
		{
			if (room->synInfo->characterList[i]->markDirty == false)
			{
				continue;
			}
			else
			{
				room->synInfo->characterList[i]->markDirty = false;
			}

			CharacterStatueMsg* characterStatueMsg = netAsyncMsg.add_characterstatuelist();
			CharacterStatue* characterStatue = room->synInfo->characterList[i];
			characterStatueMsg->set_netid(characterStatue->netId);
			characterStatueMsg->set_hp(characterStatue->hp);
			characterStatueMsg->set_speed(characterStatue->speed);
			characterStatueMsg->set_maxspeed(characterStatue->maxSpeed);
			characterStatueMsg->set_isdeath(characterStatue->isDeath);
			characterStatueMsg->set_isarmed(characterStatue->isArmed);
			characterStatueMsg->set_isimmunity(characterStatue->isImmunity);
			characterStatueMsg->set_actiontype(characterStatue->actionType);
			characterStatueMsg->set_score(characterStatue->score);
			Vec3Msg* pos = characterStatueMsg->mutable_position();
			pos->set_x(characterStatue->position.x);
			pos->set_y(characterStatue->position.y);
			pos->set_z(characterStatue->position.z);
			Vec3Msg* rot = characterStatueMsg->mutable_rotation();
			rot->set_x(characterStatue->rotation.x);
			rot->set_y(characterStatue->rotation.y);
			rot->set_z(characterStatue->rotation.z);
		}

		int len = encode(s_send_buff, SERVER_BROADCAST_CHARACTERSTATUE, netAsyncMsg.SerializeAsString().c_str(), netAsyncMsg.ByteSize());
		for (auto client : SceneMgr::GetInstance().room2players[roomId])
		{
			cout << "CHARACTERSTATUE" << len << endl;
			sendData((uv_stream_t*)client, s_send_buff, len);
		}
		
		// 子弹同步
		BulletsSynMsg bulletsSynMsg;
		for (auto& bullet : room->synInfo->bulletList)
		{
			BulletSynMsg* msg = bulletsSynMsg.add_bulletlist();
			msg->set_id(bullet.second.id);
			Vec3Msg* pos = msg->mutable_position();
			pos->set_x(bullet.second.position.x);
			pos->set_y(bullet.second.position.y);
			pos->set_z(bullet.second.position.z);
			Vec3Msg* rot = msg->mutable_rotation();
			rot->set_x(bullet.second.rotation.x);
			rot->set_y(bullet.second.rotation.y);
			rot->set_z(bullet.second.rotation.z);
			msg->set_isactive(bullet.second.isActive);

			if (!bullet.second.isActive)
			{
				SceneMgr::GetInstance().bulletPool.push_back(bullet.first);
			}
		}

		if (!SceneMgr::GetInstance().bulletPool.empty())
		{
			for (auto i : SceneMgr::GetInstance().bulletPool)
			{
				room->synInfo->bulletList.erase(i);
			}
			SceneMgr::GetInstance().bulletPool.clear();
		}
		len = encode(s_send_buff, SERVER_BULLETSYN_RSP, bulletsSynMsg.SerializeAsString().c_str(), bulletsSynMsg.ByteSize());
		for (auto client : SceneMgr::GetInstance().room2players[roomId])
		{
			cout << "bull" << len << endl;
			sendData((uv_stream_t*)client, s_send_buff, len);
		}

		MonstersSynMsg rsp;
		uv_tcp_t* god = room->god;
		string playerId = "";
		for (auto member : g_playerMgr.m_playerMap)
		{
			if (member.first == god)
			{
				playerId = member.second->PlayerID;
				break;
			}
		}
		int netId = -1;
		if (room->playerid2netId.find(playerId) != room->playerid2netId.end())
		{
			netId = room->playerid2netId[playerId];
		}
		rsp.set_netid(netId);

		// 怪物同步
		for (auto& monsterData : room->synInfo->monsterList)
		{
			MonsterSynMsg* msg = rsp.add_monsterlist();
			int id = monsterData.second.id;
			int hp = monsterData.second.hp;
			float posx = monsterData.second.position.x;
			float posy = monsterData.second.position.y;
			float posz = monsterData.second.position.z;
			float rotx = monsterData.second.rotation.x;
			float roty = monsterData.second.rotation.y;
			float rotz = monsterData.second.rotation.z;
			float tarx = monsterData.second.targetpos.x;
			float tary = monsterData.second.targetpos.y;
			float tarz = monsterData.second.targetpos.z;
			msg->set_id(id);
			msg->set_hp(hp);
			Vec3Msg* pos = msg->mutable_position();
			pos->set_x(posx);
			pos->set_y(posy);
			pos->set_z(posz);
			Vec3Msg* rot = msg->mutable_rotation();
			rot->set_x(rotx);
			rot->set_y(roty);
			rot->set_z(rotz);
			Vec3Msg* tar = msg->mutable_targetpos();
			tar->set_x(tarx);
			tar->set_y(tary);
			tar->set_z(tarz);
		}

		len = encode(s_send_buff, SERVER_MONSTERSYN_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
		for (auto client : SceneMgr::GetInstance().room2players[roomId])
		{
			cout << "monster" << len << endl;
			sendData((uv_stream_t*)client, s_send_buff, len);
		}


	}

	dirtyRoom.clear();
}

void LogicMgr::clearAll() 
{
	//clientPool.clear();
	dirtyRoom.clear();
}