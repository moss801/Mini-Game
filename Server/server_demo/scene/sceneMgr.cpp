#include "sceneMgr.h"
#include "../../cJSON/cJSON.h"
#include <filedb/filedb.h>
#include "../../codec/codec.h"
#include "../../utils/utils.h"
#include "logic/logicMgr.h"
#include<chrono>

const char* sceneConfigPath = "config/sceneConfig.json";
const char* enemyConfigPath = "config/enemyConfig.json";
const char* characterConfigPath = "config/characterConfig.json";
static char s_send_buff[1024 * 64];
const float BIAS = 0.5;
const int GAMETIME = 180;
const float BULLETSPEED = 5.0;
const int ATTACKCD = 0.5 * 1000;

extern PlayerMgr g_playerMgr;
extern LogicMgr g_logicMgr;

SceneMgr& SceneMgr::GetInstance()
{
    static SceneMgr sceneMgr;
    return sceneMgr;
}

SceneMgr::SceneMgr() : gen(rd()) {
    LevelCount = 0;
    LoadBriefInfo();
    LoadEnemyConfig();
    rankLists.resize(LevelCount + 1);
    LoadRankList();
    roomIndex = 1;
    bulletIndex = 0;
}

SceneMgr::~SceneMgr() {

}

void SceneMgr::SetDirty(int roomId, int type)
{
    g_logicMgr.dirtyRoom.insert(roomId);
}

void SceneMgr::PlayerSuccess(int roomId, int netId)
{
    if (roomsInfo.find(roomId) == roomsInfo.end())
    {
        return;
    }

    int time = (TimerMgr::GetInstance().GetServerTime() - roomsInfo[roomId]->startTime) / 1000.0;
    if (roomsInfo[roomId]->synInfo == nullptr)
    {
        return;
    }
    SynInfo* syn = roomsInfo[roomId]->synInfo;

    if (syn->characterList.find(netId) == syn->characterList.end())
    {
        return;
    }
    if (syn->characterList[netId]->isSuccess)
    {
        return;
    }

    syn->characterList[netId]->isSuccess = true;
    syn->characterList[netId]->time = time;
    
    string playerId = "";
    for (auto it : roomsInfo[roomId]->playerid2netId)
    {
        if (it.second == netId)
        {
            playerId = it.first;
            break;
        }
    }
    uv_tcp_t* client = nullptr;
    for (auto player : g_playerMgr.m_playerMap)
    {
        if (player.second->PlayerID == playerId)
        {
            client = player.first;
            break;
        }
    }
    if (!client)
    {
        return;
    }
    
    GameResultRsp rsp;
    rsp.set_isfinished(true);
    int len = encode(s_send_buff, SERVER_END_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);

    bool result = true;
    for (auto it : syn->characterList)
    {
        result &= (it.second->isSuccess || it.second->isDeath);
    }
    if (result)
    {
        roomsInfo[roomId]->isEnd = true;
        g_logicMgr.dirtyRoom.insert(roomId);
    }
}

void SceneMgr::PlayerDeath(int roomId, int netId)
{
    if (roomsInfo.find(roomId) == roomsInfo.end())
    {
        return;
    }

    int time = (TimerMgr::GetInstance().GetServerTime() - roomsInfo[roomId]->startTime) / 1000.0;
    if (roomsInfo[roomId]->synInfo == nullptr)
    {
        return;
    }
    SynInfo* syn = roomsInfo[roomId]->synInfo;

    if (syn->characterList.find(netId) == syn->characterList.end())
    {
        return;
    }
    if (syn->characterList[netId]->isSuccess)
    {
        return;
    }

    syn->characterList[netId]->isSuccess = false;
    syn->characterList[netId]->time = time;

    bool result = true;
    for (auto it : syn->characterList)
    {
        result &= (it.second->isSuccess || it.second->isDeath);
    }
    if (result)
    {
        roomsInfo[roomId]->isEnd = true;
        g_logicMgr.dirtyRoom.insert(roomId);
    }
}

void SceneMgr::LoadBriefInfo() {
    int len = readCfg(sceneConfigPath, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", sceneConfigPath);
        return;
    }
    char* temp = (char*)malloc(len);
    readCfg(sceneConfigPath, temp, len);
    const cJSON* scenes = NULL;
    const cJSON* scene = NULL;
    const cJSON* id = NULL;
    const cJSON* brief = NULL;
    const cJSON* name = NULL;
    const cJSON* gargoyle = NULL;
    const cJSON* ghost = NULL;
    const cJSON* difficult = NULL;
    int length = 0;
    cJSON* monitor_json = cJSON_Parse(temp);
    if (monitor_json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto end;
    }
    scenes = cJSON_GetObjectItemCaseSensitive(monitor_json, "scenes");
    if (!cJSON_IsArray(scenes)) {
        fprintf(stderr, "invalid config, scenes field must array\n");
        goto end;
    }
    length = cJSON_GetArraySize(scenes);
    LevelCount = length;
    for (int i = 0; i < length; i++) {
        scene = cJSON_GetArrayItem(scenes, i);
        id = cJSON_GetObjectItemCaseSensitive(scene, "id");
        brief = cJSON_GetObjectItemCaseSensitive(scene, "brief");
        name = cJSON_GetObjectItemCaseSensitive(brief, "name");
        gargoyle = cJSON_GetObjectItemCaseSensitive(brief, "gargoyle");
        ghost = cJSON_GetObjectItemCaseSensitive(brief, "ghost");
        difficult = cJSON_GetObjectItemCaseSensitive(brief, "difficult");
        SceneBriefMsg* sceneBriefMsg = sceneBriefRsp.add_scenebrief();
        sceneBriefMsg->set_sceneid(id->valueint);
        sceneBriefMsg->set_difficult(difficult->valueint);
        sceneBriefMsg->set_gargoylecount(gargoyle->valueint);
        sceneBriefMsg->set_ghostcount(ghost->valueint);
        sceneBriefMsg->set_scenename(name->valuestring);
    }
    end:
        cJSON_Delete(monitor_json);
        free(temp);
}

