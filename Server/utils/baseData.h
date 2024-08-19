#pragma once
#include <random>
#include <stdio.h>
#include <stack>
#include <string>
#include "timer/timer.h"
#include <map>
#include <vector>
#include <uv.h>
#include "../proto/player.pb.h"
#include "timer/timerMgr.h"

class RPCHandle;
class RoomClear;

using namespace TCCamp;
using namespace std;

const int DEFAULTHP = 100;
const float MAXSPEED = 1.0;

typedef struct vec3 {
	float x;
	float y;
	float z;
	vec3() :x(0), y(0), z(0) {}
	vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
}vec3;

typedef struct CharacterStatue {
	int netId;
	
	int hp;
	vec3 position;
	vec3 rotation;
	float speed;
	float maxSpeed;
	bool markDirty;


	int score;
	int time;
	int beatCount;
	bool isDeath;
	bool isArmed;
	bool isImmunity;
	bool isSuccess;
	int actionType;
	bool isDisconnect;
	bool isAttack;
	long attackTime;
	//int roomId;
	CharacterStatue() :
		netId(-1),
		hp(DEFAULTHP),
		position(vec3()),
		rotation(vec3()),
		speed(0),
		maxSpeed(0),
		markDirty(false),
		score(0),
		time(0),
		beatCount(0),
		isDeath(false),
		isArmed(false),
		isImmunity(false),
		isSuccess(false),
		actionType(0),
		isDisconnect(false),
		isAttack(false),
		attackTime(0)
		//roomId(0)
	{}
}CharacterStatue;

struct BulletData {
	int id;
	int netId;
	bool isActive;
	vec3 position;
	vec3 rotation;

	BulletData() : id(0), netId(0), isActive(true) {}
	BulletData(int _id, int _netId) : id(_id), netId(_netId), isActive(true) {}
};

struct ObjectData {
	int id;
	bool isActive;
	vec3 position;
	vec3 rotation;

	ObjectData() : id(0), isActive(false) {}
	ObjectData(int _id) : id(_id), isActive(false) {}
};

struct MonsterData {
	int id;
	int hp;
	SceneObjectType type;
	vec3 position;
	vec3 oriPosition;
	vec3 rotation;
	vec3 targetpos;
	long deathTime;

	MonsterData(int _id, int _hp) :id(_id), hp(_hp), type(OBJECT_NONE), deathTime(0) {}
	MonsterData() : id(0), hp(0), type(OBJECT_NONE), deathTime(0) {}
};

struct RankList
{
	string name;
	int score;
	int time;
	RankList(string _name, int _score, int _time):name(_name), score(_score), time(_time) {}
};


struct SynInfo
{
	vector<vec3> patrolPoints;
	map<int, MonsterData> monsterList;
	map<int, ObjectData> doorList;
	map<int, ObjectData> lightList;
	map<int, ObjectData> slimeList;
	map<int, ObjectData> trapList;
	map<int, ObjectData> boxList;
	//bulletid
	map<int, BulletData> bulletList;
	//netid
	map<int, CharacterStatue*> characterList;
};

struct MonsterAttribute {
	int damage;
	int health;
	float speed;
	float rebirthTime;
	int killPoint;
	float detectRange;
};

struct RoomInfo
{
	int roomId;
	int sceneId;
	string roomName;
	int maxNum;
	int currentNum;
	uv_tcp_t* god;
	int readyCount;
	int netIndex;
	bool isStart;
	vec3 endPos;

	//Timer* timer;
	vector<int> timerPool;
	int gameTimerId;
	long startTime;
	SynInfo* synInfo;
	RPCHandle* rpcHandle;
	RoomClear* clear;
	bool isEnd;
	unordered_map<string, int> playerid2netId;

	RoomInfo(string _roomName, int _sceneId, int _maxNum, uv_tcp_t* _god, int _roomId) :
		roomName(_roomName),
		sceneId(_sceneId),
		maxNum(_maxNum),
		god(_god),
		netIndex(0),
		roomId(_roomId),
		currentNum(1),
		synInfo(nullptr),
		readyCount(0),
		isStart(false),
		isEnd(false),
		//timer(nullptr),
		rpcHandle(nullptr),
		gameTimerId(-1),
		startTime(0),
		clear(nullptr)
	{}
};