#include "nethandle.h"
#include "../codec/codec.h"
#include "player/player.h"
#include <map>
#include "logic/logicMgr.h"
#include "scene/sceneMgr.h"
#include "item/item.h"

using namespace std;

static map<uv_tcp_t*, PeerData*> s_clientMap;
static char s_send_buff[64 * 1024];

extern PlayerMgr g_playerMgr;
extern LogicMgr g_logicMgr;


// 发送一个pb对象到客户端
bool SendPBToClient(uv_tcp_t* client, uint16_t cmd, ::google::protobuf::Message* msg) {
    string data = msg->SerializeAsString();
    int len = encode(s_send_buff, cmd, data.c_str(), (int)data.length());
    return sendData((uv_stream_t*)client, s_send_buff, len);
}

bool OnNewClient(uv_tcp_t* client) {
    bool result = false;
    PeerData* peerData = malloc_peer_data();
    if (!peerData) {
        goto Exit0;
    }
    s_clientMap.insert(pair<uv_tcp_t*, PeerData*>(client, peerData));
    result = true;
Exit0:
    return result;
}

bool OnCloseClient(uv_tcp_t* client) {
    g_logicMgr.quit(client, &g_playerMgr);
    SceneMgr::GetInstance().Quit(client);
    g_playerMgr.on_client_close(client);
    auto it = s_clientMap.find(client);
    if (it != s_clientMap.end()) {
        free_peer_data(it->second);
        s_clientMap.erase(it);
    }

    return true;
}


// 收到网络层发过来的数据
bool OnRecv(uv_tcp_t* client, const char* data, int len) {
    bool result = false;
    PeerData* peerData = nullptr;

    auto it = s_clientMap.find(client);
    if (it == s_clientMap.end()) {
        goto Exit0;
    }
    peerData = it->second;

    if (sizeof(peerData->recv_buf) - peerData->nowPos < len) {
        goto Exit0;
    }

    memcpy(peerData->recv_buf + peerData->nowPos, data, len);
    peerData->nowPos += len;

    // 完整协议包检测，处理网络粘包
    while (true) {
        int packPos = check_pack(peerData->recv_buf, peerData->nowPos);
        if (packPos < 0) {          // 异常
            goto Exit0;
        }
        else if (packPos > 0) {     // 协议数据完整了
            Packet pack;        // 协议包格式
            // 解码，得到协议号和序列化后的数据
            if (!decode(&pack, peerData->recv_buf, packPos)) {
                goto Exit0;
            }
            // 处理一个数据包
            if (!_OnPackHandle(client, &pack)) {
                goto Exit0;
            }
            if (packPos >= peerData->nowPos) {
                peerData->nowPos = 0;
            }
            else {
                memmove(peerData->recv_buf, peerData->recv_buf + packPos, peerData->nowPos - packPos);
                peerData->nowPos -= packPos;
            }
        }
        else {
            // 没有完全到达完整包
            break;
        }
    }

    result = true;
Exit0:
    return result;
}

