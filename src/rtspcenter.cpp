#include<cstdlib>
#include<cstring>
#include"httpdata.h"
#include"tokenutil.h"
#include"msgcenter.h"
#include"rtspcenter.h"
#include"httputil.h"
#include"isockmsg.h"
#include"cache.h"
#include"rtspdata.h"
#include"httpcenter.h"
#include"demo.h"
#include"misc.h"
#include"rtspres.h"
#include"rtspsess.h"
#include"socktool.h"


static const Token DEF_NULL_TOKEN(NULL, 0);

static const char DEF_RTSP_PUBLIC_VAL[] =
    "OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, RECORD";

RtspHandler::RtspHandler(HttpCenter* center)
    : m_center(center) {

    m_dealer = new TestDemo(center);
    m_resource = new RtspRes;
    m_sess = new RtspSess;
}

RtspHandler::~RtspHandler() {
    if (NULL != m_dealer) {
        delete m_dealer;
        m_dealer = NULL;
    }

    if (NULL != m_resource) {
        delete m_resource;
        m_resource = NULL;
    }

    if (NULL != m_sess) {
        delete m_sess;
        m_sess = NULL;
    }
}

void RtspHandler::parseSession(
    RtspReq* req, Token* token) {
    bool bOk = false; 
    Token key;
    Token val; 

    bOk = TokenUtil::next(&val, token, ";");
    if (bOk) {
        TokenUtil::copy(req->m_sid, MAX_SESS_ID_LEN, &val); 
    
        TokenUtil::skipOf(token, "; "); 
        bOk = TokenUtil::split(&key, &val, token, '=');
        if (bOk) {
            TokenUtil::toNum(&req->m_sid_timeout, &val);
        }
    }
}

bool RtspHandler::chkRtpUdp(Token* token) { 
    bool bOk = false;
    Token val[3];
    
    for (int i=0; i<3; ++i) {
        bOk = TokenUtil::next(&val[i], token, "/");
        if (!bOk) {
            break;
        }
    }

    if (TokenUtil::strCmp(&val[0], "rtp", true) &&
        !TokenUtil::strCmp(&val[2], "tcp", true)) {
        return true;
    } else {
        return false;
    }
}

bool RtspHandler::parsePort(Token* token, 
    int& port_min, int& port_max) {
    bool bOk = false;
    Token t[2];

    bOk = TokenUtil::split(&t[0], &t[1], token, '-');
    if (bOk) {
        TokenUtil::toNum(&port_min, &t[0]);
        TokenUtil::toNum(&port_max, &t[1]); 
    }

    return bOk;
}

void RtspHandler::parseParam(
    RtspReq* req, Token* token) { 
    RtspTransport* param = &req->m_transport;
    bool bOk = false;
    Token key;
    Token val;

    bOk = TokenUtil::next(&key, token, "=");
    if (bOk) {
        /* may not has */
        TokenUtil::next(&val, token, "=");
    }

    if (TokenUtil::strCmp(&key, "port", true)) {
        parsePort(&val, param->m_port_min,
            param->m_port_max);
    } else if (TokenUtil::strCmp(&key, "client_port", true)) {
        parsePort(&val, param->m_client_port_min,
            param->m_client_port_max);
    } else if (TokenUtil::strCmp(&key, "server_port", true)) {
        parsePort(&val, param->m_server_port_min,
            param->m_server_port_max);
    } else if (TokenUtil::strCmp(&key, "unicast", true)) {
        param->m_is_unicast = true;
    } else if (TokenUtil::strCmp(&key, "ttl", true)) {
        TokenUtil::toNum(&param->m_ttl, &val);
    } else if (TokenUtil::strCmp(&key, "destination", true)) {
        TokenUtil::copy(param->m_dest, sizeof(param->m_dest), &val);
    } else if (TokenUtil::strCmp(&key, "source", true)) {
        TokenUtil::copy(param->m_source, 
            sizeof(param->m_source), &val);
    } else if (TokenUtil::strCmp(&key, "mode", true)) {
    
    } else {
    }
}

