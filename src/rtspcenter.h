#ifndef __RTSPCENTER_H__
#define __RTSPCENTER_H__
#include"ihttpdealer.h"
#include"rtspdata.h"


struct NodeMsg;
struct Token;
struct HttpFrameHd;
class HttpCenter;
class RtspRes;
class RtspSess;

class RtspHandler : public IHttpDealer {
public:
    RtspHandler(HttpCenter* center);
    ~RtspHandler();
    
    virtual int procFrameHd(int hd, HttpCtx* ctx, NodeMsg* msg);
    
    virtual int procFrameMid(int hd, HttpCtx* ctx, NodeMsg* msg);

    virtual HttpCtx* allocHttpEnv();
    virtual void freeHttpCtx(HttpCtx*);
    virtual bool bindCtx(int fd, HttpCtx*);
        
    void parseSession(RtspReq* req, Token* token); 
    bool chkRtpUdp(Token* token);
    bool parsePort(Token* token, int& port_min, int& port_max);
    
    void parseParam(RtspReq* req, Token* token);
    
    void parseTransport(RtspReq* req, Token* token);
    
    void parseAuthInfo(RtspReq* req, Token* token);
    
    void parseField(RtspReq* req, Token* key, Token* val);
    void parseFrame(RtspReq* req, HttpFrameHd* frame);
    
    int dealRtspMsg(int hd, RtspCtx* ctx,
        HttpFrameHd* frame, RtspRsp& rsp, bool& done); 

    void rtspReply(int fd, RtspCtx* ctx, RtspRsp& rsp);

private: 
    int dealOption(RtspCtx*, RtspReq& req,
        RtspRsp& rsp);
    
    int dealDescribe(RtspCtx*, RtspReq& req, RtspRsp& rsp);
    
    int dealSetup(RtspCtx*, RtspReq& req, RtspRsp& rsp);

    int dealPlay(RtspCtx* ctx, RtspReq& req, RtspRsp& rsp);

    int dealTeardown(RtspCtx* ctx, RtspReq& req, RtspRsp& rsp);

    int dealAnnounce(RtspCtx* ctx, RtspReq& req, RtspRsp& rsp);

    int dealRecord(RtspCtx* ctx, RtspReq& req, RtspRsp& rsp);

    bool chkSid(RtspCtx* ctx, RtspReq& req);
    
    void resetRtsp(RtspCtx* rtsp); 

    int parseSdp(RtspCtx* ctx, RtspReq& req);

    int prepareSess(RtspCtx* ctx, RtspReq& req);

    FFStream* prepareStream(RtspCtx* ctx, RtspReq& req);

    int startStream(RtspCtx* ctx, const Token& path);
    int stopStream(RtspCtx* ctx, const Token& path);

private:
    HttpCenter* m_center;
    RtspRes* m_resource;
    RtspSess* m_sess;
    IHttpDealer* m_dealer;
}; 


#endif

