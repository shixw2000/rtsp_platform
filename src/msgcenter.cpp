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

void MsgCenter::setExtraCache(NodeMsg* pb, 
        Cache* cache, int size) {
    MsgTool::setExtraCache(pb, cache, size);
}

Cache* MsgCenter::getExtraCache(NodeMsg* pb) {
    return MsgTool::getExtraCache(pb);
}

int MsgCenter::getExtraSize(NodeMsg* pb) {
    return MsgTool::getExtraSize(pb);
}

int MsgCenter::getExtraPos(NodeMsg* pb) {
    return MsgTool::getExtraPos(pb);
}

char* MsgCenter::getExtraData(NodeMsg* pb) {
    Cache* cache = NULL;

    cache = MsgTool::getExtraCache(pb);
    if (NULL != cache) {
        return CacheUtil::data(cache);
    } else {
        return NULL;
    }
}

void MsgCenter::setExtraPos(NodeMsg* pb, int pos) {
    MsgTool::setExtraPos(pb, pos);
}

void MsgCenter::freeFrameHdCb(char* psz) {
    HttpFrameHd* http = (HttpFrameHd*)psz;

    CacheUtil::del(http->m_head);
    CacheUtil::del(http->m_body);

    http->m_head = NULL;
    http->m_body = NULL;
}

void MsgCenter::freeFrameMidCb(char* psz) {
    HttpFrameMid* http = (HttpFrameMid*)psz;

    CacheUtil::del(http->m_body);
    http->m_body = NULL;
}

NodeMsg* MsgCenter::allocFrameHd(int line) {
    NodeMsg* msg = NULL;
    int total = sizeof(HttpFrameHd) + sizeof(Token) * line * 2;

    msg = allocMsg(ENUM_HTTP_FRAME_HD, total);
    if (NULL != msg) {
        MsgTool::setInfraCb(msg, &MsgCenter::freeFrameHdCb);
    }
    
    return msg;
}

NodeMsg* MsgCenter::allocFrameMid() {
    NodeMsg* msg = NULL;
    int total = sizeof(HttpFrameMid);

    msg = allocMsg(ENUM_HTTP_FRAME_MID, total); 
    if (NULL != msg) {
        MsgTool::setInfraCb(msg, &MsgCenter::freeFrameMidCb);
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

char* MsgCenter::getMsg(NodeMsg* msg) {
    return MsgTool::getMsg(msg);
}

int MsgCenter::getMsgSize(NodeMsg* msg) {
    return MsgTool::getMsgSize(msg);
}

