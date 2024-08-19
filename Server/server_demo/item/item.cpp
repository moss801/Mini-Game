#include "item.h"
#pragma once
#include "item.h"
#include <filedb/filedb.h>
#include "../../cJSON/cJSON.h"
#include "../../codec/codec.h"
#include "../../utils/utils.h"
#include "player/player.h"
static char s_send_buff[1024 * 64];

ItemMgr::ItemMgr() {
    
}
ItemMgr::~ItemMgr() {

}
void ItemMgr::init(){
    InitItemConfigs();
    InitPublicShop();
}
void ItemMgr::bag_get(uv_tcp_t* client, PlayerMgr* playerMgr) {
    if (m_playerBag.find(client) == m_playerBag.end()) {
        if (!InitBag(client, playerMgr)) {
            return;
        }
    }
    int len = encode(s_send_buff, SERVER_GETBAGITEMS_RSP, m_playerBag[client].SerializeAsString().c_str(), m_playerBag[client].ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}
void ItemMgr::shop_get(uv_tcp_t* client) {
    if (m_pirvateShop.find(client) == m_pirvateShop.end()) {
        return;
    }
    for (int i = 0; i < m_publicShopItemRsp.item_size(); i++) {
        if (m_publicShopItemRsp.item().Get(i).num() != -1) {
            ItemInfo itemInfo = m_pirvateShop[client].item().Get(i);
            itemInfo.set_num(m_publicShopItemRsp.item().Get(i).num());
        }
    }
    int len = encode(s_send_buff, SERVER_GETSHOPITEMS_RSP, m_publicShopItemRsp.SerializeAsString().c_str(), m_publicShopItemRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}
void ItemMgr::config_get(uv_tcp_t* client) {
    int len = encode(s_send_buff, SERVER_ITEMCONFIG_RSP, m_ItemConfigsRsp.SerializeAsString().c_str(), m_ItemConfigsRsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}
void ItemMgr::remove_Item(uv_tcp_t* client, DeleteItemReq* req, PlayerMgr* playerMgr) {
    StatueCodeRsp rsp;
    if (m_playerBag.find(client) == m_playerBag.end()) {
        rsp.set_code(STATUE_NOTEXIST);
        rsp.set_reason("Bag not exist");
    }
    bool isRemove = false;
    for (int i = 0; i < m_playerBag[client].item().size(); i++) {
        ItemInfo* itemInfo = m_playerBag[client].mutable_item(i);
        if (itemInfo->id() == req->id()) {
            if (itemInfo->num() >= req->num()) {
                itemInfo->set_num(itemInfo->num() - req->num());
                rsp.set_code(STATUE_SUCCESS);
                rsp.set_reason("Remove Success");
                isRemove = true;
            }
            break;
        }
    }
    if(!isRemove) {
        rsp.set_code(STATUE_DELETE_ITEMNOTENOUGH);
        rsp.set_reason("Item not enough");
    }
    int len = encode(s_send_buff, SERVER_DELETEITEM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
    save(playerMgr->GetPlayerIDByClient(client).c_str(), m_playerBag[client].SerializeAsString().c_str(), "Bag", m_playerBag[client].ByteSize());
}
void ItemMgr::money_get(uv_tcp_t* client) {
    MoneyRsp rsp;
    if (m_playerBag.find(client) == m_playerBag.end()) {
        rsp.set_code(STATUE_NOTEXIST);
    }
    else {
        rsp.set_code(STATUE_SUCCESS);
        rsp.set_num(m_playerBag[client].money());
    }
    int len = encode(s_send_buff, SERVER_GETMONEY_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}
void ItemMgr::money_add(uv_tcp_t* client, AddMoneyReq* req,PlayerMgr* playerMgr) {
    MoneyRsp rsp;
    if (m_playerBag.find(client) == m_playerBag.end()) {
        rsp.set_code(STATUE_NOTEXIST);
    }
    else if(m_playerBag[client].money() + req->num()<0) {
        rsp.set_code(STATUE_MONEY_NOTENOUGH);
        rsp.set_num(m_playerBag[client].money());
    }
    else {
        rsp.set_code(STATUE_SUCCESS);
        m_playerBag[client].set_money(m_playerBag[client].money() + req->num());
        rsp.set_num(m_playerBag[client].money());
    }
    save(playerMgr->GetPlayerIDByClient(client).c_str(), m_playerBag[client].SerializeAsString().c_str(), "Bag", m_playerBag[client].ByteSize());
    int len = encode(s_send_buff, SERVER_ADDMONEY_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}
void ItemMgr::buy(uv_tcp_t* client, BuyReq* req, PlayerMgr* playerMgr) {
    StatueCodeRsp rsp;
    if (m_playerBag.find(client) == m_playerBag.end()) {
        return;
    }
    int id = req->id();
    if (req->id() >= m_itemConfigs.size()) {
        rsp.set_code(STATUE_ITEM_ITEMNOTEXIST);
        rsp.set_reason("Item not exist");
    }
    else if (m_itemConfigs[req->id()].price * req->num() > m_playerBag[client].money()) {
        rsp.set_code(STATUE_MONEY_NOTENOUGH);
        rsp.set_reason("money is not enough");
    }
    else if (m_publicShopItemRsp.item().Get(id).num() != -1) {
        if (m_publicShopItemRsp.item().Get(id).num() < req->num()) {
            rsp.set_code(STATUE_BUY_ITEMNOTENOUGH);
            rsp.set_reason("item is not enough");
        }
        else {
            ItemInfo* itemInfo = m_publicShopItemRsp.mutable_item(id);
            itemInfo->set_num(itemInfo->num() - req->num());
            rsp.set_code(STATUE_SUCCESS);
            rsp.set_reason("buy successful");
            save("Master", m_publicShopItemRsp.SerializeAsString().c_str(), "Shop", m_publicShopItemRsp.ByteSize());
        }
    }
    else if(m_pirvateShop[client].item().Get(req->id()).num() != -1 ){
        if (m_pirvateShop[client].item().Get(req->id()).num() < req->num()) {
            rsp.set_code(STATUE_BUY_ITEMNOTENOUGH);
            rsp.set_reason("item is not enough");
        }
        else {
            ItemInfo* itemInfo = m_pirvateShop[client].mutable_item(id);
            itemInfo->set_num(itemInfo->num() - req->num());
            rsp.set_code(STATUE_SUCCESS);
            rsp.set_reason("buy successful");
            save(playerMgr->GetPlayerIDByClient(client).c_str(), m_pirvateShop[client].SerializeAsString().c_str(), "Shop", m_pirvateShop[client].ByteSize());
        }
    }
    else {
        rsp.set_code(STATUE_SUCCESS);
        rsp.set_reason("buy successful");
    }
    if (rsp.code() == STATUE_SUCCESS) {
        m_playerBag[client].set_money(m_playerBag[client].money() - m_itemConfigs[req->id()].price * req->num());
        bool isAdd = false;
        for (int i = 0; i < m_playerBag[client].item().size(); i++) {
            ItemInfo* itemInfo = m_playerBag[client].mutable_item(i);
            if (itemInfo->id() == id) {
                itemInfo->set_num(itemInfo->num() + req->num());
                isAdd = true;
                break;
            }
        }
        if (!isAdd) {
            ItemInfo* itemInfo = m_playerBag[client].add_item();
            itemInfo->set_id(id);
            itemInfo->set_num(req->num());
        }
        save(playerMgr->GetPlayerIDByClient(client).c_str(), m_playerBag[client].SerializeAsString().c_str(), "Bag", m_playerBag[client].ByteSize());
    }
    int len = encode(s_send_buff, SERVER_BUYITEM_RSP, rsp.SerializeAsString().c_str(), rsp.ByteSize());
    sendData((uv_stream_t*)client, s_send_buff, len);
}
bool ItemMgr::InitBag(uv_tcp_t* client,PlayerMgr* playerMgr) {
    string playerId = playerMgr->GetPlayerIDByClient(client);
    if (playerId == "") {
        return false;
    }
    BagItemRsp bagItemRsp;
    char data[DEFAULTSIZE];
    int result = load(playerId.c_str(), data, "Bag", DEFAULTSIZE);
    if (result > 0) {
        bagItemRsp.ParseFromArray(data, result);
    }
    else {
        bagItemRsp.set_money(DEFAULTMONEY);
    }
    m_playerBag[client] = bagItemRsp;

    ShopItemRsp shopItemRsp;
    result = load(playerId.c_str(), data, "Shop", DEFAULTSIZE);
    if (result > 0) {
        shopItemRsp.ParseFromArray(data, result);
    }
    else {
        for (int i = 0; i < m_itemConfigs.size(); i++) {
            ItemInfo* itemInfo = shopItemRsp.add_item();
            itemInfo->set_id(i);
            itemInfo->set_num(m_publicShopItemRsp.item().Get(i).num());
        }
    }
    m_pirvateShop[client] = shopItemRsp;
    return true;
}
void ItemMgr::_parseCfg(const char* const monitor){
    const cJSON* resolution = NULL;
    const cJSON* resolutions = NULL;
    const cJSON* items = NULL;
    const cJSON* item = NULL;
    const cJSON* name = NULL;
    const cJSON* introduce = NULL;
    const cJSON* price = NULL;
    const cJSON* iconName = NULL;
    int length = 0;
    int status = 0;
    cJSON* monitor_json = cJSON_Parse(monitor);
    if (monitor_json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = 0;
        goto end;
    }

    items = cJSON_GetObjectItemCaseSensitive(monitor_json, "items");
    if (!cJSON_IsArray(items)) {
        fprintf(stderr, "invalid config, items field must array\n");
        goto end;
    }
    length = cJSON_GetArraySize(items);
    for (int i = 0; i < length; i++) {
        item = cJSON_GetArrayItem(items, i);
        name = cJSON_GetObjectItemCaseSensitive(item, "name");
        introduce = cJSON_GetObjectItemCaseSensitive(item, "introduce");
        price = cJSON_GetObjectItemCaseSensitive(item, "price");
        iconName = cJSON_GetObjectItemCaseSensitive(item, "iconName");
        m_itemConfigs.emplace_back(name->valuestring, introduce->valuestring, price->valueint, iconName->valuestring);
        ItemConfig* itemConfig = m_ItemConfigsRsp.add_item();
        itemConfig->set_id(i);
        itemConfig->set_name(name->valuestring);
        itemConfig->set_introduce(introduce->valuestring);
        itemConfig->set_price(price->valueint);
        itemConfig->set_iconname(iconName->valuestring);
    }

end:
    cJSON_Delete(monitor_json);
    return;
}
void ItemMgr::InitItemConfigs() {
	m_itemConfigs.clear();
    const char* path = "config/ItemConfig.json";
    char* temp = nullptr;
    int len = 0;
    len = readCfg(path, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", path);
        return;
    }
    temp = (char*)malloc(len);
    readCfg(path, temp, len);

    _parseCfg(temp);

    free(temp);
}
void ItemMgr::InitPublicShop() {
    char data[DEFAULTSIZE];
    int result = load("Master", data, "Shop", DEFAULTSIZE);
    if (result>0) {
        m_publicShopItemRsp.ParseFromArray(data, result);
    }
    else {
        for (int i = 0; i < m_itemConfigs.size(); i++) {
            ItemInfo* itemInfo = m_publicShopItemRsp.add_item();
            itemInfo->set_id(i);
            itemInfo->set_num(DEFAULTITEMNUM);
        }
        m_publicShopItemRsp.SerializeToArray(data, m_publicShopItemRsp.ByteSize());
        save("Master", data, "Shop", m_publicShopItemRsp.ByteSize());
    }
}

ItemMgr& ItemMgr::GetInst()
{
    static ItemMgr inst;
    return inst;
}
