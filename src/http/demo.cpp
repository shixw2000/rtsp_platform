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
#include"sockframe.h"


TestDemo::TestDemo(SockFrame* frame, HttpCenter* center)
    : m_frame(frame), m_center(center) {
}

bool TestDemo::procFrameHd(int hd, NodeMsg* msg) {
    HttpFrameHd* body = NULL;
    bool isAlive = false;

    body = MsgCenter::getBody<HttpFrameHd>(msg);
    isAlive = chkAlive(body);
    
    if (0 == body->m_frame_beg) { 
        rspHttpFrame(hd, body->m_http_ver, isAlive,
            body->m_body, body->m_body_size);
    }

    if (body->m_is_end && !isAlive) {
        m_frame->closeData(hd);
    }
    
    return true;
}

bool TestDemo::chkAlive(const HttpFrameHd* body) {
    bool bOk = false;
    bool isAlive = false;
    Token key;
    
    switch (body->m_http_ver) {
    case HTTP_VER_10_VAL:
        isAlive = false;
        break;
        
    case HTTP_VER_11_VAL:
    default:
        isAlive = true;
        break;
    }
    
    bOk = m_center->findEnumField(body, 
        ENUM_HTTP_CONNECTION, &key);
    if (bOk) {
        if (TokenUtil::strCmp(&key, "Keep-Alive", true)) {
            isAlive = true;
        } else if (TokenUtil::strCmp(&key, "Close", true)) {
            isAlive = false;
        } else {
            /* do nothing */
        }
    }

    return isAlive;
}

void TestDemo::rspHttpFrame(int hd, int ver,
    bool isAlive, Cache* body, int blen) {
    NodeMsg* rsp = NULL;
    HttpCache* pp[1] = {NULL};
    HttpCache cache;
    
    HttpUtil::init(&cache);

    HttpUtil::addRspParam(&cache, ver, RTSP_STATUS_OK);

    if (isAlive) {
        HttpUtil::addEnumField(&cache, ENUM_HTTP_CONNECTION, "Keep-Alive");
    } else {
        HttpUtil::addEnumField(&cache, ENUM_HTTP_CONNECTION, "Close");
    }
    
    pp[0] = &cache;
    rsp = HttpUtil::genHttpCache(pp, 1, body, blen);

    m_center->send(hd, rsp);

    HttpUtil::release(&cache);
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

