#include "../utils/utils.h"
#include "../netpack/nethandle.h"
#include "../utils/baseData.h"
#include "logic/logicMgr.h"
#include "player/player.h"
#include "scene/rpcHandle.h"

#ifdef WIN32

#else
#include <unistd.h>
#endif

//////////////////////////////////////////////////////////////////////////
using namespace std;

uv_loop_t* g_loop = nullptr;

ServerCfg g_config;
PlayerMgr g_playerMgr;
LogicMgr g_logicMgr;


void my_sleep(int t) {
#ifdef WIN32
    Sleep(t);
#else
    sleep(t);
#endif
}

bool init() {
    if (loadConfig() != 0) {
        fprintf(stderr, "load config failed\n");
        return false;
    }
    return true;
}

void main_loop(uv_idle_t* handle) {
    // todo game logic update
    TimerMgr::GetInstance().Tick();
    my_sleep(100);
}

int main(int argc, char* argv[]) {
    g_loop = uv_default_loop();

    if (!init()) {
        return 1;
    }

    if (init_socket_server(g_config.game_server_ip, g_config.game_server_port, g_loop) != 0) {
        fprintf(stderr, "init game server failed\n");
        return 2;
    }
    fprintf(stdout, "start game server, ip:%s, port:%d\n", g_config.game_server_ip, g_config.game_server_port);
 
    uv_idle_t idler;
    uv_idle_init(g_loop, &idler);
    uv_idle_start(&idler, main_loop);

    g_playerMgr.init();
    ItemMgr::GetInst().init();

    uv_run(g_loop, UV_RUN_DEFAULT);

    printf("main loop stop\n");
    return 0;
}