void SceneMgr::LoadEnemyConfig()
{
    int len = readCfg(enemyConfigPath, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", enemyConfigPath);
        return;
    }
    char* temp = (char*)malloc(len);
    readCfg(enemyConfigPath, temp, len);
    const cJSON* enemys = NULL;
    const cJSON* enemy = NULL;
    const cJSON* name = NULL;
    const cJSON* type_id = NULL;
    const cJSON* damage = NULL;
    const cJSON* health = NULL;
    const cJSON* speed = NULL;
    const cJSON* rebirthtime = NULL;
    const cJSON* killpoint = NULL;
    const cJSON* detectRange = NULL;
    int length = 0;
    cJSON* monitor_json = cJSON_Parse(temp);
    if (monitor_json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto end;
    }
    enemys = cJSON_GetObjectItemCaseSensitive(monitor_json, "enemys");
    if (!cJSON_IsArray(enemys)) {
        fprintf(stderr, "invalid config, enemys field must array\n");
        goto end;
    }

    length = cJSON_GetArraySize(enemys);
    for (int i = 0; i < length; i++) {
        enemy = cJSON_GetArrayItem(enemys, i);
        name = cJSON_GetObjectItemCaseSensitive(enemy, "name");
        type_id = cJSON_GetObjectItemCaseSensitive(enemy, "typeid");
        damage = cJSON_GetObjectItemCaseSensitive(enemy, "damage");
        health = cJSON_GetObjectItemCaseSensitive(enemy, "health");
        speed = cJSON_GetObjectItemCaseSensitive(enemy, "speed");
        rebirthtime = cJSON_GetObjectItemCaseSensitive(enemy, "rebirthtime");
        killpoint = cJSON_GetObjectItemCaseSensitive(enemy, "killpoint");
        detectRange = cJSON_GetObjectItemCaseSensitive(enemy, "detectRange");
        SceneObjectType type = (SceneObjectType)type_id->valueint;

        MonsterAttribute attribute;
        attribute.damage = damage->valueint;
        attribute.detectRange = detectRange->valuedouble;
        attribute.health = health->valueint;
        attribute.killPoint = killpoint->valueint;
        attribute.rebirthTime = rebirthtime->valuedouble;
        attribute.speed = speed->valuedouble;

        monstersAttribute[type] = attribute;

        MonsterAttributeMsg* enemyConfig = enemyConfigRsp.add_attributelist();
        enemyConfig->set_name(name->valuestring);
        enemyConfig->set_type(type);
        enemyConfig->set_attack(damage->valueint);
        enemyConfig->set_hp(health->valueint);
        enemyConfig->set_speed(speed->valuedouble);
        enemyConfig->set_reborntime(rebirthtime->valuedouble);
        enemyConfig->set_value(killpoint->valueint);
        enemyConfig->set_detectrange(detectRange->valuedouble);
    }
end:
    cJSON_Delete(monitor_json);
    free(temp);
}

void SceneMgr::LoadRankList()
{
    char data[1024];
    int result = load("RankList", data, "RankList", 1024);
    if (result < 0)
    {
        return;
    }

    RankListData rankListData;
    rankListData.ParseFromArray(data, result);
    for (int i = 0; i < rankListData.rankdata_size(); ++i)
    {
        multimap<int, RankList> rank;
        RankListRsp rsp = rankListData.rankdata(i);
        for (int j = 0; j < rsp.ranklist_size(); ++j)
        {
            RankListMsg msg = rsp.ranklist(j);
            string name = msg.name();
            int score = msg.score();
            int time = msg.time();
            
            rank.insert(make_pair(score, RankList(name, score, time)));
        }
        rankLists[i] = rank;
    }

}

void SceneMgr::SaveRankList()
{
    RankListData rankListData;

    for (auto& rank : rankLists)
    {
        RankListRsp* rsp = rankListData.add_rankdata();
        for (auto& element : rank)
        {
            RankListMsg* msg = rsp->add_ranklist();
            msg->set_name(element.second.name);
            msg->set_score(element.second.score);
            msg->set_time(element.second.time);
        }
    }

    save("RankList", rankListData.SerializeAsString().c_str(), "RankList", rankListData.ByteSize());
}

