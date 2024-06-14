#ifndef __RTSPCENTER_H__
#define __RTSPCENTER_H__
#include"ihttpdealer.h"
#include"rtspdata.h"


struct NodeMsg;
struct Token;
struct HttpFrameHd;
class SockFrame;
class HttpCenter;
class RtspRes;
class RtspSess;
class RtpCenter;
class SdpUtil;

class RtspHandler : public IHttpDealer {
public:
    RtspHandler(SockFrame* frame, HttpCenter* center);
    ~RtspHandler();
    
    virtual bool procFrameHd(int hd, NodeMsg* msg);
    
    RtspCtx* allocRtsp();
    void freeRtsp(RtspCtx*);
    
    void parseSession(RtspReq* req, Token* token); 
    bool chkRtpUdp(Token* token);
    bool parsePort(Token* token, int& port_min, int& port_max);
    
    void parseParam(RtspReq* req, Token* token);
    
    void parseTransport(RtspReq* req, Token* token);
    
    void parseAuthInfo(RtspReq* req, Token* token);
    
    void parseField(RtspReq* req, Token* key, Token* val);
    void parseFrame(RtspReq* req, HttpFrameHd* frame);
    
    bool dealRtspMsg(int hd, RtspReq& req, RtspRsp& rsp); 

    void rtspReply(int fd, RtspRsp& rsp);

private: 
    int dealOption(int hd, RtspReq& req, RtspRsp& rsp);
    
    int dealDescribe(int hd, RtspReq& req, RtspRsp& rsp);
    
    int dealSetup(int hd, RtspReq& req, RtspRsp& rsp);

    int dealPlay(int hd, RtspReq& req, RtspRsp& rsp);

    int dealTeardown(int hd, RtspReq& req, RtspRsp& rsp);

    int dealAnnounce(int hd, RtspReq& req, RtspRsp& rsp);

    int dealRecord(int hd, RtspReq& req, RtspRsp& rsp);

    bool chkSdpHdr(RtspReq& req);

    RtspCtx* prepareSess(RtspReq& req);

    bool activate(RtspCtx* ctx, const char path[],
        bool is_publisher);
    
    bool deactivate(RtspCtx* ctx, const char path[]);

    bool removeFF(RtspCtx* ctx, const char path[]);

    bool existFF(RtspCtx* ctx, const char path[]);

private:
    SockFrame* m_frame;
    HttpCenter* m_center;
    RtpCenter* m_rtp_center;
    RtspRes* m_resource;
    RtspSess* m_sess;
}; 


#endif

