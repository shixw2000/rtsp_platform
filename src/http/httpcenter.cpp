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


static const int PRINT_HTTP_BODY_LEN = 10;

HttpCenter::HttpCenter(SockFrame* frame) 
    : m_frame(frame) {
    m_tool = NULL;
    m_dealer = NULL;
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

void HttpCenter::setDealer(IHttpDealer* dealer) {
    m_dealer = dealer;
}

void HttpCenter::resetCtx(HttpCtx* ctx) {
    HttpUtil::release(&ctx->m_head);
    HttpUtil::release(&ctx->m_body);
    
    MiscTool::bzero(ctx, sizeof(HttpCtx)); 
}

HttpCtx* HttpCenter::allocHttpEnv() {
    HttpCtx* ctx = NULL;

    if (NULL != m_dealer) {
        ctx = m_dealer->allocHttpEnv();
        if (NULL != ctx) {
            MiscTool::bzero(ctx, sizeof(HttpCtx));
        }
    }
    
    return ctx;
}

void HttpCenter::freeHttpCtx(HttpCtx* ctx) { 
    if (NULL != ctx) {
        resetCtx(ctx);

        if (NULL != m_dealer) {
            m_dealer->freeHttpCtx(ctx);
        }
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

int HttpCenter::send(int fd, NodeMsg* msg) {
    int ret = 0;
    int elen = 0;
    int prntlen = 0;

    elen = MsgCenter::getExtraSize(msg);
    if (PRINT_HTTP_BODY_LEN < elen) {
        prntlen = PRINT_HTTP_BODY_LEN;
    } else {
        prntlen = elen;
    }

    LOG_DEBUG("send_msg| hlen=%d| elen=%d|"
        " msg=%.*s%.*s|",
        MsgCenter::getMsgSize(msg), elen,  
        MsgCenter::getMsgSize(msg),
        MsgCenter::getMsg(msg),
        prntlen, MsgCenter::getExtraData(msg));

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
            " hd_size=%d| body_size=%d|\n"
            "%.*s %.*s %s\n",
            body->m_seq, body->m_is_end,
            body->m_is_chunk,
            body->m_line_no,
            body->m_content_len,
            body->m_hd_size,
            body->m_body_size,
            key.m_len, key.m_str,
            val.m_len, val.m_str,
            HttpUtil::getVersionStr(body->m_http_ver));
    } else {
        len = MiscTool::strPrint(psz, max, "print_http_hd|"
            " seq=%u| isEnd=%d| is_chunk=%d|"
            " line_no=%d| content_len=%d|"
            " hd_size=%d| body_size=%d|\n"
            "%s %.*s %.*s\n",
            body->m_seq, body->m_is_end,
            body->m_is_chunk,
            body->m_line_no,
            body->m_content_len,
            body->m_hd_size,
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

void HttpCenter::printFrameMid(NodeMsg* msg) {
    const HttpFrameMid* body = NULL;
    char* psz = NULL;
    char* txt = NULL;
    int max = 0;
    int txtlen = 0;
    int len = 0;
    char tmp[1024] = {0};

    psz = tmp;
    max = sizeof(tmp);

    body = MsgCenter::getBody<HttpFrameMid>(msg);

    len = MiscTool::strPrint(psz, max, "print_http_mid|"
        " seq=%u| isEnd=%d| is_chunk=%d|"
        " frame_beg=%d| body_size=%d|\n",
        body->m_seq, body->m_is_end,
        body->m_is_chunk,
        body->m_frame_beg,
        body->m_body_size); 
    if (0 < len && len < max) {
        psz += len;
        max -= len;
    }

    if (0 < body->m_body_size) {
        txt = CacheUtil::data(body->m_body);

        if (PRINT_HTTP_BODY_LEN > body->m_body_size) {
            txtlen = body->m_body_size;
        } else {
            txtlen = PRINT_HTTP_BODY_LEN;
        }
    }
    
    len = MiscTool::strPrint(psz, max, "body=%.*s|", txtlen, txt);
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

HttpCtx* HttpCenter::creatSvrSock(int fd) {
    HttpCtx* env = NULL;
    bool bOk = false;

    env = allocHttpEnv();
    if (NULL != env && NULL != m_dealer) {
        bOk = m_dealer->bindCtx(fd, env);
        if (!bOk) {
            freeHttpCtx(env);

            env = NULL;
        }
    }
    
    return env;
}

HttpCtx* HttpCenter::creatCliSock() {
    HttpCtx* env = NULL;

    env = allocHttpEnv();
    return env;
}

void HttpCenter::onListenerClose(int) { 
}

void HttpCenter::onConnFail(long extra, int) {
    HttpCtx* ctx = NULL;
    
    ctx = (HttpCtx*)extra;
    freeHttpCtx(ctx);
}

int HttpCenter::onConnOK(int hd) {
    HttpCtx* env = NULL;
    int ret = 0; 
    bool bOk = false;

    env = (HttpCtx*)m_frame->getExtra(hd);
    if (NULL != m_dealer) {
        bOk = m_dealer->bindCtx(hd, env);
        if (!bOk) {
            ret = -1;
        }
    }
  
    return ret;
} 

void HttpCenter::onClose(int hd) {
    HttpCtx* ctx = NULL;

    ctx = (HttpCtx*)m_frame->getExtra(hd);
    freeHttpCtx(ctx);
}

int HttpCenter::process(int hd, NodeMsg* msg) { 
    HttpCtx* ctx = NULL;
    int ret = 0; 
    int type = 0;

    ctx = (HttpCtx*)m_frame->getExtra(hd);
    
    type = MsgCenter::getType(msg);
    switch (type) {
    case ENUM_HTTP_FRAME_HD:
        ret = procFrameHd(hd, ctx, msg);
        break;

    case ENUM_HTTP_FRAME_MID:
        ret = procFrameMid(hd, ctx, msg);
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
    unsigned seq = 0;

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
    http->m_is_chunk = ctx->m_is_chunk;
    http->m_is_end = isEnd;
    http->m_seq = seq;
    http->m_content_len = ctx->m_content_len;

    if (0 < ctx->m_line_no) {
        total = ctx->m_line_no * 2 * sizeof(Range);
        
        MiscTool::bcopy(http->m_hd_opts,
            ctx->m_ranges, total);
    } 
    
    return msg;
}

NodeMsg* HttpCenter::creatFrameMid(HttpCtx* ctx, bool isEnd) {
    NodeMsg* msg = NULL;
    HttpFrameMid* http = NULL;
    unsigned seq = 0;

    msg = MsgCenter::allocFrameMid();
    http = MsgCenter::getBody<HttpFrameMid>(msg); 
    
    http->m_body_size = HttpUtil::totalRd(&ctx->m_body);
    if (0 < http->m_body_size) {
        http->m_body = CacheUtil::ref(ctx->m_body.m_cache);
    }
    
    http->m_frame_beg = ctx->m_frame_beg;
    http->m_is_end = isEnd;
    http->m_is_chunk = ctx->m_is_chunk;
    http->m_seq = seq; 
    
    return msg;
}

int HttpCenter::procFrameHd(int hd, HttpCtx* ctx, NodeMsg* msg) {
    int ret = 0; 
    
    printFrameHd(msg);

    if (NULL != m_dealer) {
        ret = m_dealer->procFrameHd(hd, ctx, msg);
    }
    
    return ret;
}

int HttpCenter::procFrameMid(int hd, HttpCtx* ctx, NodeMsg* msg) {
    int ret = 0;

    printFrameMid(msg);

    if (NULL != m_dealer) {
        ret = m_dealer->procFrameMid(hd, ctx, msg); 
    }
    
    return ret;
} 

