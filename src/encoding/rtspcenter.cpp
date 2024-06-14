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
#include"misc.h"
#include"rtspres.h"
#include"rtspsess.h"
#include"rtspsdp.h"
#include"socktool.h"
#include"rtpcenter.h"
#include"sockframe.h"
#include"rtpinst.h"


static const Token DEF_NULL_TOKEN(NULL, 0);

static const char DEF_RTSP_PUBLIC_VAL[] =
    "OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, RECORD";

RtspHandler::RtspHandler(SockFrame* frame, 
    HttpCenter* center)
    : m_frame(frame), m_center(center) { 
    
    m_sess = new RtspSess;
    m_rtp_center = new RtpCenter(frame);
    m_resource = new RtspRes(frame, m_rtp_center);
}

RtspHandler::~RtspHandler() {
    if (NULL != m_resource) {
        delete m_resource;
        m_resource = NULL;
    }

    if (NULL != m_sess) {
        delete m_sess;
        m_sess = NULL;
    }

    if (NULL != m_rtp_center) {
        delete m_rtp_center;
        m_rtp_center = NULL;
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

bool RtspHandler::chkSdpHdr(RtspReq& req) {
    if (!TokenUtil::strCmp(&req.m_content_type,
        DEF_RTSP_FIELD_CONTENT_TYPE_VAL, true)) {
        return false;
    }

    if (!(0 < req.m_body.m_len && 
        4096 > req.m_body.m_len)) {
        return false;
    }

    return true; 
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

RtspCtx* RtspHandler::allocRtsp() {
    RtspCtx* ctx = NULL;
    
    ctx = (RtspCtx*)CacheUtil::mallocAlign(sizeof(RtspCtx)); 
    MiscTool::bzero(ctx, sizeof(RtspCtx));
    return ctx;
}

void RtspHandler::freeRtsp(RtspCtx* ctx) {
    FFStream* ff = NULL;
    
    if (NULL != ctx) {
        for (int i=0; i<ctx->m_stream_cnt; ++i) {
            ff = &ctx->m_stream[i];
            if (ff->m_setup) {
                m_resource->delFF(ff); 
                ff->m_setup = false;
            }
        }
        
        CacheUtil::freeAlign(ctx);
    }
} 

bool RtspHandler::procFrameHd(int hd, NodeMsg* msg) { 
    HttpFrameHd* frame = NULL;
    bool done = false;
    RtspRsp rsp;
    RtspReq req; 

    MiscTool::bzero(&rsp, sizeof(rsp));
    MiscTool::bzero(&req, sizeof(req));

    frame = MsgCenter::getBody<HttpFrameHd>(msg);
    parseFrame(&req, frame); 
    
    rsp.m_errcode = RTSP_STATUS_OK;
    rsp.m_seq = req.m_seq;
    
    done = dealRtspMsg(hd, req, rsp);
    
    HttpUtil::release(&rsp.m_head);
    HttpUtil::release(&rsp.m_body);
    
    return done;
}

void RtspHandler::rtspReply(int fd, RtspRsp& rsp) {
    NodeMsg* msg = NULL;
    HttpCache* pp[2] = {NULL};
    HttpCache cmdCache;

    HttpUtil::init(&cmdCache);
    
    HttpUtil::addRspParam(&cmdCache, 
        RTSP_VER_10_VAL, rsp.m_errcode);
    
    HttpUtil::addEnumFieldInt(&cmdCache, 
        ENUM_HTTP_SEQ, rsp.m_seq);

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

bool RtspHandler::dealRtspMsg(int hd, RtspReq& req, RtspRsp& rsp) {
    int ret = 0;
    bool done = false;

    if (TokenUtil::strCmp(&req.m_method, "OPTIONS")) {
        done = true;
        ret = dealOption(hd, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "DESCRIBE")) {
        done = true;
        ret = dealDescribe(hd, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "SETUP")) {
        done = true;
        ret = dealSetup(hd, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "ANNOUNCE")) {
        done = true;
        ret = dealAnnounce(hd, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "RECORD")) {
        done = true;
        ret = dealRecord(hd, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "PLAY")) {
        done = true;
        ret = dealPlay(hd, req, rsp);
    } else if (TokenUtil::strCmp(&req.m_method, "PAUSE")) {
        done = true;
    } else if (TokenUtil::strCmp(&req.m_method, "TEARDOWN")) {
        done = true;
        ret = dealTeardown(hd, req, rsp);
    } else {
        done = false;
    }

    if (done) {
        rtspReply(hd, rsp);
        
        if (0 != ret) {
            //m_frame->closeData(hd);
        }
    }
    
    return done;
}

int RtspHandler::dealOption(int, RtspReq&, RtspRsp& rsp) {
    int ret = 0;
    
    HttpUtil::addEnumField(&rsp.m_head,
        ENUM_HTTP_PUBLIC, DEF_RTSP_PUBLIC_VAL);
    
    rsp.m_errcode = RTSP_STATUS_OK;
    return ret;
}

int RtspHandler::dealDescribe(int, RtspReq& req, RtspRsp& rsp) {
    int ret = 0;
    char path[MAX_PATH_SIZE] = {0};

    TokenUtil::copy(path, sizeof(path), &req.m_path);

    do { 
        if (!TokenUtil::strCmp(&req.m_accept, 
            DEF_RTSP_FIELD_CONTENT_TYPE_VAL, true)) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
        
        ret = m_resource->genSdp(&rsp.m_body, path);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_CONTENT_BASE, req.m_url);

        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_CONTENT_TYPE, 
            DEF_RTSP_FIELD_CONTENT_TYPE_VAL);
        
        rsp.m_errcode = RTSP_STATUS_OK;
        return 0;
    } while (false);
    
    return ret;
}

RtspCtx* RtspHandler::prepareSess(RtspReq& req) {
    RtspCtx* ctx = NULL;
    
    if (DEF_NULL_CHAR == req.m_sid[0]) {
        m_sess->genSess(req.m_sid, sizeof(req.m_sid));

        ctx = allocRtsp(); 
        MiscTool::strCpy(ctx->m_sid, req.m_sid, MAX_SESS_ID_LEN); 
        m_sess->addSess(ctx);
    } else {
        ctx = m_sess->findSess(req.m_sid);
    }

    return ctx;
}

int RtspHandler::dealSetup(int fd, RtspReq& req, RtspRsp& rsp) {
    RtspCtx* ctx = NULL;
    InstStream* inst = NULL;
    InstGroup* grp = NULL;
    FFStream* ff = NULL;
    int ret = 0;
    bool found = false;
    char cli_ip[DEF_IP_SIZE] = {0}; 
    char path[MAX_PATH_SIZE] = {0};

    TokenUtil::copy(path, sizeof(path), &req.m_path);
    
    do { 
        if (!req.m_transport.m_is_rtp_udp) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 
        
        ctx = prepareSess(req);
        if (NULL == ctx) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }

        found = existFF(ctx, path);
        if (found) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }

        grp = m_resource->findGrp(path);
        if (NULL != grp) {
            inst = grp->m_stream[0];
        } else { 
            inst = m_resource->findInst(path);
            if (NULL == inst) {
                rsp.m_errcode = RTSP_STATUS_SERVICE;
                break;
            }
        }
        
        m_frame->getAddr(fd, NULL, cli_ip, sizeof(cli_ip));

        ff = &ctx->m_stream[ctx->m_stream_cnt];
        ret = m_resource->setupFF(ff, inst, cli_ip, 
            req.m_transport.m_client_port_min,
            req.m_transport.m_client_port_max); 
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        ff->m_rtsp = ctx;
        ++ctx->m_stream_cnt;

        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        HttpUtil::addFormat(&rsp.m_head,
            "Transport: RTP/AVP/UDP;unicast;"
            "client_port=%d-%d;server_port=%d-%d\r\n",
            ff->m_addr.m_min_port,
            ff->m_addr.m_max_port,
            inst->m_stat.m_addr.m_min_port,
            inst->m_stat.m_addr.m_max_port);

        rsp.m_errcode = RTSP_STATUS_OK;
        return 0;
    } while (false);

    return -1;
}

