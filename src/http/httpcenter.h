#ifndef __HTTPCENTER_H__
#define __HTTPCENTER_H__
#include"httpdata.h"
#include"isockmsg.h"


struct Cache;
struct NodeMsg; 
class SockFrame;
class HttpTool;
class IHttpDealer;

class HttpCenter {
public:
    HttpCenter(SockFrame* frame);
    ~HttpCenter();

    int init();
    void finish();

    void setDealer(IHttpDealer* dealer);

    void printFrameHd(NodeMsg* msg);
    void printFrameMid(NodeMsg* msg);

    int parseData(int fd, const char* buf, int size);

    HttpCtx* creatSvrSock(int fd);
    HttpCtx* creatCliSock();

    void onListenerClose(int);
    
    void onClose(int hd);

    int process(int, NodeMsg* msg);

    void onConnFail(long extra, int);
    
    int onConnOK(int hd);

    int dispatch(int fd, NodeMsg* msg);

    int send(int fd, NodeMsg* msg);
    
    NodeMsg* creatFrameHd(HttpCtx* ctx, bool isEnd);

    NodeMsg* creatFrameMid(HttpCtx* ctx, bool isEnd); 

    void resetCtx(HttpCtx* ctx); 

    void getLine(const HttpFrameHd* frame,
        int line, Token* key, Token* val);

    bool findEnumField(const HttpFrameHd* frame,
        EnumHttpField enType, Token* val);

    bool findField(const HttpFrameHd* frame,
        const Token& name, Token* val);

private:
    HttpCtx* allocHttpEnv();
    void freeHttpCtx(HttpCtx*);

    int procFrameHd(int hd, HttpCtx* ctx, NodeMsg* msg);
    int procFrameMid(int hd, HttpCtx* ctx, NodeMsg* msg);

private:
    SockFrame* m_frame;
    HttpTool* m_tool;
    IHttpDealer* m_dealer;
};


#endif

