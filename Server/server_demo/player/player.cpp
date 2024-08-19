#pragma once
#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include "filedb/filedb.h"
#include "../codec/codec.h"
#include "../utils/utils.h"


static char s_send_buff[1024*64];
extern ServerCfg g_config;

PlayerMgr::PlayerMgr() {

}

PlayerMgr::~PlayerMgr() {

}

bool PlayerMgr::init() {
    m_playerMap.clear();
    m_announce = g_config.default_announce;
    return true;
}

bool PlayerMgr::un_init() {
    for (auto it = m_playerMap.begin(); it != m_playerMap.end(); ++it) {
        delete it->second;
    }
    m_playerMap.clear();
    return true;
}

// 客户端拉取公告内容
bool PlayerMgr::announce_request(uv_tcp_t* client) {
    fprintf(stdout, "announce request\n");
    SyncAnnounce sync;
    int len = 0;
    sync.set_announce(m_announce);

    len = encode(s_send_buff, SERVER_ANNOUNCE_RSP, sync.SerializeAsString().c_str(), sync.ByteSize());

    return sendData((uv_stream_t*)client, s_send_buff, len);
}

// 根据playerID查找玩家
Player* PlayerMgr::find_player(string playerID) {
    for (auto it = m_playerMap.begin(); it != m_playerMap.end(); ++it) {
        if (it->second->PlayerID == playerID)
            return it->second;
    }
    return nullptr;
}

string PlayerMgr::GetPlayerIDByClient(uv_tcp_t* client) {
    string id = "";
    if (client != nullptr && m_playerMap.find(client) != m_playerMap.end()) {
        id = m_playerMap[client]->PlayerID;
    }
    return id;
}

// 客户端断链，清除对应的玩家对象
bool PlayerMgr::on_client_close(uv_tcp_t* client) {
    auto it = m_playerMap.find(client);
    if (it != m_playerMap.end()) {
        _save_player(it->second);
        delete it->second;
        m_playerMap.erase(it);
    }
    return true;
}

// 广播公告
bool PlayerMgr::broadcast_announce(string announce) {
    SyncAnnounce sync;
    int len = 0;

    m_announce = announce;
    sync.set_announce(announce);

    if (m_playerMap.empty())
        return true;

    len = encode(s_send_buff, SERVER_ANNOUNCE_RSP, sync.SerializeAsString().c_str(), sync.ByteSize());

    for (auto it = m_playerMap.begin(); it != m_playerMap.end(); ++it) {
        sendData((uv_stream_t*)it->first, s_send_buff, len);
    }
    return true;
}

