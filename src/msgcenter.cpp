#include"msgtool.h"
#include"msgcenter.h"
#include"isockmsg.h"
#include"cache.h"
#include"shareheader.h"


NodeMsg* MsgCenter::allocMsg(int type, int total) {
    NodeMsg* msg = NULL;
    PreHeadHttp* ph = NULL;

    msg = MsgTool::creatPreMsg<PreHeadHttp>(total);
    if (NULL != msg) {
        ph = MsgTool::getPreHead<PreHeadHttp>(msg);
        ph->m_type = type;
    }
    
    return msg;
}

void MsgCenter::freeMsg(NodeMsg* msg) {
    MsgTool::freeMsg(msg);
}

NodeMsg* MsgCenter::refMsg(NodeMsg* pb) {
    NodeMsg* msg = NULL;
    PreHeadHttp* ph1 = NULL;
    PreHeadHttp* ph2 = NULL;

    msg = MsgTool::refPreMsg<PreHeadHttp>(pb);
    if (NULL != msg) {
        ph1 = MsgTool::getPreHead<PreHeadHttp>(msg);
        ph2 = MsgTool::getPreHead<PreHeadHttp>(pb);
        
        ph1->m_type = ph2->m_type;
    }
    
    return msg;
}

void MsgCenter::freeFrameHdCb(char* psz) {
    HttpFrameHd* http = (HttpFrameHd*)psz;

    CacheUtil::del(http->m_head);
    CacheUtil::del(http->m_body);

    http->m_head = NULL;
    http->m_body = NULL;
}

NodeMsg* MsgCenter::allocFrameHd(int line) {
    NodeMsg* msg = NULL;
    int total = sizeof(HttpFrameHd) + sizeof(Token) * line * 2;

    msg = allocMsg(ENUM_HTTP_FRAME_HD, total);
    if (NULL != msg) {
        MsgTool::setCb(msg, &MsgCenter::freeFrameHdCb);
    }
    
    return msg;
}

NodeMsg* MsgCenter::allocHttpMsg(int total) {
    NodeMsg* msg = NULL;

    msg = allocMsg(ENUM_HTTP_MSG, total); 
    return msg;
}

int MsgCenter::getType(NodeMsg* msg) {
    PreHeadHttp* ph = NULL;

    ph = MsgTool::getPreHead<PreHeadHttp>(msg);
    return ph->m_type;
}

int MsgCenter::getTotal(NodeMsg* pb) {
    return MsgTool::getLeft(pb) +
        MsgTool::getLeft(pb, true);
}

char* MsgCenter::getMsg(NodeMsg* msg, bool ext) {
    return MsgTool::getMsg(msg, ext);
}

char* MsgCenter::getCurr(NodeMsg* msg, bool ext) {
    return MsgTool::getCurr(msg, ext);
}

int MsgCenter::getLeft(NodeMsg* msg, bool ext) {
    return MsgTool::getLeft(msg, ext);
}

int MsgCenter::getMsgSize(NodeMsg* msg, bool ext) {
    return MsgTool::getMsgSize(msg, ext);
}

int MsgCenter::getMsgPos(NodeMsg* msg, bool ext) {
    return MsgTool::getMsgPos(msg, ext);
}

void MsgCenter::setMsgSize(NodeMsg* pb, int size, bool ext) {
    MsgTool::setMsgSize(pb, size, ext);
}

void MsgCenter::setMsgPos(NodeMsg* pb, int pos, bool ext) {
    MsgTool::setMsgPos(pb, pos, ext);
}

void MsgCenter::skipMsgPos(NodeMsg* pb, int pos, bool ext) {
    MsgTool::skipMsgPos(pb, pos, ext);
}

Cache* MsgCenter::getCache(NodeMsg* pb, bool ext) {
    Buffer* buffer = NULL;
    
    buffer = MsgTool::getBuffer(pb, ext);
    return buffer->m_cache;
}

void MsgCenter::setCache(NodeMsg* pb, 
    Cache* cache, int size, bool ext) {
    MsgTool::setCache(pb, cache, size, ext);
}

NodeMsg* MsgCenter::allocUdpMsg(int size) {
    return MsgTool::allocUdpMsg(size);
}

NodeMsg* MsgCenter::refUdpMsg(NodeMsg* msg) {
    return MsgTool::refUdpMsg(msg);
}

int MsgCenter::setUdpAddr(NodeMsg* msg, const SockAddr& addr) {
    return MsgTool::setUdpAddr(msg, addr);
}

const SockAddr* MsgCenter::getUdpAddr(NodeMsg* msg) {
    return MsgTool::getUdpAddr(msg);
}

