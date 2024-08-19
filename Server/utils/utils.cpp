#include "utils.h"
#include "../netpack/nethandle.h"
#include "../3rd/src/cJSON/cJSON.h"
#include "filedb/filedb.h"

extern uv_loop_t* g_loop;
extern ServerCfg g_config;

void alloc_read_buff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = (int)suggested_size;
}

void free_read_buff(const uv_buf_t* buf) {
    if (buf->base) {
        free(buf->base);
    }
}

my_write_t* alloc_write_buff(size_t size) {
    my_write_t* w = (my_write_t*)malloc(sizeof(my_write_t));
    if (w == nullptr)
        return nullptr;

    w->buf.base = (char*)malloc(size);
    if (w->buf.base == nullptr) {
        free(w);
        return nullptr;
    }
    w->buf.len = (int)size;

    return w;
}

void free_write_buff(my_write_t* w) {
    if (w->buf.base != nullptr) {
        free(w->buf.base);
    }
    free(w);
}

PeerData* malloc_peer_data() {
    PeerData* peerData = (PeerData*)malloc(sizeof(PeerData));
    if (!peerData)
        return nullptr;

    peerData->nowPos = 0;
    
    return peerData;
}

void free_peer_data(PeerData* peerData) {
    free(peerData);
}

void on_close(uv_handle_t *handle) {
    fprintf(stdout, "client closed:%llu\n", (uint64_t)handle);

    // 通知逻辑层连接关闭
    OnCloseClient((uv_tcp_t*)handle);

    if (handle != NULL) {
        free(handle);
    }
}

void on_write(uv_write_t *req, int status) {
    if (status<0) {
        fprintf(stderr, "Write error %s, req:%llu\n", uv_strerror(status), (uint64_t)req);
        goto Exit0;
    }
//    fprintf(stdout, "on write, send data to client:%llu success, len:%d\n", (uint64_t)req, ((my_write_t*)req)->buf.len);
Exit0:
    free_write_buff((my_write_t*)req);
}

bool sendData(uv_stream_t* stream, const char* data, int len) {
    fprintf(stdout, "%d\n", len);
    bool result = false;
    my_write_t* req = nullptr;
    int res = 0;
    req = alloc_write_buff(len);
    if (req == nullptr) {
        goto Exit0;
    }

    memcpy(req->buf.base, data, len);

    //发送buffer数组，第四个参数表示数组大小
    res = uv_write(req, stream, &(req->buf), 1, on_write);
    if ( res != 0) {
        goto Exit0;
    }
    req = nullptr;

    result = true;
Exit0:
    if (req) {
        free_write_buff(req);
        req = nullptr;
    }

    if (!result &&uv_is_writable(stream)) {
        uv_close((uv_handle_t*)stream, on_close);
    }

    return result;
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name((int)nread));
        }
        else {
            fprintf(stderr, "client disconnect\n");
        }
        uv_close((uv_handle_t*)stream, on_close);
        goto Exit0;
    }
    // 调用逻辑层消息处理
    if (!OnRecv((uv_tcp_t*)stream, buf->base, (int)nread)) {
        uv_close((uv_handle_t*)stream, on_close);
        goto Exit0;
    }
Exit0:
    free_read_buff(buf);
}

void on_new_connection(uv_stream_t *server, int status) {
    uv_tcp_t *client = nullptr;
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        goto Exit0;
    }
    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    uv_tcp_init(g_loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        fprintf(stdout, "accept client: %llu\n", (uint64_t)client);

        // 通知逻辑层新连接建立
        if (!OnNewClient(client)) {
            goto Exit0;
        }

        uv_read_start((uv_stream_t*)client, alloc_read_buff, on_read);
        client = nullptr;
    }
    else {
        fprintf(stderr, "accept failed\n");
    }
Exit0:
    if (client) {
        uv_close((uv_handle_t*)client, NULL);
        free(client);
        client = nullptr;
    }
    return;
}

int _parseCfg(const char * const monitor) {
    const cJSON *resolution = NULL;
    const cJSON *resolutions = NULL;
    const cJSON *temp = NULL;
    const cJSON *ip = NULL;
    const cJSON *port = NULL;

    int status = 0;
    cJSON *monitor_json = cJSON_Parse(monitor);
    if (monitor_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = 0;
        goto end;
    }

    temp = cJSON_GetObjectItemCaseSensitive(monitor_json, "gameserver");
    if (!cJSON_IsObject(temp)) {
        fprintf(stderr, "invalid config, gameserver field must object\n");
        goto end;
    }

    ip = cJSON_GetObjectItemCaseSensitive(temp, "ip");
    if (!cJSON_IsString(ip)) {
        fprintf(stderr, "invalid gameserver config, ip field is not string\n");
        goto end;
    }
    strcpy(g_config.game_server_ip, ip->valuestring);

    port = cJSON_GetObjectItemCaseSensitive(temp, "port");
    if (!cJSON_IsNumber(port)) {
        fprintf(stderr, "invalid gameserver config, port field is not number\n");
        goto end;
    }
    g_config.game_server_port = port->valueint;

    temp = cJSON_GetObjectItemCaseSensitive(monitor_json, "announce");
    if (cJSON_IsString(temp))
        g_config.default_announce = temp->valuestring;
end:
    cJSON_Delete(monitor_json);
    return status;
}

int loadConfig() {
    const char* path = "config/config.json";
    char* temp = nullptr;
    int len = 0;
    len = readCfg(path, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", path);
        return -1;
    }
    temp = (char*)malloc(len);
    readCfg(path, temp, len);

    _parseCfg(temp);

    free(temp);

    return 0;
}

int init_socket_server(const char* ip, uint16_t port, uv_loop_t* loop) {
    int result = 0;
    sockaddr_in addr;
    // todo 没地方释放了
    uv_tcp_t* server = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    result = uv_tcp_init(loop, server);
    if (result != 0) {
        fprintf(stderr, "uv_tcp_init error %s\n", uv_strerror(result));
        goto Exit0;
    }

    result = uv_ip4_addr(ip, port, &addr);
    if (result != 0) {
        fprintf(stderr, "uv_ip4_addr error %s\n", uv_strerror(result));
        goto Exit0;
    }

    result = uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
    if (result != 0) {
        fprintf(stderr, "uv_tcp_bind error %s\n", uv_strerror(result));
        goto Exit0;
    }

    result = uv_listen((uv_stream_t*)server, SOMAXCONN, on_new_connection);
    if (result != 0) {
        fprintf(stderr, "uv_listen error %s\n", uv_strerror(result));
        goto Exit0;
    }
    result = 0;
Exit0:
    return result;
}