// 处理一个完成的消息包
bool _OnPackHandle(uv_tcp_t* client, Packet* pack) {
    bool result = false;
    int len = 0;
    // todo 处理收到的数据包
    fprintf(stdout, "OnPackHandle: cmd:%d, len:%d, client:%llu\n", pack->cmd, pack->len, (uint64_t)client);
    switch (pack->cmd) {
        case CLIENT_PING:           // 处理客户端的ping
        {
            fprintf(stdout, "client ping, client:%llu\n", (uint64_t)client);
            len = encode(s_send_buff, SERVER_PONG, nullptr, 0);
            sendData((uv_stream_t*)client, s_send_buff, len);
            break;
        }
        case CLIENT_LOGIN_REQ:
        {
            PlayerLoginReq req;
            req.ParseFromArray(pack->data, pack->len);
            g_playerMgr.player_login(client, &req);
            break;
        }
        case CLIENT_LOGOUT_REQ:
        {
            g_playerMgr.on_client_close(client);
            break;
        }
        case CLIENT_CREATE_REQ:
        {
            PlayerCreateReq req;
            req.ParseFromArray(pack->data, pack->len);
            g_playerMgr.player_create(client, &req);
            break;
        }
        case CLIENT_ANNOUNCE_REQ:
        {
            g_playerMgr.announce_request(client);
            break;
        }

        case CLIENT_UPDATESTATUE_REQ: 
        {
            CharacterStatueReq req;
            req.ParseFromArray(pack->data, pack->len);
            g_logicMgr.receive_statue_update(client, &req);
            break;
        }
        case CLIENT_CHARACTERADD_REQ:
        {
            g_logicMgr.init(client, &g_playerMgr);
            break;
        }
        case CLIENT_SCENEBRIEF_REQ:
        {
            SceneMgr::GetInstance().OnRecvSceneBriefReq(client);
            break;
        }
        case CLIENT_MONSTERATTRIBUTE_REQ:
        {
            SceneMgr::GetInstance().OnRecvEnemyConfigReq(client);
            break;
        }
        case CLIENT_ROOM_REQ:
        {
            RoomInfoReq req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvRoomReq(client, &req);
            break;
        }
        case CLIENT_CREATEROOM_REQ:
        {
            CreateRoomReq req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvRoomCreateReq(client, &req);
            break;
        }
        case CLIENT_ENTERROOM_REQ:
        {
            EnterRoomReq req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvRoomEnterReq(client, &req);
            break;
        }
        case CLIENT_SCENEDETAIL_REQ:
        {
            SceneMgr::GetInstance().OnRecvSceneInfoReq(client);
            break;
        }
        case CLIENT_EXITROOM_REQ:
        {
            g_logicMgr.quit(client, &g_playerMgr);
            SceneMgr::GetInstance().Quit(client);
            break;
        }
        case CLIENT_RANKLIST_REQ:
        {
            RankListReq req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvRankListReq(client, &req);
            break;
        }
        case CLIENT_START_REQ:
        {
            SceneMgr::GetInstance().OnRecvStartReq(client);
            break;
        }
        case CLIENT_MONSTERSYN_REQ:
        {
            MonstersSynMsg req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvMonsterSynReq(client, &req);
            break;
        }
        case CLIENT_GAMETIME_REQ:
        {
            SceneMgr::GetInstance().OnRecvGameTimeReq(client);
            break;
        }
        case CLIENT_GAMESCORE_REQ:
        {
            SceneMgr::GetInstance().OnRecvGameScoreReq(client);
            break;
        }
        case CLIENT_GAMECONTINUE_REQ:
        {
            SceneMgr::GetInstance().OnRecvGameContinueReq(client);
            break;
        }
        case CLIENT_COLLISIONCHECK_REQ:
        {
            CollisionReq req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvCollisionReq(client, &req);
            break;
        }
        case CLIENT_RPC_REQ:
        {
            RPC req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvRPCReq(client, &req);
            break;
        }
        case CLIENT_ATTACK_REQ:
        {
            AttackReq req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvAttackReq(client, &req);
        }
        case CLIENT_BULLETSYN_REQ:
        {
            BulletsSynMsg req;
            req.ParseFromArray(pack->data, pack->len);
            SceneMgr::GetInstance().OnRecvBulletsSynReq(client, &req);
            break;
        }
        case CLIENT_BUYITEM_REQ:
        {
            BuyReq req;
            req.ParseFromArray(pack->data, pack->len);
            ItemMgr::GetInst().buy(client, &req, &g_playerMgr);
            break;
        }
        case CLIENT_DELETEITEM_REQ:
        {
            DeleteItemReq req;
            req.ParseFromArray(pack->data, pack->len);
            ItemMgr::GetInst().remove_Item(client, &req, &g_playerMgr);
            break;
        }
        case CLIENT_GETSHOPITEMS_REQ:
        {
            ItemMgr::GetInst().shop_get(client);
            break;
        }
        case CLIENT_GETBAGITEMS_REQ:
        {
            ItemMgr::GetInst().bag_get(client, &g_playerMgr);
            break;
        }
        case CLIENT_GETMONEY_REQ:
        {
            ItemMgr::GetInst().money_get(client);
            break;
        }
        case CLIENT_ADDMONEY_REQ:
        {
            AddMoneyReq req;
            req.ParseFromArray(pack->data, pack->len);
            ItemMgr::GetInst().money_add(client, &req, &g_playerMgr);
            break;
        }
        case CLIENT_ITEMCONFIG_REQ:
        {
            ItemMgr::GetInst().config_get(client);
            break;
        }

        default:
            fprintf(stderr, "这个还没做呢*_*:%d\n", pack->cmd);
            break;
            return false;
    }
    result = true;
Exit0:
    return result;
}