void SceneMgr::LoadSceneInfo(const int roomId)
{
    int len = readCfg(sceneConfigPath, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", sceneConfigPath);
        return;
    }
    char* temp = (char*)malloc(len);
    readCfg(sceneConfigPath, temp, len);
    const cJSON* scenes = NULL;
    const cJSON* scene = NULL;
    const cJSON* id = NULL;
    const cJSON* gargoyles = NULL;
    const cJSON* gargoyle = NULL;
    const cJSON* ghosts = NULL;
    const cJSON* ghost = NULL;
    const cJSON* objects = NULL;
    const cJSON* object = NULL;
    const cJSON* hangoutpoints = NULL;
    const cJSON* hangoutpoint = NULL;
    const cJSON* detailId = NULL;
    const cJSON* position = NULL;
    const cJSON* rotation = NULL;
    int length = 0;
    int detailLengh = 0;

    cJSON* monitor_json = cJSON_Parse(temp);
    if (monitor_json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        cJSON_Delete(monitor_json);
        free(temp);
        return;
    }
    scenes = cJSON_GetObjectItemCaseSensitive(monitor_json, "scenes");
    if (!cJSON_IsArray(scenes)) {
        fprintf(stderr, "invalid config, scenes field must array\n");
        cJSON_Delete(monitor_json);
        free(temp);
        return;
    }

    RoomInfo* newSceneInfo = roomsInfo[roomId];
    int sceneId = newSceneInfo->sceneId;

    if (newSceneInfo->synInfo == nullptr)
    {
        cJSON_Delete(monitor_json);
        free(temp);
        return;
    }

    int ghostHp = monstersAttribute[GHOST].health;
    int garloyleHp = monstersAttribute[GARGOYLE].health;

    length = cJSON_GetArraySize(scenes);
    for (int i = 0; i < length; i++) {
        scene = cJSON_GetArrayItem(scenes, i);
        id = cJSON_GetObjectItemCaseSensitive(scene, "id");
        if (id->valueint == sceneId) {
            gargoyles = cJSON_GetObjectItemCaseSensitive(scene, "gargoyle");
            detailLengh = cJSON_GetArraySize(gargoyles);
            for (int j = 0; j < detailLengh; j++) {
                gargoyle = cJSON_GetArrayItem(gargoyles, j);
                detailId = cJSON_GetObjectItemCaseSensitive(gargoyle, "enemyid");
                position = cJSON_GetObjectItemCaseSensitive(gargoyle, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(gargoyle, "rotation");
                int enemyid = detailId->valueint;
                newSceneInfo->synInfo->monsterList.insert(make_pair(enemyid, MonsterData(enemyid, garloyleHp)));
                newSceneInfo->synInfo->monsterList[enemyid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].oriPosition = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].targetpos = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].type = GARGOYLE;
            }
            ghosts = cJSON_GetObjectItemCaseSensitive(scene, "ghost");
            detailLengh = cJSON_GetArraySize(ghosts);
            for (int j = 0; j < detailLengh; j++) {
                ghost = cJSON_GetArrayItem(ghosts, j);
                detailId = cJSON_GetObjectItemCaseSensitive(ghost, "enemyid");
                position = cJSON_GetObjectItemCaseSensitive(ghost, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(ghost, "rotation");
                int enemyid = detailId->valueint;
                newSceneInfo->synInfo->monsterList.insert(make_pair(enemyid, MonsterData(enemyid, ghostHp)));
                newSceneInfo->synInfo->monsterList[enemyid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].oriPosition = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].targetpos = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->monsterList[enemyid].type = GHOST;
            }
            objects = cJSON_GetObjectItemCaseSensitive(scene, "slime");
            detailLengh = cJSON_GetArraySize(objects);
            for (int j = 0; j < detailLengh; j++) {
                object = cJSON_GetArrayItem(objects, j);
                detailId = cJSON_GetObjectItemCaseSensitive(object, "objectid");
                position = cJSON_GetObjectItemCaseSensitive(object, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(object, "rotation");
                int objectid = detailId->valueint;
                newSceneInfo->synInfo->slimeList.insert(make_pair(objectid, ObjectData(objectid)));
                newSceneInfo->synInfo->slimeList[objectid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->slimeList[objectid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
            }
            objects = cJSON_GetObjectItemCaseSensitive(scene, "trap");
            detailLengh = cJSON_GetArraySize(objects);
            for (int j = 0; j < detailLengh; j++) {
                object = cJSON_GetArrayItem(objects, j);
                detailId = cJSON_GetObjectItemCaseSensitive(object, "objectid");
                position = cJSON_GetObjectItemCaseSensitive(object, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(object, "rotation");
                int objectid = detailId->valueint;
                newSceneInfo->synInfo->trapList.insert(make_pair(objectid, ObjectData(objectid)));
                newSceneInfo->synInfo->trapList[objectid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->trapList[objectid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
            }
            objects = cJSON_GetObjectItemCaseSensitive(scene, "door");
            detailLengh = cJSON_GetArraySize(objects);
            for (int j = 0; j < detailLengh; j++) {
                object = cJSON_GetArrayItem(objects, j);
                detailId = cJSON_GetObjectItemCaseSensitive(object, "objectid");
                position = cJSON_GetObjectItemCaseSensitive(object, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(object, "rotation");
                int objectid = detailId->valueint;
                newSceneInfo->synInfo->doorList.insert(make_pair(objectid, ObjectData(objectid)));
                newSceneInfo->synInfo->doorList[objectid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->doorList[objectid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
            }
            objects = cJSON_GetObjectItemCaseSensitive(scene, "light");
            detailLengh = cJSON_GetArraySize(objects);
            for (int j = 0; j < detailLengh; j++) {
                object = cJSON_GetArrayItem(objects, j);
                detailId = cJSON_GetObjectItemCaseSensitive(object, "objectid");
                position = cJSON_GetObjectItemCaseSensitive(object, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(object, "rotation");
                int objectid = detailId->valueint;
                newSceneInfo->synInfo->lightList.insert(make_pair(objectid, ObjectData(objectid)));
                newSceneInfo->synInfo->lightList[objectid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->lightList[objectid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
            }
            objects = cJSON_GetObjectItemCaseSensitive(scene, "box");
            detailLengh = cJSON_GetArraySize(objects);
            for (int j = 0; j < detailLengh; j++) {
                object = cJSON_GetArrayItem(objects, j);
                detailId = cJSON_GetObjectItemCaseSensitive(object, "objectid");
                position = cJSON_GetObjectItemCaseSensitive(object, "position");
                rotation = cJSON_GetObjectItemCaseSensitive(object, "rotation");
                int objectid = detailId->valueint;
                newSceneInfo->synInfo->boxList.insert(make_pair(objectid, ObjectData(objectid)));
                newSceneInfo->synInfo->boxList[objectid].position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
                    cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
                newSceneInfo->synInfo->boxList[objectid].rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
                    cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
            }

            hangoutpoints = cJSON_GetObjectItemCaseSensitive(scene, "hangoutpoint");
            detailLengh = cJSON_GetArraySize(hangoutpoints);
            for (int j = 0; j < detailLengh; j++) {
                hangoutpoint = cJSON_GetArrayItem(hangoutpoints, j);
                detailId = cJSON_GetObjectItemCaseSensitive(hangoutpoint, "pointid");
                position = cJSON_GetObjectItemCaseSensitive(hangoutpoint, "position");
                newSceneInfo->synInfo->patrolPoints.emplace_back(cJSON_GetArrayItem(position, 0)->valuedouble, cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
            }

            objects = cJSON_GetObjectItemCaseSensitive(scene, "endposition");
            detailLengh = cJSON_GetArraySize(objects);
            for (int j = 0; j < detailLengh; j++) {
                object = cJSON_GetArrayItem(objects, j);
                position = cJSON_GetObjectItemCaseSensitive(object, "position");
                int objectid = position->valueint;
                newSceneInfo->endPos = vec3(cJSON_GetArrayItem(position, 0)->valuedouble, cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
            }

            break;
        }
    }
}

void SceneMgr::SendRoomReady(const int roomId)
{
    int len = encode(s_send_buff, SERVER_ROOMREADY_RSP, "", 0);
    for (auto client : room2players[roomId])
    {
        sendData((uv_stream_t*)client, s_send_buff, len);
    }
}

void SceneMgr::RoomNumberChange(const int roomId)
{
    RoomNumChange rsp;
    int currentNum = roomsInfo[roomId]->currentNum;
    rsp.set_currentnum(currentNum);

    int len = encode(s_send_buff, SERVER_ROOMNUMCHANGE_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    for (auto client : room2players[roomId])
    {
        sendData((uv_stream_t*)client, s_send_buff, len);
    }
}

void SceneMgr::UpdateMonstList(RoomInfo* room, const MonstersSynMsg* syn)
{
    int size = syn->monsterlist_size();
    SynInfo* synInfo = room->synInfo;

    for (int i = 0; i < size; ++i)
    {
        MonsterSynMsg monster = syn->monsterlist(i);
        int id = monster.id();
        float x = monster.position().x();
        float y = monster.position().y();
        float z = monster.position().z();
        vec3 position(x, y, z);
        x = monster.rotation().x();
        y = monster.rotation().y();
        z = monster.rotation().z();
        vec3 rotation(x, y, z);

        synInfo->monsterList[id].position = position;
        synInfo->monsterList[id].rotation = rotation;
        /*if (monster.hp() == 0)
        {
            synInfo->monsterList[id].hp = 0;
        }*/

        if (distance(synInfo->monsterList[id].targetpos, synInfo->monsterList[id].position) <= BIAS)
        {
            int patrolSize = synInfo->patrolPoints.size();
            // 更新目标点
            int targetId = GenRandom(patrolSize - 1);
            synInfo->monsterList[id].targetpos = synInfo->patrolPoints[targetId];
        }
    }

    int roomId = room->roomId;
    g_logicMgr.dirtyRoom.insert(roomId);
}

void SceneMgr::GameStart(int roomId)
{
    RoomInfo* room = roomsInfo[roomId];
    if (!room->synInfo)
    {
        cout << "room not syn" << endl;
        return;
    }
    // 开始计时
    /*
    room->timer = new Timer(std::chrono::seconds(GAMETIME), boundFunction, roomId);
    room->timer->Start();*/
    RoomClear* clear = new RoomClear(roomId);
    room->clear = clear;
    room->gameTimerId = TimerMgr::GetInstance().AddTimer(GAMETIME, 1, clear, &RoomClear::GameOver, true);
    room->startTime = TimerMgr::GetInstance().GetServerTime();

    room->isStart = true;

    int timerId = TimerMgr::GetInstance().AddTimer(1, -1, &g_logicMgr, &LogicMgr::broad_statue, false);
    room->timerPool.push_back(timerId);
    timerId = TimerMgr::GetInstance().AddTimer(1, -1, &SceneMgr::GetInstance(), &SceneMgr::UpdateStatus, false);
    room->timerPool.push_back(timerId);
}

void SceneMgr::GameOver(int roomId)
{
    if (roomsInfo.find(roomId) == roomsInfo.end())
    {
        return;
    }
    RoomInfo* room = roomsInfo[roomId];
    int sceneId = room->sceneId;

    // score netid;
    multimap<int, int> rank;
    for (auto& info : room->synInfo->characterList)
    {
        if (info.second->time == 0)
        {
            info.second->time = GAMETIME;
        }
        int netId = info.first;

        int score = info.second->score + (GAMETIME - info.second->time) * 300;
        if (info.second->isDeath)
        {
            score = info.second->score;
        }
        info.second->score = score;

        rank.insert(make_pair(score, netId));
    }
    // 清除Timer
    //TimerMgr::GetInstance().RemoveTimer(room->timerId);
    timerPool.push_back(room->gameTimerId);
    clearPool.push_back(room->clear);
    for (auto index : room->timerPool)
    {
        TimerMgr::GetInstance().RemoveTimer(index);
    }

    // 给成员发结算信息
    GameResultRsp rsp;
    for (auto client : room2players[roomId])
    {
        string name = g_playerMgr.m_playerMap[client]->Name;
        string playerid = g_playerMgr.m_playerMap[client]->PlayerID;
        int netId = room->playerid2netId[playerid];
        auto info = room->synInfo->characterList[netId];
        if (!info)
        {
            return;
        }
        int score = info->score;

        RankList ranklist(name, score, info->time);
        rankLists[sceneId].insert(make_pair(score, ranklist));

        int roomRank = room->maxNum;
        for (auto it : rank)
        {
            if (it.second == netId)
            {
                rsp.set_roomrank(roomRank);
                break;
            }
            --roomRank;
        }

        rsp.set_score(score);
        rsp.set_gametime(info->time);
        rsp.set_success(info->isSuccess);
        rsp.set_beatcount(info->beatCount);

        int len = encode(s_send_buff, SERVER_END_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
        sendData((uv_stream_t*)client, s_send_buff, len);
    }

    // 清房
    delete room->rpcHandle;
    room->isStart = false;
    CloseRoom(roomId);
}

void SceneMgr::CloseRoom(int roomId)
{
    RoomInfo* info = roomsInfo[roomId];
    if (info == nullptr)
    {
        return;
    }
    int sceneId = info->sceneId;

    for (auto client : room2players[roomId])
    {
        player2room.erase(client);
    }
    room2players.erase(roomId);
    scene2roomList[sceneId].remove(info);
    roomsInfo.erase(roomId);

    /*if (info->timer)
    {
        timerPool.push_back(info->timer);
    }*/
    timerPool.push_back(info->gameTimerId);
    clearPool.push_back(info->clear);

    if (!info->synInfo->characterList.empty())
    {
        for (auto& character : info->synInfo->characterList)
        {
            CharacterStatue* state = character.second;
            delete state;
        }
    }
    delete info->synInfo;
    delete info;
}

void SceneMgr::SaveDisconnectPlayer(uv_tcp_t* client)
{
    string playerid = g_playerMgr.m_playerMap[client]->PlayerID;
    if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
    {
        return;
    }
    int roomId = player2room[client];

    disconnectPlayers.insert(make_pair(playerid, roomId));
    /*
    CharacterStatue* characterStatue = g_logicMgr.client2Character[client];
    if (characterStatue == nullptr)
    {
        return;
    }

    CharacterStatueMsg msg;
    msg.set_netid(characterStatue->netId);
    msg.set_hp(characterStatue->hp);
    msg.set_speed(characterStatue->speed);
    msg.set_speed(characterStatue->maxSpeed);
    msg.set_isdeath(characterStatue->isDeath);
    msg.set_isarmed(characterStatue->isArmed);
    msg.set_isimmunity(characterStatue->isImmunity);
    msg.set_actiontype(characterStatue->actionType);
    Vec3Msg* pos = msg.mutable_position();
    pos->set_x(characterStatue->position.x);
    pos->set_y(characterStatue->position.y);
    pos->set_z(characterStatue->position.z);
    Vec3Msg* rot = msg.mutable_rotation();
    rot->set_x(characterStatue->rotation.x);
    rot->set_y(characterStatue->rotation.y);
    rot->set_z(characterStatue->rotation.z);

    save(playerid.c_str(), msg.SerializeAsString().c_str(), "CharacterStatue", msg.ByteSize());*/

}

void SceneMgr::LoadDisconnectPlayer(uv_tcp_t* client, int roomId)
{
    string playerId = g_playerMgr.m_playerMap[client]->PlayerID;
    
    CharacterStatueMsg characterStatueMsg;
    RoomInfo* room = roomsInfo[roomId];
    if (room == nullptr)
    {
        return;
    }

    int netId = room->playerid2netId[playerId];
    CharacterStatue* characterStatue = room->synInfo->characterList[netId];

    characterStatueMsg.set_netid(characterStatue->netId);
    characterStatueMsg.set_hp(characterStatue->hp);
    characterStatueMsg.set_speed(characterStatue->speed);
    characterStatueMsg.set_maxspeed(characterStatue->maxSpeed);
    characterStatueMsg.set_isdeath(characterStatue->isDeath);
    characterStatueMsg.set_isarmed(characterStatue->isArmed);
    characterStatueMsg.set_isimmunity(characterStatue->isImmunity);
    characterStatueMsg.set_actiontype(characterStatue->actionType);
    Vec3Msg* pos = characterStatueMsg.mutable_position();
    pos->set_x(characterStatue->position.x);
    pos->set_y(characterStatue->position.y);
    pos->set_z(characterStatue->position.z);
    Vec3Msg* rot = characterStatueMsg.mutable_rotation();
    rot->set_x(characterStatue->rotation.x);
    rot->set_y(characterStatue->rotation.y);
    rot->set_z(characterStatue->rotation.z);

    int len = encode(s_send_buff, SERVER_CHARACTERADD_RSP, characterStatueMsg.SerializeAsString().c_str(), characterStatueMsg.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);

}

void SceneMgr::LoadNewPlayer(uv_tcp_t* client, int roomId)
{
    CharacterStatueMsg characterStatueMsg;

    if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
    {
        return;
    }
    RoomInfo* room = roomsInfo[roomId];
    if (room == nullptr)
    {
        return;
    }
    int netid = room->netIndex;
    string playerId = g_playerMgr.GetPlayerIDByClient(client);
    room->playerid2netId[playerId] = netid;
    ++room->netIndex;

    int len = readCfg(characterConfigPath, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", enemyConfigPath);
        return;
    }
    char* temp = (char*)malloc(len);
    readCfg(characterConfigPath, temp, len);
    const cJSON* character = NULL;
    const cJSON* originpositions = NULL;
    const cJSON* originposition = NULL;
    const cJSON* position = NULL;
    const cJSON* rotation = NULL;
    const cJSON* speed = NULL;
    const cJSON* hp = NULL;
    const cJSON* actiontype = NULL;
    int length = 0;
    int detailLength = 0;
    cJSON* monitor_json = cJSON_Parse(temp);
    if (monitor_json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        cJSON_Delete(monitor_json);
        free(temp);
        return;
    }
    CharacterStatue* characterStatue = new CharacterStatue();

    character = cJSON_GetObjectItemCaseSensitive(monitor_json, "character");
    originpositions = cJSON_GetObjectItemCaseSensitive(character, "originpositions");
    detailLength = cJSON_GetArraySize(originpositions);
    for (int i = 0; i < detailLength; ++i)
    {
        if (netid != i)
        {
            continue;
        }
        originposition = cJSON_GetArrayItem(originpositions, i);
        position = cJSON_GetObjectItemCaseSensitive(originposition, "position");
        rotation = cJSON_GetObjectItemCaseSensitive(originposition, "rotation");
        characterStatue->position = vec3(cJSON_GetArrayItem(position, 0)->valuedouble,
            cJSON_GetArrayItem(position, 1)->valuedouble, cJSON_GetArrayItem(position, 2)->valuedouble);
        characterStatue->rotation = vec3(cJSON_GetArrayItem(rotation, 0)->valuedouble,
            cJSON_GetArrayItem(rotation, 1)->valuedouble, cJSON_GetArrayItem(rotation, 2)->valuedouble);
    }
    speed = cJSON_GetObjectItemCaseSensitive(character, "speed");
    hp = cJSON_GetObjectItemCaseSensitive(character, "hp");
    actiontype = cJSON_GetObjectItemCaseSensitive(character, "actiontype");
    characterStatue->netId = netid;
    characterStatue->maxSpeed = speed->valuedouble;
    characterStatue->hp = hp->valueint;
    characterStatue->actionType = actiontype->valueint;

    characterStatueMsg.set_netid(characterStatue->netId);
    characterStatueMsg.set_hp(characterStatue->hp);
    characterStatueMsg.set_speed(0);
    characterStatueMsg.set_maxspeed(characterStatue->maxSpeed);
    characterStatueMsg.set_isdeath(characterStatue->isDeath);
    characterStatueMsg.set_isarmed(characterStatue->isArmed);
    characterStatueMsg.set_isimmunity(characterStatue->isImmunity);
    characterStatueMsg.set_actiontype(characterStatue->actionType);
    Vec3Msg* pos = characterStatueMsg.mutable_position();
    pos->set_x(characterStatue->position.x);
    pos->set_y(characterStatue->position.y);
    pos->set_z(characterStatue->position.z);
    Vec3Msg* rot = characterStatueMsg.mutable_rotation();
    rot->set_x(characterStatue->rotation.x);
    rot->set_y(characterStatue->rotation.y);
    rot->set_z(characterStatue->rotation.z);

    room->synInfo->characterList[netid] = characterStatue;

    len = encode(s_send_buff, SERVER_CHARACTERADD_RSP, characterStatueMsg.SerializeAsString().c_str(), characterStatueMsg.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::UpdateStatus()
{
    for (auto it : roomsInfo)
    {
        RoomInfo* room = it.second;
        if (!room)
        {
            continue;
        }
        int roomId = it.first;

        long gameStartTime = room->startTime;
        long currentTime = TimerMgr::GetInstance().GetServerTime();
        float deltaTime = TimerMgr::GetInstance().GetDeltaTime();

        if (!room->synInfo)
        {
            continue;
        }
        // 怪物重生
        for (auto& monster : room->synInfo->monsterList) 
        {

            if (monster.second.hp == 0 && monster.second.deathTime != 0)
            {
                // 复活
                if (currentTime - monster.second.deathTime >= monstersAttribute[GHOST].rebirthTime * 1000.0)
                {
                    monster.second.deathTime = 0;
                    monster.second.hp = monstersAttribute[GHOST].health;
                    monster.second.position = monster.second.oriPosition;
                    monster.second.targetpos = room->synInfo->patrolPoints[0];
                }
            }
            // 记录死亡时间
            if (monster.second.hp == 0 && monster.second.deathTime == 0)
            {
                monster.second.deathTime = currentTime;
            }
        }

        // 子弹
        for (auto& bullet : room->synInfo->bulletList)
        {
            float len = BULLETSPEED * deltaTime;
            bullet.second.position = vec3(bullet.second.position.x + bullet.second.rotation.x * len,
                bullet.second.position.y + bullet.second.rotation.y * len,
                bullet.second.position.z + bullet.second.rotation.z * len);
        }

        // 角色攻击cd
        for (auto& character : room->synInfo->characterList)
        {
            if (character.second->isAttack && character.second->attackTime != 0)
            {
                if (ATTACKCD <= currentTime - character.second->attackTime)
                {
                    character.second->isAttack = false;
                }
            }
        }
    }
}

SynInfo* SceneMgr::GetSceneInfo(uv_tcp_t* client)
{
    if (player2room.find(client) == player2room.end())
    {
        return nullptr;
    }

    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];
    if (!room)
    {
        return nullptr;
    }

    return room->synInfo;
}

int SceneMgr::GetCharacterNetid(uv_tcp_t* client)
{
    string playerId = g_playerMgr.GetPlayerIDByClient(client);
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];
    if (!room)
    {
        return -1;
    }

    return room->playerid2netId[playerId];
}

vec3 SceneMgr::GetFinishPos(int roomId)
{
    if (roomsInfo.find(roomId) == roomsInfo.end())
    {
        return vec3(0, 0, 0);
    }

    return roomsInfo[roomId]->endPos;
}

list<uv_tcp_t*> SceneMgr::GetClientInRoom(int roomId)
{
    if (room2players.find(roomId) == room2players.end())
    {
        return list<uv_tcp_t*>();
    }
    return room2players[roomId];
}

int SceneMgr::GetWeaponDamage(int weaponId)
{
    return 3;
}

MonsterAttribute* SceneMgr::GetMonsterAttribute(SceneObjectType type)
{
    if (type == GHOST || type == GARGOYLE)
    {
        return &monstersAttribute[type];
    }
    return nullptr;
}

int SceneMgr::GenRandom(int max)
{
    uniform_int_distribution<> distr(0, max);
    return distr(gen);
}

float SceneMgr::distance(vec3 a, vec3 b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;
    return sqrtf(x * x + y * y + z * z);
}

void SceneMgr::OnRecvSceneBriefReq(uv_tcp_t* client)
{
    int len = encode(s_send_buff, SERVER_SCENEBRIEF_RSP, sceneBriefRsp.SerializeAsString().c_str(), sceneBriefRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvEnemyConfigReq(uv_tcp_t* client)
{
    int len = encode(s_send_buff, SERVER_MONSTERATTRIBUTE_RSP, enemyConfigRsp.SerializeAsString().c_str(), enemyConfigRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvSceneInfoReq(uv_tcp_t* client)
{
    int roomId;
    if (player2room.find(client) == player2room.end())
    {
        //找断线
        string playerId = g_playerMgr.GetPlayerIDByClient(client);
        if (disconnectPlayers.find(playerId) != disconnectPlayers.end())
        {
            roomId = disconnectPlayers[playerId];
        }
        else 
        {
            return;
        }
    }
    else 
    {
        roomId = player2room[client];
    }

    RoomInfo* newSceneInfo = roomsInfo[roomId];
    if (newSceneInfo == nullptr) 
    {
        return;
    }
    int sceneId = newSceneInfo->sceneId;

    if (newSceneInfo->synInfo->patrolPoints.empty())
    {
        LoadSceneInfo(roomId);
    }

    SceneMonsterRsp monsterRsp;
    SceneObjectRsp objectRsp;
    SynInfo* syn = newSceneInfo->synInfo;

    if (syn == nullptr)
    {
        return;
    }
    
    uv_tcp_t* god = newSceneInfo->god;
    string playerId = g_playerMgr.GetPlayerIDByClient(client);
    int netId = newSceneInfo->playerid2netId[playerId];
    monsterRsp.set_netid(netId);

    for (auto& monster : syn->monsterList)
    {
        SceneObjectMsg* monsterData = monsterRsp.add_monsterlist();
        monsterData->set_type(monster.second.type);
        monsterData->set_id(monster.second.id);
        Vec3Msg* pos = monsterData->mutable_position();
        pos->set_x(monster.second.position.x);
        pos->set_y(monster.second.position.y);
        pos->set_z(monster.second.position.z);
        Vec3Msg* rot = monsterData->mutable_rotation();
        rot->set_x(monster.second.rotation.x);
        rot->set_y(monster.second.rotation.y);
        rot->set_z(monster.second.rotation.z);
    }

    for (auto& object : syn->boxList)
    {
        SceneObjectMsg* objectData = objectRsp.add_objectlist();
        objectData->set_type(BOX);
        objectData->set_id(object.second.id);
        Vec3Msg* pos = objectData->mutable_position();
        pos->set_x(object.second.position.x);
        pos->set_y(object.second.position.y);
        pos->set_z(object.second.position.z);
        Vec3Msg* rot = objectData->mutable_rotation();
        rot->set_x(object.second.rotation.x);
        rot->set_y(object.second.rotation.y);
        rot->set_z(object.second.rotation.z);
    }

    for (auto& object : syn->doorList)
    {
        SceneObjectMsg* objectData = objectRsp.add_objectlist();
        objectData->set_type(DOOR);
        objectData->set_id(object.second.id);
        Vec3Msg* pos = objectData->mutable_position();
        pos->set_x(object.second.position.x);
        pos->set_y(object.second.position.y);
        pos->set_z(object.second.position.z);
        Vec3Msg* rot = objectData->mutable_rotation();
        rot->set_x(object.second.rotation.x);
        rot->set_y(object.second.rotation.y);
        rot->set_z(object.second.rotation.z);
    }

    for (auto& object : syn->lightList)
    {
        SceneObjectMsg* objectData = objectRsp.add_objectlist();
        objectData->set_type(LIGHT);
        objectData->set_id(object.second.id);
        Vec3Msg* pos = objectData->mutable_position();
        pos->set_x(object.second.position.x);
        pos->set_y(object.second.position.y);
        pos->set_z(object.second.position.z);
        Vec3Msg* rot = objectData->mutable_rotation();
        rot->set_x(object.second.rotation.x);
        rot->set_y(object.second.rotation.y);
        rot->set_z(object.second.rotation.z);
    }

    for (auto& object : syn->slimeList)
    {
        SceneObjectMsg* objectData = objectRsp.add_objectlist();
        objectData->set_type(SLIME);
        objectData->set_id(object.second.id);
        Vec3Msg* pos = objectData->mutable_position();
        pos->set_x(object.second.position.x);
        pos->set_y(object.second.position.y);
        pos->set_z(object.second.position.z);
        Vec3Msg* rot = objectData->mutable_rotation();
        rot->set_x(object.second.rotation.x);
        rot->set_y(object.second.rotation.y);
        rot->set_z(object.second.rotation.z);
    }

    for (auto& object : syn->trapList)
    {
        SceneObjectMsg* objectData = objectRsp.add_objectlist();
        objectData->set_type(TRAP);
        objectData->set_id(object.second.id);
        Vec3Msg* pos = objectData->mutable_position();
        pos->set_x(object.second.position.x);
        pos->set_y(object.second.position.y);
        pos->set_z(object.second.position.z);
        Vec3Msg* rot = objectData->mutable_rotation();
        rot->set_x(object.second.rotation.x);
        rot->set_y(object.second.rotation.y);
        rot->set_z(object.second.rotation.z);
    }

    // 终点
    SceneObjectMsg* objectData = objectRsp.add_objectlist();
    objectData->set_type(FINISHLINE);
    Vec3Msg* pos = objectData->mutable_position();
    pos->set_x(newSceneInfo->endPos.x);
    pos->set_y(newSceneInfo->endPos.y);
    pos->set_z(newSceneInfo->endPos.z);

    int len = encode(s_send_buff, SERVER_SCENEMONSTER_RSP, monsterRsp.SerializeAsString().c_str(), monsterRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
    len = encode(s_send_buff, SERVER_SCENEOBJECT_RSP, objectRsp.SerializeAsString().c_str(), objectRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvRoomReq(uv_tcp_t* client, const RoomInfoReq* req)
{
    RoomInfoRsp rsp;
    int sceneId = req->sceneid();
    if (scene2roomList.count(sceneId)) {
        for (auto roomInfo : scene2roomList[sceneId]) 
        {
            RoomInfoMsg* room = rsp.add_roominfolist();
            room->set_roomid(roomInfo->roomId);
            room->set_roomname(roomInfo->roomName);
            room->set_maxnum(roomInfo->maxNum);
        }
    }

    int len = encode(s_send_buff, SERVER_ROOM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvRoomCreateReq(uv_tcp_t* client, const CreateRoomReq* req)
{
    EnterRoomRsp rsp;
    string roomName = req->roomname();
    int sceneId = req->sceneid();
    int maxNum = req->maxnum();
    RoomInfo* room = new RoomInfo(roomName, sceneId, maxNum, client, roomIndex);
    room->synInfo = new SynInfo();

    //player2room.insert(make_pair(client, roomIndex));
    player2room[client] = roomIndex;
    scene2roomList[sceneId].push_back(room);
    roomsInfo.insert(make_pair(roomIndex, room));
    list<uv_tcp_t*> temp;
    temp.push_back(client);
    room2players.insert(make_pair(roomIndex, temp));

    rsp.set_rescode(1);
    rsp.set_resstr("success");
    RoomInfoMsg* msg = rsp.mutable_roominfo();
    msg->set_maxnum(maxNum);
    msg->set_roomid(roomIndex);
    msg->set_roomname(roomName);
    ++roomIndex;

    int len = encode(s_send_buff, SERVER_ENTERROOM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);

    if (room->maxNum == room->currentNum)
    {
        SendRoomReady(room->roomId);
    }
}

void SceneMgr::OnRecvRoomEnterReq(uv_tcp_t* client, const EnterRoomReq* req)
{
    EnterRoomRsp rsp;
    int EnterRoomId = req->roomid();

    if (roomsInfo.count(EnterRoomId)) 
    {
        RoomInfo* room = roomsInfo[EnterRoomId];
        if (room->netIndex > room->maxNum) 
        {
            rsp.set_rescode(-2);
            rsp.set_resstr("Room Full");

            int len = encode(s_send_buff, SERVER_ENTERROOM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
            sendData((uv_stream_t*)client, s_send_buff, len);
        }
        else 
        {
            if (room->god == nullptr)
            {
                room->god = client;
            }
            room->currentNum += 1;
            player2room.insert(make_pair(client, EnterRoomId));
            room2players[EnterRoomId].push_back(client);

            rsp.set_rescode(1);
            rsp.set_resstr("Success");
            RoomInfoMsg* msg = rsp.mutable_roominfo();
            msg->set_roomname(room->roomName);
            msg->set_roomid(room->roomId);
            msg->set_maxnum(room->maxNum);

            int len = encode(s_send_buff, SERVER_ENTERROOM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
            sendData((uv_stream_t*)client, s_send_buff, len);

            RoomNumberChange(EnterRoomId);

            if (room->currentNum == room->maxNum)
            {
                SendRoomReady(room->roomId);
            }
        }
    }
    else 
    {
        rsp.set_rescode(-1);
        rsp.set_resstr("No Room");

        int len = encode(s_send_buff, SERVER_ENTERROOM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
        sendData((uv_stream_t*)client, s_send_buff, len);
    }


}

void SceneMgr::OnRecvRankListReq(uv_tcp_t* client, const RankListReq* req)
{
    RankListRsp rsp;
    int sceneId = req->sceneid();
    if (sceneId > LevelCount) 
    {
        return;
    }

    for (auto rank = rankLists[sceneId].rbegin(); rank != rankLists[sceneId].rend(); ++rank)
    {
        RankListMsg* msg = rsp.add_ranklist();
        msg->set_name((*rank).second.name);
        msg->set_score((*rank).first);
        msg->set_time((*rank).second.time);
    }

    int len = encode(s_send_buff, SERVER_RANKLIST_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvStartReq(uv_tcp_t* client)
{
    string playerId = g_playerMgr.m_playerMap[client]->PlayerID;
    if (disconnectPlayers.find(playerId) != disconnectPlayers.end())
    {
        int roomId = disconnectPlayers[playerId];
        // 房还在就继续
        if (roomsInfo.find(roomId) != roomsInfo.end())
        {
            RoomInfo* room = roomsInfo[roomId];
            if (room->isStart)
            {
                player2room[client] = roomId;
                room2players[roomId].push_back(client);
                disconnectPlayers.erase(playerId);

                int len = encode(s_send_buff, SERVER_START_RSP, nullptr, 0);
                sendData((uv_stream_t*)client, s_send_buff, len);
                return;
            }
        }
    }

    if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
    {
        return;
    }
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];
    room->readyCount += 1;


    if (room->readyCount != room->maxNum)
    {
        return;
    }

    room->rpcHandle = new RPCHandle(room->synInfo, roomId);

    int len = encode(s_send_buff, SERVER_START_RSP, nullptr, 0);
    for (auto target : room2players[roomId])
    {
        sendData((uv_stream_t*)target, s_send_buff, len);
    }
    GameStart(roomId);
}

void SceneMgr::OnRecvMonsterSynReq(uv_tcp_t* client, const MonstersSynMsg* syn)
{
    if (SceneMgr::GetInstance().player2room.find(client) == SceneMgr::GetInstance().player2room.end())
    {
        return;
    }
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];
    if (room == nullptr)
    {
        return;
    }

    if (client != room->god)
    {
        return;
    }

    UpdateMonstList(room, syn);

}

void SceneMgr::OnRecvGameTimeReq(uv_tcp_t* client)
{
    if (player2room.find(client) == player2room.end())
    {
        return;
    }
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];

    if (room == nullptr) 
    {
        return;
    }
    int time = TimerMgr::GetInstance().GetServerTime() - room->startTime;
    time /= 1000.0;

    time = GAMETIME - time;

    GameTimeRsp rsp;
    rsp.set_time(time);

    int len = encode(s_send_buff, SERVER_GAMETIME_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvGameScoreReq(uv_tcp_t* client)
{
    string playerId = g_playerMgr.GetPlayerIDByClient(client);
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];
    if (!room)
    {
        return;
    }
    int netId = room->playerid2netId[playerId];


    CharacterStatue* status = room->synInfo->characterList[netId];
    if (status == nullptr)
    {
        return;
    }
    int score = status->score;

    ScoreRsp rsp;
    rsp.set_score(score);

    int len = encode(s_send_buff, SERVER_GAMESCORE_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvGameContinueReq(uv_tcp_t* client)
{
    string playerId = g_playerMgr.m_playerMap[client]->PlayerID;
    bool needContinue = false;
    if (disconnectPlayers.find(playerId) != disconnectPlayers.end())
    {
        int roomId = disconnectPlayers[playerId];
        // 房还在就继续
        if (roomsInfo.find(roomId) != roomsInfo.end())
        {
            if (roomsInfo[roomId]->god == nullptr)
            {
                roomsInfo[roomId]->god = client;
            }
            needContinue = true;
        }
        else 
        {
            disconnectPlayers.erase(playerId);
        }
    }

    GameContinueRsp rsp;
    rsp.set_needcontinue(needContinue);

    int len = encode(s_send_buff, SERVER_GAMECONTINUE_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}

void SceneMgr::OnRecvAttackReq(uv_tcp_t* client, const AttackReq* req)
{
    if (player2room.find(client) == player2room.end())
    {
        return;
    }

    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];

    int netId = req->id();
    // 攻击cd
    if (room->synInfo->characterList[netId]->isAttack)
    {
        return;
    }

    BulletData bullet(bulletIndex, netId);
    bullet.position = vec3(req->position().x(), req->position().y(), req->position().z());
    bullet.rotation = vec3(req->rotation().x(), req->rotation().y(), req->rotation().z());
    room->synInfo->bulletList[bulletIndex++] = bullet;

    int len = encode(s_send_buff, SERVER_ATTACK_RSP, nullptr, 0);
    sendData((uv_stream_t*)client, s_send_buff, len);

    CharacterAnimSynMsg msg;
    msg.set_animttype(ATTACK);
    msg.set_netid(netId);
    len = encode(s_send_buff, SERVER_CHARACTERANIMSYN_RSP, msg.SerializeAsString().c_str(), msg.ByteSize());
    for (auto target : room2players[roomId])
    {
        if (target == client)
        {
            continue;
        }
        sendData((uv_stream_t*)target, s_send_buff, len);
    }

    g_logicMgr.dirtyRoom.insert(roomId);
}

void SceneMgr::OnRecvBulletsSynReq(uv_tcp_t* client, const BulletsSynMsg* req)
{
    if (player2room.find(client) == player2room.end())
    {
        return;
    }

    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];

    if (!room)
    {
        return;
    }

    if (client != room->god)
    {
        return;
    }

    SynInfo* syn = room->synInfo;
    if (!syn)
    {
        return;
    }

    for (int i = 0; i < req->bulletlist_size(); ++i)
    {
        BulletSynMsg msg = req->bulletlist(i);
        if (syn->bulletList.find(msg.id()) == syn->bulletList.end())
        {
            continue;
        }

        syn->bulletList[msg.id()].isActive &= msg.isactive();
    }

}

void SceneMgr::OnRecvRPCReq(uv_tcp_t* client, RPC* req)
{
    if (player2room.find(client) == player2room.end())
    {
        return;
    }
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];

    if (!room)
    {
        return;
    }

    room->rpcHandle->HandleRPCReq(client, req);
}

void SceneMgr::OnRecvCollisionReq(uv_tcp_t* client, CollisionReq* req)
{
    if (player2room.find(client) == player2room.end())
    {
        return;
    }
    int roomId = player2room[client];
    RoomInfo* room = roomsInfo[roomId];

    if (!room)
    {
        return;
    }
    if (!room->rpcHandle)
    {
        return;
    }
    room->rpcHandle->HandleCollisionCheckReq(client, req);
}

void SceneMgr::Quit(uv_tcp_t* client)
{
    if (!player2room.count(client))
    {
        return;
    }

    for (int i = 0; i<timerPool.size();++i)
    {
        TimerMgr::GetInstance().RemoveTimer(timerPool[i]);
    }
    timerPool.clear();
    /*for (int i = 0; i < clearPool.size(); ++i)
    {
        delete clearPool[i];
    }*/
    clearPool.clear();

    int roomId = player2room[client];
    RoomInfo* info = roomsInfo[roomId];
    if (!info)
    {
        return;
    }
    int sceneId = info->sceneId;

    player2room.erase(client);
    room2players[roomId].remove(client);

    info->currentNum -= 1;
    info->readyCount -= 1;
    if (info->currentNum > 0) 
    {
        if (client == info->god)
        {
            info->god = room2players[roomId].front();
        }
        if (!info->isStart)
        {
            RoomNumberChange(roomId);
            return;
        }
    }
    info->god = nullptr;

    // 游戏已经开始
    if (info->isStart)
    {
        return;
    }

    CloseRoom(roomId);
}

void RoomClear::GameOver()
{
    SceneMgr::GetInstance().GameOver(roomId);
}