int RtspHandler::dealPlay(int, RtspReq& req, RtspRsp& rsp) {
    RtspCtx* ctx = NULL;
    int ret = 0;
    bool bOk = false;
    char path[MAX_PATH_SIZE] = {0};

    TokenUtil::copy(path, sizeof(path), &req.m_path);

    do {
        ctx = m_sess->findSess(req.m_sid);
        if (NULL == ctx) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        bOk = activate(ctx, path, false);
        if (!bOk) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
        
        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

int RtspHandler::dealTeardown(int, RtspReq& req, RtspRsp& rsp) {
    RtspCtx* ctx = NULL;
    int ret = 0;
    bool bOk = false;
    char path[MAX_PATH_SIZE] = {0};

    TokenUtil::copy(path, sizeof(path), &req.m_path);

    do {
        ctx = m_sess->findSess(req.m_sid);
        if (NULL == ctx) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        bOk = removeFF(ctx, path);
        if (!bOk) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
      
        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

int RtspHandler::dealAnnounce(int fd, RtspReq& req, RtspRsp& rsp) {
    int ret = 0;
    bool bOk = false;
    SockAddr addr;
    SockName name;
    char cname[MAX_CNAME_SIZE] = {0};
    char path[MAX_PATH_SIZE] = {0};

    TokenUtil::copy(path, sizeof(path), &req.m_path);

    do {
        bOk = chkSdpHdr(req);
        if (!bOk) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            ret = -1;
            break;
        }

        SockTool::getLocalSock(fd, addr);
        SockTool::addr2IP(&name, &addr);

        m_sess->genSess(cname, MAX_CNAME_SIZE);
        
        ret = m_resource->addGrp(name.m_ip, path, cname, &req.m_body);
        if (0 != ret) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }

        rsp.m_errcode = RTSP_STATUS_OK;
        return 0;
    } while (false); 
    
    return ret;
}

int RtspHandler::dealRecord(int, RtspReq& req, RtspRsp& rsp) { 
    RtspCtx* ctx = NULL;
    int ret = 0;
    bool bOk = false;
    char path[MAX_PATH_SIZE] = {0};

    TokenUtil::copy(path, sizeof(path), &req.m_path);

    do {
        ctx = m_sess->findSess(req.m_sid);
        if (NULL == ctx) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        } 

        bOk = activate(ctx, path, true);
        if (!bOk) {
            rsp.m_errcode = RTSP_STATUS_SERVICE;
            break;
        }
        
        HttpUtil::addEnumField(&rsp.m_head,
            ENUM_HTTP_SESSION, ctx->m_sid);

        rsp.m_errcode = RTSP_STATUS_OK;
    } while (false);

    return ret;
}

bool RtspHandler::activate(RtspCtx* ctx, 
    const char path[], bool is_publisher) {
    FFStream* ff = NULL;
    InstStream* inst = NULL;
    bool bOk = false;

    for (int i=0; i<ctx->m_stream_cnt; ++i) {
        ff = &ctx->m_stream[i];
        
        if (ff->m_setup) {
            inst = ff->m_inst;
            
            if (RtpInstMng::match(path, inst)) {
                RtpInstMng::setPublisher(ff, is_publisher);
                RtpInstMng::activateFF(inst, ff); 

                if(!bOk) {
                    bOk = true;
                }
            }
        }
    }

    return bOk;
}

bool RtspHandler::deactivate(RtspCtx* ctx, const char path[]) {
    FFStream* ff = NULL;
    InstStream* inst = NULL;
    bool bOk = false;

    for (int i=0; i<ctx->m_stream_cnt; ++i) {
        ff = &ctx->m_stream[i];

        if (ff->m_setup) {
            inst = ff->m_inst;
            
            if (RtpInstMng::match(path, inst)) {
                RtpInstMng::deactivateFF(inst, ff);

                if(!bOk) {
                    bOk = true;
                }
            }
        }
    }

    return bOk;
}

bool RtspHandler::removeFF(RtspCtx* ctx, const char path[]) {
    FFStream* ff = NULL;
    InstStream* inst = NULL;
    bool bOk = false;

    for (int i=0; i<ctx->m_stream_cnt; ++i) {
        ff = &ctx->m_stream[i];

        if (ff->m_setup) {
            inst = ff->m_inst;
            
            if (RtpInstMng::match(path, inst)) {
                m_resource->delFF(ff); 
                ff->m_setup = false;

                if(!bOk) {
                    bOk = true;
                }
            }
        }
    }

    return bOk;
} 

bool RtspHandler::existFF(RtspCtx* ctx, const char path[]) {
    FFStream* ff = NULL;
    InstStream* inst = NULL;
    Token token(path);
    
    for (int i=0; i<ctx->m_stream_cnt; ++i) {
        ff = &ctx->m_stream[i];
        if (ff->m_setup) {
            inst = ff->m_inst;
            
            if (TokenUtil::strCmp(&token, inst->m_path)) {
                return true;
            }
        }
    }

    return false;
}

