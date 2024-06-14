#include<cstdlib>
#include<cstdio>
#include<cstring>
#include"httptool.h"
#include"sockframe.h"
#include"msgcenter.h"
#include"httpcenter.h"
#include"tokenutil.h"
#include"cache.h"
#include"misc.h"
#include"isockmsg.h"
#include"ihttpdealer.h"
#include"httputil.h"


static const int PRINT_HTTP_BODY_LEN = 100;

HttpCenter::HttpCenter(SockFrame* frame) 
    : m_frame(frame) {
    m_tool = NULL;
    
    MiscTool::bzero(m_dealer, sizeof(m_dealer));
}

HttpCenter::~HttpCenter() {
}

int HttpCenter::init() {
    int ret = 0; 

    m_tool = new HttpTool(this);

    return ret;
}

void HttpCenter::finish() {
    if (NULL != m_tool) {
        delete m_tool;
        m_tool = NULL;
    }
}

void HttpCenter::addDealer(IHttpDealer* dealer) {
    for (int i=0; i<MAX_DEALER_SIZE; ++i) {
        if (NULL == m_dealer[i]) {
            m_dealer[i] = dealer;
            break;
        }
    }
}

void HttpCenter::resetCtx(HttpCtx* ctx) {
    
    HttpUtil::release(&ctx->m_head);
    HttpUtil::release(&ctx->m_body);
    
    MiscTool::bzero(ctx, sizeof(HttpCtx)); 
}

HttpCtx* HttpCenter::allocHttp() {
    HttpCtx* ctx = NULL;
    
    ctx = (HttpCtx*)CacheUtil::mallocAlign(sizeof(HttpCtx));
    MiscTool::bzero(ctx, sizeof(HttpCtx));
    return ctx;
}

void HttpCenter::freeHttp(HttpCtx* ctx) { 
    if (NULL != ctx) {
        resetCtx(ctx);
        
        CacheUtil::freeAlign(ctx);
    }
}

int HttpCenter::dispatch(int fd, NodeMsg* msg) {
    int ret = 0;

    ret = m_frame->dispatch(fd, msg);
    if (0 != ret) {
        LOG_INFO("send_msg| fd=%d| ret=%d|"
            " msg=send error|",
            fd, ret);
    }
    
    return ret;
}

void HttpCenter::printHttpSendMsg(NodeMsg* msg) {
    int hlen = 0;
    int blen = 0;
    int prntlen = 0;

    hlen = MsgCenter::getMsgSize(msg);
    
    blen = MsgCenter::getMsgSize(msg, true);
    if (PRINT_HTTP_BODY_LEN < blen) {
        prntlen = PRINT_HTTP_BODY_LEN;
    } else {
        prntlen = blen;
    }

    LOG_DEBUG("print_http_send_msg| hlen=%d| blen=%d|"
        " msg=%.*s%.*s|",
        hlen, blen,  
        hlen, MsgCenter::getMsg(msg),
        prntlen, MsgCenter::getMsg(msg, true));
}

int HttpCenter::send(int fd, NodeMsg* msg) {
    int ret = 0;

    printHttpSendMsg(msg);

    ret = m_frame->sendMsg(fd, msg);
    if (0 != ret) {
        LOG_INFO("send_msg| fd=%d| ret=%d|"
            " msg=send error|",
            fd, ret);
    }
    
    return ret;
}

void HttpCenter::getLine(const HttpFrameHd* frame,
    int line, Token* key, Token* val) {
    const char* psz = NULL;
    int n = 0;
    Token t[2];
    
    if (0 <= line && line < frame->m_line_no) {
        n = line * 2;
        psz = CacheUtil::data(frame->m_head);
        
        TokenUtil::range2Token(&t[0], psz, &frame->m_hd_opts[n]);
        TokenUtil::range2Token(&t[1], psz, &frame->m_hd_opts[n+1]);
    }

    if (NULL != key) {
        *key = t[0];
    }

    if (NULL != val) {
        *val = t[1];
    }
}

bool HttpCenter::findEnumField(
    const HttpFrameHd* frame,
    EnumHttpField enType, Token* val) {
    const char* psz = NULL;

    psz = HttpUtil::getFieldStr(enType);
    return findField(frame, psz, val);
}

bool HttpCenter::findField(
    const HttpFrameHd* frame,
    const Token& name, Token* val) {
    Token t[2];
    
    for (int i=1; i<frame->m_line_no; ++i) {
        getLine(frame, i, &t[0], &t[1]);
        if (TokenUtil::strCmp(&t[0], name, true)) {
            if (NULL != val) {
                *val = t[1];
            }

            return true;
        }
    }

    if (NULL != val) {
        val->set(NULL, 0);
    }

    return false;
}