void RtspHandler::parseTransport(
    RtspReq* req, Token* token) {
    bool bOk = false; 
    Token part1;
    Token part2;

    while (TokenUtil::next(&part1, token, ",")) {
        bOk = TokenUtil::next(&part2, &part1, ";");
        if (!bOk) {
            continue;
        }
        
        bOk = chkRtpUdp(&part2);
        if (bOk) {
            req->m_transport.m_is_rtp_udp = true;
        } else {
            continue;
        }
        
        while (TokenUtil::next(&part2, &part1, ";")) {
            parseParam(req, &part2);
        } 
        
        break;
    }
}

void RtspHandler::parseAuthInfo(
    RtspReq* req, Token* token) {
    bool bOk = false; 
    Token key; 
    Token val;
    Token t;

    bOk = TokenUtil::next(&t, token, " ");
    if (!bOk) {
        return;
    }
    
    if (TokenUtil::strCmp(&t, "Basic", true)) {
        while (TokenUtil::next(&t, token, ",")) {
            bOk = TokenUtil::split(&key, &val, token, '=');
            if (bOk && TokenUtil::strCmp(&key, "realm", true)) {
                TokenUtil::copy(req->m_auth.m_realm,
                    sizeof(req->m_auth.m_realm), &val);
                break;
            }
        }
    }
}

void RtspHandler::parseField(RtspReq* req,
    Token* key, Token* val) {
    int type = 0;

    type = HttpUtil::getFieldType(key);
    
    if (ENUM_HTTP_SEQ == type) {
        TokenUtil::toNum(&req->m_seq, val);
    } else if (ENUM_HTTP_ACCEPT == type) {
        req->m_accept = *val;
    } else if (ENUM_HTTP_CONTENT_TYPE == type) {
        req->m_content_type = *val;
    } else if (ENUM_HTTP_SESSION == type) {
        parseSession(req, val);
    } else if (ENUM_HTTP_TRANSPORT == type) {
        parseTransport(req, val);
    } else if (ENUM_HTTP_WWW_AUTH == type) {
        parseAuthInfo(req, val);
    } else {
    }
}

int RtspHandler::parseSdp(RtspCtx* ctx,
    RtspReq& req) {
    int ret = 0;

    if (!TokenUtil::strCmp(&req.m_content_type,
        DEF_RTSP_FIELD_CONTENT_TYPE_VAL, true)) {
        return -1;
    }

    if (!(0 < req.m_body.m_len && 
        4096 > req.m_body.m_len)) {
        return -1;
    }

    ret = m_resource->parseSDP(req.m_path, req.m_body);
    if (0 == ret) {
        ctx->m_is_publisher = true;
        TokenUtil::copy(ctx->m_path, sizeof(ctx->m_path),
            &req.m_path);
    }
    
    return ret;
}

void RtspHandler::parseFrame(RtspReq* req,
    HttpFrameHd* frame) {
    Token schema;
    Token auth;
    Token key;
    Token val;

    m_center->getLine(frame, 0, &req->m_method,
        &req->m_url);
    
    HttpUtil::splitUrl(&req->m_url, &schema, 
        &auth, &req->m_host,
        &req->m_port, &req->m_path);
    
    LOG_DEBUG("parse_url| url=%.*s| schema=%.*s| auth=%.*s|"
        " ip=%.*s| port=%d| path=%.*s|",
        req->m_url.m_len, req->m_url.m_str,
        schema.m_len, schema.m_str,
        auth.m_len, auth.m_str,
        req->m_host.m_len, 
        req->m_host.m_str,
        req->m_port,
        req->m_path.m_len, 
        req->m_path.m_str); 
    
    for (int i=1; i<frame->m_line_no; ++i) {
        m_center->getLine(frame, i, &key, &val);
        
        parseField(req, &key, &val);
    }

    if (0 < frame->m_body_size) {
        req->m_body.set(CacheUtil::data(frame->m_body),
            frame->m_body_size);
    } else {
        req->m_body.set(NULL, 0);
    }
}