// 玩家登录
bool PlayerMgr::player_login(uv_tcp_t* client, const PlayerLoginReq* req) {
    // todo （作业6）实现以下内容
    // 1. 查看玩家是否已经在游戏中，如果在游戏中则应答客户端登录失败
    // 2. 从文件中读取玩家数据并展开
    // 3. 检验玩家登录的用户名密码与文件中的是否匹配
    // 4. 创建玩家对象，并构造
    // 5. 把玩家对象加入m_playerMap中进行管理
    // 6. 应答客户端登录结果

    // 注意：在业务处理过程中，除非发生内部错误，否则不要返回false，这会导致底层进行错误清理
    PlayerLoginRsp playerLoginRsp;
    if (find_player(req->playerid())) {
        playerLoginRsp.set_reason("Has Login");
        playerLoginRsp.set_result(-1);
    }
    else {
        PlayerSaveData playerData;
        if (_load_player(req->playerid(), &playerData)) {
            if (playerData.password() == req->password()) {
                Player* player = nullptr;
                if (m_playerMap.find(client) != m_playerMap.end()) {
                    player = m_playerMap[client];
                }
                else{
                    player = new Player();
                }
                player->Name = playerData.name();
                player->Password = playerData.password();
                player->PlayerID = playerData.playerid();
                m_playerMap[client] = player;
                playerLoginRsp.set_reason("Login Success");
                playerLoginRsp.set_result(0);
                PlayerSyncData* syn = playerLoginRsp.mutable_playerdata();
                syn->set_name(playerData.name());
                syn->set_playerid(playerData.playerid());
                ItemMgr::GetInst().InitBag(client, this);
            }
            else {
                playerLoginRsp.set_reason("Wrong password");
                playerLoginRsp.set_result(-2);
            }
        }
        else {
            playerLoginRsp.set_reason("Not Exist");
            playerLoginRsp.set_result(-3);
        }
    }
    fprintf(stdout, "%s %s\n", req->playerid().c_str(), playerLoginRsp.reason().c_str());
    int len = encode(s_send_buff, SERVER_LOGIN_RSP, playerLoginRsp.SerializeAsString().c_str(), playerLoginRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
    return true;
}

// 玩家注册
bool PlayerMgr::player_create(uv_tcp_t* client, const PlayerCreateReq* req) {
    // todo （作业6）实现以下内容
    // 1. 查看玩家是否已经在游戏中，如果在游戏中登录了则返回失败
    // 2. 尝试从文件中加载玩家数据，如果加载成功则该账号已经被注册，返回失败
    // 3. 创建玩家对象并构造
    // 4. 对玩家做一次存盘操作
    // 5. 把玩家对象加入m_playerMap中进行管理
    // 6. 应答客户端注册结果

    // 注意：在业务处理过程中，除非发生内部错误，否则不要返回false，这会导致底层进行错误清理
    PlayerCreateRsp playerCreateRsp;
    if (find_player(req->playerid())) {
        playerCreateRsp.set_reason("Has Login");
        playerCreateRsp.set_result(-1);
    }
    else {
        PlayerSaveData playerData;
        if (_load_player(req->playerid(), &playerData)) {
            playerCreateRsp.set_reason("Has Regist");
            playerCreateRsp.set_result(-2);
        }
        else {
            Player* player = nullptr;
            if (m_playerMap.find(client) != m_playerMap.end()) {
                player = m_playerMap[client];
            }
            else{
                player = new Player();
            }
            player->Name = req->name();
            player->PlayerID = req->playerid();
            player->Password = req->password();
            if (_save_player(player)) {
                m_playerMap[client] = player;
                playerCreateRsp.set_name(player->Name);
                playerCreateRsp.set_playerid(player->PlayerID);
                playerCreateRsp.set_reason("Regist Success");
                playerCreateRsp.set_result(0);
            }
            else {
                playerCreateRsp.set_reason("Server Save Error");
                playerCreateRsp.set_result(-3);
            }
        }
    }
    fprintf(stdout, "%s %s\n", req->playerid().c_str(), playerCreateRsp.reason().c_str());
    int len = encode(s_send_buff, SERVER_CREATE_RSP, playerCreateRsp.SerializeAsString().c_str(), playerCreateRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
    
    return true;
}

// 从文件中加载玩家数据，成功返回true，失败返回false
bool PlayerMgr::_load_player(string playerID, PlayerSaveData* playerData) {
    // todo （作业5） 从文件中读取数据，并反序列化到playerData中，参考load函数
    if (playerData == nullptr) {
        return false;
    }
    char data[512] = { 0 };
    int result = load(playerID.c_str(), data, "AccountInfo", 512);
    if (result > 0) {
        playerData->ParseFromArray(data, result);
        return true;
    }
    return false;
}

// 把玩家数据保存到文件中，成功返回true，失败返回false
bool PlayerMgr::_save_player(const Player* player) {
    // todo （作业5）把玩家数据序列化后存盘，参考save函数
    if (player == nullptr) {
        return false;
    }
    PlayerSaveData playerData;
    playerData.set_name(player->Name);
    playerData.set_password(player->Password);
    playerData.set_playerid(player->PlayerID);
    char data[512];
    playerData.SerializeToArray(data, playerData.ByteSize());
    int result = save(player->PlayerID.c_str(), data, "AccountInfo", playerData.ByteSize());
    if (result == 0) {
        return true;
    }
    return false;
}

