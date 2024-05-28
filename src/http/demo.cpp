#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<cstring>
#include"cache.h"
#include"shareheader.h"
#include"tokenutil.h"
#include"httpcenter.h"
#include"msgcenter.h"
#include"isockmsg.h"
#include"httputil.h"
#include"demo.h"


TestDemo::TestDemo(HttpCenter* center)
    : m_center(center) {
}

int TestDemo::procFrameHd(int hd, HttpCtx*, NodeMsg* msg) {
    HttpFrameHd* body = NULL;
    NodeMsg* rsp = NULL;
    int ret = 0;
    HttpCache* pp[1] = {NULL};
    HttpCache cache;

    HttpUtil::init(&cache);
    body = MsgCenter::getBody<HttpFrameHd>(msg); 
    
    HttpUtil::addRspParam(&cache,
        body->m_http_ver, RTSP_STATUS_OK);

    HttpUtil::addEnumField(&cache, ENUM_HTTP_CONNECTION, "Keep-Alive");
    
    pp[0] = &cache;
    rsp = HttpUtil::genHttpCache(pp, 1,
        body->m_body, body->m_body_size);

    ret = m_center->send(hd, rsp);

    HttpUtil::release(&cache);
    return ret;
}

int TestDemo::procFrameMid(int, HttpCtx*, NodeMsg* msg) {
    HttpFrameMid* body = NULL;
    int ret = 0;

    body = MsgCenter::getBody<HttpFrameMid>(msg);
    (void)body;
    
    return ret;
}

HttpCtx* TestDemo::allocHttpEnv() {
    HttpCtx* env = NULL;
    
    env = (HttpCtx*)CacheUtil::mallocAlign(sizeof(HttpCtx));
    return env;
}

void TestDemo::freeHttpCtx(HttpCtx* ctx) {
    if (NULL != ctx) {
        CacheUtil::freeAlign(ctx);
    }
} 

bool TestDemo::bindCtx(int, HttpCtx*) {
    return true;
}

void TestDemo::writeBytes(unsigned seq, const char* psz, 
    int size, bool truncated) {
    int fd = 0;
    int flag = O_WRONLY | O_CREAT;
    int len = 0;
    char name[32] = {0};

    if (!(0 < size)) {
        return;
    }

    TokenUtil::toStr(name, 32, seq);

    if (truncated) {
        flag |= O_TRUNC;
    } else {
        flag |= O_APPEND;
    }
    
    fd = open(name, flag, 0644);
    if (0 >= fd) return; 
    
    len = write(fd, psz, size);
    close(fd);

    if (len != size) {
        LOG_INFO("write_body| size=%d| len=%d|",
            size, len);
    }
}

void TestDemo::writeBody(unsigned seq, Cache* cache, 
    int size, bool truncated) {
    char* psz = NULL;

    if (!(0 < size)) {
        return;
    }

    psz = CacheUtil::data(cache);
    writeBytes(seq, psz, size, truncated);
}