void RtspHandler::resetRtsp(RtspCtx* ctx) {
    MiscTool::bzero(ctx, sizeof(RtspCtx));
}

HttpCtx* RtspHandler::allocHttpEnv() {
    RtspEnv* env = NULL;
    
    env = (RtspEnv*)CacheUtil::mallocAlign(sizeof(RtspEnv));
    
    resetRtsp(&env->m_rtsp);
    return &env->m_http;
}

void RtspHandler::freeHttpCtx(HttpCtx* ctx) {
    RtspEnv* env = (RtspEnv*)ctx;
    
    if (NULL != env) {
        resetRtsp(&env->m_rtsp);
        
        CacheUtil::freeAlign(env);
    }
}

bool RtspHandler::bindCtx(int fd, HttpCtx* ctx) {
    RtspEnv* env = NULL;
    RtspCtx* rtsp = NULL;
    int ret = 0;
    SockAddr addr;
    SockName name;
    Token tmp;

    env = (RtspEnv*)ctx; 
    rtsp = &env->m_rtsp;

    ret = SockTool::getLocalSock(fd, addr);
    if (0 == ret) {
        SockTool::addr2IP(&name, &addr);
        
        tmp.set(name.m_ip);
        TokenUtil::copy(rtsp->m_server_ip, 
            sizeof(rtsp->m_server_ip), &tmp);
    }
    
    ret = SockTool::getPeerSock(fd, addr);
    if (0 == ret) {
        SockTool::addr2IP(&name, &addr);
        
        tmp.set(name.m_ip);
        TokenUtil::copy(rtsp->m_client_ip, 
            sizeof(rtsp->m_client_ip), &tmp);
    }
    
    return true;
}

int RtspHandler::procFrameHd(int hd, HttpCtx* ctx, NodeMsg* msg) { 
    HttpFrameHd* frame = NULL;
    RtspEnv* env = NULL;
    bool done = false;
    int ret = 0;
    RtspRsp rsp;

    MiscTool::bzero(&rsp, sizeof(RtspRsp));
    rsp.m_errcode = RTSP_STATUS_OK;
    
    env = (RtspEnv*)ctx; 
    frame = MsgCenter::getBody<HttpFrameHd>(msg);
    
    ret = dealRtspMsg(hd, &env->m_rtsp, frame, rsp, done);
    if (done) {
        rtspReply(hd, &env->m_rtsp, rsp);
    } else { 
        ret = m_dealer->procFrameHd(hd, ctx, msg);
    }

    HttpUtil::release(&rsp.m_head);
    HttpUtil::release(&rsp.m_body);
    return ret;
}

int RtspHandler::procFrameMid(int, HttpCtx* ctx, NodeMsg*) {
    RtspEnv* env = (RtspEnv*)ctx;

    (void)env;
    return 0;
}

void RtspHandler::rtspReply(int fd,
    RtspCtx* ctx, RtspRsp& rsp) {
    NodeMsg* msg = NULL;
    HttpCache* pp[2] = {NULL};
    HttpCache cmdCache;

    HttpUtil::init(&cmdCache);
    
    HttpUtil::addRspParam(&cmdCache, 
        RTSP_VER_10_VAL, rsp.m_errcode);
    
    HttpUtil::addEnumFieldInt(&cmdCache,
        ENUM_HTTP_SEQ, ctx->m_seq);

    pp[0] = &cmdCache;
    pp[1] = &rsp.m_head;
    msg = HttpUtil::genHttpCache(pp, 2, 
        rsp.m_body.m_cache, 
        rsp.m_body.m_size);
    if (NULL != msg) { 
        m_center->send(fd, msg);
    }

    HttpUtil::release(&cmdCache);
}

