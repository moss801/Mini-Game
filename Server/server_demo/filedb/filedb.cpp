#pragma once
#include "filedb.h"
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#define SAVE_PATH "./data"

int save(const char* playerID, const char* data, const char* type, int len) {
    int result = -1;
    FILE * fp = nullptr;
    char path[512] = {0};
    char rootpath[512] = { 0 };
    sprintf(rootpath, "%s/%s", SAVE_PATH, playerID);
    if (0 != _access(rootpath,0) && (_mkdir(rootpath))) {
        result = -1;
        goto Exit0;
    }
    sprintf(path, "%s/%s/%s", SAVE_PATH, playerID, type);
    fp = fopen(path, "wb");
    if (fp == nullptr) {
        result = -1;
        goto Exit0;
    }
    result = (int)fwrite(data, 1, len, fp);
    if (result != len) {
        result = -2;
        goto Exit0;
    }
    result = 0;
Exit0:
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
    return result;
}


int load(const char* playerID, char* data, const char* type, int size) {
    int result = -1;
    FILE * fp = nullptr;
    char path[512] = {0};
    int fileSize = 0;

    sprintf(path, "%s/%s/%s", SAVE_PATH, playerID, type);
    fp = fopen(path, "rb");
    if (fp == nullptr) {
        result = -1;
        goto Exit0;
    }
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    if (size < fileSize) {
        result = -2;
        goto Exit0;
    }
    rewind(fp);

    result = (int)fread(data, 1, fileSize, fp);
    if (result != fileSize) {
        result = -3;
        goto Exit0;
    }

Exit0:
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
    return result;
}

// ¶ÁÈ¡ÅäÖÃÎÄ¼þ
int readCfg(const char* path, char* data, int size) {
    int result = -1;
    FILE * fp = nullptr;
    int fileSize = 0;

    fp = fopen(path, "rb");
    if (fp == nullptr) {
        result = -1;
        goto Exit0;
    }
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    if (data == nullptr) {
        result = fileSize;
        goto Exit0;
    }

    if (size < fileSize) {
        result = -2;
        goto Exit0;
    }
    rewind(fp);

    result = (int)fread(data, 1, fileSize, fp);
    if (result != fileSize) {
        result = -3;
        goto Exit0;
    }

Exit0:
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
    return result;
}
