#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include "uv.h"
#include "../proto/player.pb.h"

class PlayerMgr;

using namespace std;
using namespace TCCamp;
const int DEFAULTITEMNUM = 100;
const int DEFAULTMONEY = 10000;
const int DEFAULTSIZE = 1024;

struct Item {
	string name;
	string introduce;
	int price;
	string iconName;
	Item(string initName, string initIntroduce, int initPrice, string initIconName) :
		name(initName),
		introduce(initIntroduce),
		price(initPrice),
		iconName(initIconName) {}
};

class ItemMgr {
public:
	static ItemMgr& GetInst();
	
	void init();

	void shop_get(uv_tcp_t* client);

	void bag_get(uv_tcp_t* client,PlayerMgr* playerMgr);

	void config_get(uv_tcp_t* client);

	void money_get(uv_tcp_t* client);

	void money_add(uv_tcp_t* client, AddMoneyReq* req, PlayerMgr* playerMgr);

	void remove_Item(uv_tcp_t* client, DeleteItemReq* req, PlayerMgr* playerMgr);

	void buy(uv_tcp_t* client, BuyReq* req, PlayerMgr* playerMgr);

	bool InitBag(uv_tcp_t* client, PlayerMgr* playerMgr);


private:
	ItemMgr();
	~ItemMgr();

	void InitItemConfigs();

	void InitPublicShop();

	void _parseCfg(const char* const monitor);

	vector<Item> m_itemConfigs;
	map<uv_tcp_t*, BagItemRsp> m_playerBag;
	map<uv_tcp_t*, ShopItemRsp> m_pirvateShop;

	ShopItemRsp m_publicShopItemRsp;
	ItemConfigs m_ItemConfigsRsp;
};