int RtspHandler::dealRtspMsg(int, RtspCtx* ctx,
    HttpFrameHd* frame, RtspRsp& rsp, bool& done) {
    int ret = 0; 
    RtspReq req;

    parseFrame(&req, frame); 

    ctx->m_seq = req.m_seq;
    
    if (TokenUtil::strCmp(&req.m_method, "OPTIONS")) {
        done = true;
        ret = dealOption(ctx, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "DESCRIBE")) {
        done = true;
        ret = dealDescribe(ctx, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "SETUP")) {
        done = true;
        ret = dealSetup(ctx, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "ANNOUNCE")) {
        done = true;
        ret = dealAnnounce(ctx, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "RECORD")) {
        done = true;
        ret = dealRecord(ctx, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "PLAY")) {
        done = true;
        ret = dealPlay(ctx, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "PAUSE")) {
        done = true;
    } else if (TokenUtil::strCmp(&req.m_method, "TEARDOWN")) {
        done = true;
        ret = dealTeardown(ctx, req, rsp);
    } else {
        done = false;
    } 
    
    return ret;
}

int RtspHandler::dealOption(RtspCtx*, 
    RtspReq&, RtspRsp& rsp) {
    int ret = 0;
    
    HttpUtil::addEnumField(&rsp.m_head,
        ENUM_HTTP_PUBLIC, DEF_RTSP_PUBLIC_VAL);
    
    rsp.m_errcode = RTSP_STATUS_OK;
    return ret;
}

int RtspHandler::dealDescribe(RtspCtx* ctx,
    RtspReq& req, RtspRsp& rsp) {
    int ret = 0;

    do { 
        ret = m_resource->prepareSDP(
            &rsp.m_body, req.m_path);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        ctx->m_is_publisher = false;
        TokenUtil::copy(ctx->m_path, sizeof(ctx->m_path),
            &req.m_path); 

        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_CONTENT_BASE, req.m_url);

        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_CONTENT_TYPE, 
            DEF_RTSP_FIELD_CONTENT_TYPE_VAL);
        
        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);
    
    return ret;
}

int RtspHandler::prepareSess(RtspCtx* ctx, 
    RtspReq& req) {
    SessionCtx* sess = NULL;
    char buf[DEF_SESS_NUM_LEN] = {0};
    
    if (DEF_NULL_CHAR == req.m_sid[0]) {
        MiscTool::getRand(buf, DEF_SESS_NUM_LEN);
        TokenUtil::toHexStr(req.m_sid, 
            buf, DEF_SESS_NUM_LEN);

        strncpy(ctx->m_sid, req.m_sid,
            DEF_SESS_NUM_LEN);
    } else if (chkSid(ctx, req)) {
        /* sid valid */
    } else {
        /* invalid sid */
        return -1;
    }

    if (NULL != ctx->m_sess) {
        return 0;
    } else {
        sess = m_sess->prepareSess(ctx->m_sid, ctx->m_path);
        if (NULL != sess) {
            sess->m_is_publisher = ctx->m_is_publisher;
            
            ctx->m_sess = sess;
            return 0;
        } else {
            return -1;
        }
    } 
}

FFStream* RtspHandler::prepareStream(RtspCtx* ctx, 
    RtspReq& req) {
    SdpMedia* media = NULL;
    FFStream* s = NULL;
    RtspTransport* transport = NULL;

    transport = &req.m_transport;

    do {
        if (!transport->m_is_rtp_udp) {
            break;
        }

        media = m_resource->findMedia(req.m_path);
        if (NULL == media) {
            break;
        }

        s = m_sess->creatStream();
        if (NULL == s) {
            break;
        }

        s->m_media = media;
        s->m_client_port_min = transport->m_client_port_min;
        s->m_client_port_max = transport->m_client_port_max;
        s->m_server_port_min = 80000;
        s->m_server_port_max = 80001;

        SockTool::ip2Net(&s->m_server_addr, ctx->m_server_ip);
        SockTool::ip2Net(&s->m_client_addr, ctx->m_client_ip);

        return s;
    } while (false); 

    if (NULL != s) {
        m_sess->freeStream(s);
    }
    
    return NULL;
}