void HttpCenter::printFrameHd(NodeMsg* msg) {
    const HttpFrameHd* body = NULL;
    char* psz = NULL;
    char* txt = NULL;
    int max = 0;
    int txtlen = 0;
    int len = 0;
    char tmp[1024] = {0};
    Token key;
    Token val;

    psz = tmp;
    max = sizeof(tmp);

    body = MsgCenter::getBody<HttpFrameHd>(msg);
    
    /* get first line */
    getLine(body, 0, &key, &val); 
    if (body->m_is_req) {
        len = MiscTool::strPrint(psz, max, "print_http_hd|"
            " seq=%u| isEnd=%d| is_chunk=%d|"
            " line_no=%d| content_len=%d|"
            " hd_size=%d| frame_beg=%d|"
            " body_size=%d|\n"
            "%.*s %.*s %s\n",
            body->m_seq, body->m_is_end,
            body->m_is_chunk,
            body->m_line_no,
            body->m_content_len,
            body->m_hd_size,
            body->m_frame_beg,
            body->m_body_size,
            key.m_len, key.m_str,
            val.m_len, val.m_str,
            HttpUtil::getVersionStr(body->m_http_ver));
    } else {
        len = MiscTool::strPrint(psz, max, "print_http_hd|"
            " seq=%u| isEnd=%d| is_chunk=%d|"
            " line_no=%d| content_len=%d|"
            " hd_size=%d| frame_beg=%d|"
            " body_size=%d|\n"
            "%s %.*s %.*s\n",
            body->m_seq, body->m_is_end,
            body->m_is_chunk,
            body->m_line_no,
            body->m_content_len,
            body->m_hd_size,
            body->m_frame_beg,
            body->m_body_size,
            HttpUtil::getVersionStr(body->m_http_ver),
            key.m_len, key.m_str,
            val.m_len, val.m_str);
    }

    if (0 < len && len < max) {
        psz += len;
        max -= len;
    }

    for (int i=1; 0 < max && i<body->m_line_no; ++i) {
        getLine(body, i, &key, &val); 
        
        len = MiscTool::strPrint(psz, max, "%.*s: %.*s\n",
            key.m_len, key.m_str,
            val.m_len, val.m_str);
        if (0 < len && len < max) {
            psz += len;
            max -= len;
        } else {
            break;
        }
    }

    if (0 < body->m_body_size) {
        txt = CacheUtil::data(body->m_body);

        if (PRINT_HTTP_BODY_LEN > body->m_body_size) {
            txtlen = body->m_body_size;
        } else {
            txtlen = PRINT_HTTP_BODY_LEN;
        }
    }
    
    len = MiscTool::strPrint(psz, max, "body=\n%.*s|", txtlen, txt);
    if (0 < len) { 
        psz += len;
        max -= len;
    }

    LOG_DEBUG("%s", tmp);
}

int HttpCenter::parseData(int fd, const char* buf, int size) {
    HttpCtx* ctx = NULL;
    int ret = 0;

    ctx = (HttpCtx*)m_frame->getExtra(fd);
    
    ret = m_tool->parseData(fd, ctx, buf, size);
    return ret;
}

HttpCtx* HttpCenter::creatSock() {
    HttpCtx* env = NULL;

    env = allocHttp(); 
    return env;
}

void HttpCenter::onListenerClose(int) { 
}

void HttpCenter::onConnFail(long extra, int) {
    HttpCtx* ctx = NULL;
    
    ctx = (HttpCtx*)extra;
    freeHttp(ctx);
}

int HttpCenter::onConnOK(int) { 
    return 0;
} 

void HttpCenter::onClose(int hd) {
    HttpCtx* ctx = NULL;

    ctx = (HttpCtx*)m_frame->getExtra(hd);
    freeHttp(ctx);
}

int HttpCenter::process(int hd, NodeMsg* msg) { 
    int ret = 0; 
    int type = 0;

    type = MsgCenter::getType(msg);
    switch (type) {
    case ENUM_HTTP_FRAME_HD:
        ret = procFrameHd(hd, msg);
        break;

    default:
        break;
    }

    return ret;
}

NodeMsg* HttpCenter::creatFrameHd(HttpCtx* ctx, bool isEnd) {
    NodeMsg* msg = NULL;
    HttpFrameHd* http = NULL;
    int total = 0;

    msg = MsgCenter::allocFrameHd(ctx->m_line_no);
    http = MsgCenter::getBody<HttpFrameHd>(msg); 

    http->m_hd_size = HttpUtil::totalRd(&ctx->m_head);
    if (0 < http->m_hd_size) {
        http->m_head = CacheUtil::ref(ctx->m_head.m_cache);
    }
    
    http->m_body_size = HttpUtil::totalRd(&ctx->m_body);
    if (0 < http->m_body_size) {
        http->m_body = CacheUtil::ref(ctx->m_body.m_cache);
    }
    
    http->m_line_no = ctx->m_line_no;
    http->m_http_ver = ctx->m_http_ver;
    http->m_is_req = ctx->m_is_req;

    http->m_frame_beg = ctx->m_frame_beg;
    http->m_is_chunk = ctx->m_is_chunk;
    http->m_is_end = isEnd;
    http->m_seq = ++ctx->m_seq;
    http->m_content_len = ctx->m_content_len;

    if (0 < ctx->m_line_no) {
        total = ctx->m_line_no * 2 * sizeof(Range);
        
        MiscTool::bcopy(http->m_hd_opts,
            ctx->m_ranges, total);
    } 
    
    return msg;
}

int HttpCenter::procFrameHd(int hd, NodeMsg* msg) {
    int ret = 0; 
    bool done = false;
    
    printFrameHd(msg);

    for (int i=0; !done && i<MAX_DEALER_SIZE; ++i) {
        if (NULL != m_dealer[i]) {
            done = m_dealer[i]->procFrameHd(hd, msg);
        } else {
            break;
        }
    }
    
    return ret;
}