int RtspHandler::dealSetup(RtspCtx* ctx, 
    RtspReq& req, RtspRsp& rsp) {
    SessionCtx* sess = NULL;
    FFStream* s = NULL;
    int ret = 0;
    Token tmp;
    
    do { 
        ret = prepareSess(ctx, req);
        if (0 == ret) {
            sess = ctx->m_sess;
        } else {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }

        s = prepareStream(ctx, req);
        if (NULL == s) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
        
        s->m_idx = sess->m_stream_cnt;
        sess->m_stream[sess->m_stream_cnt] = s;
        ++sess->m_stream_cnt;

        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        HttpUtil::addFormat(&rsp.m_head,
            "Transport: RTP/AVP/UDP;unicast;"
            "client_port=%d-%d;server_port=%d-%d\r\n",
            s->m_client_port_min,
            s->m_client_port_max,
            s->m_server_port_min,
            s->m_server_port_max);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

int RtspHandler::dealPlay(RtspCtx* ctx, 
    RtspReq& req, RtspRsp& rsp) {
    int ret = 0;

    do {
        if (NULL == ctx->m_sess ||
            !chkSid(ctx, req) ||
            ctx->m_is_publisher) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 
        
        ret = startStream(ctx, req.m_path);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
        
        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

int RtspHandler::dealTeardown(RtspCtx* ctx, 
    RtspReq& req, RtspRsp& rsp) {
    int ret = 0;

    do {
        if (!chkSid(ctx, req)) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        ret = stopStream(ctx, req.m_path);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
      
        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

int RtspHandler::dealAnnounce(RtspCtx* ctx, 
    RtspReq& req, RtspRsp& rsp) {
    int ret = 0;

    do { 
        ret = parseSdp(ctx, req);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

bool RtspHandler::chkSid(RtspCtx* ctx, 
    RtspReq& req) {
    if (0 == strcmp(req.m_sid, ctx->m_sid)) {
        return true;
    } else {
        return false;
    } 
}

int RtspHandler::dealRecord(RtspCtx* ctx, 
    RtspReq& req, RtspRsp& rsp) { 
    int ret = 0;

    do {
        if (NULL == ctx->m_sess ||
            !chkSid(ctx, req) ||
            !ctx->m_is_publisher) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        ret = startStream(ctx, req.m_path);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
        
        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

int RtspHandler::startStream(RtspCtx* ctx, const Token& path) {
    SessionCtx* sess = NULL;
    FFStream* s = NULL;

    sess = ctx->m_sess;
    if (m_sess->isSessPath(sess, path)) {
        /* start all stream */
        for (int i=0; i<sess->m_stream_cnt; ++i) {
            s = sess->m_stream[i];
            s->m_on = true;
        }
    } else {
        s = m_sess->findStream(sess, path);
        if (NULL != s) {
            s->m_on = true;
        } else {
            return -1;
        }
    }

    return 0;
}

int RtspHandler::stopStream(RtspCtx* ctx, const Token& path) {
    SessionCtx* sess = NULL;
    FFStream* s = NULL;

    sess = ctx->m_sess;
    if (m_sess->isSessPath(sess, path)) {
        /* start all stream */
        for (int i=0; i<sess->m_stream_cnt; ++i) {
            s = sess->m_stream[i];
            s->m_on = false;
        }
    } else {
        s = m_sess->findStream(sess, path);
        if (NULL != s) {
            s->m_on = false;
        } else {
            return -1;
        }
    }

    return 0;
}

