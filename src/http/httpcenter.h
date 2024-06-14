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
    static const int MAX_DEALER_SIZE = 8;
    
public:
    HttpCenter(SockFrame* frame);
    ~HttpCenter();

    int init();
    void finish();

    void addDealer(IHttpDealer* dealer);

    void printFrameHd(NodeMsg* msg);

    int parseData(int fd, const char* buf, int size);

    HttpCtx* creatSock();

    void onListenerClose(int);
    
    void onClose(int hd);

    int process(int, NodeMsg* msg);

    void onConnFail(long extra, int);
    
    int onConnOK(int hd);

    int dispatch(int fd, NodeMsg* msg);

    void printHttpSendMsg(NodeMsg* msg);
    
    int send(int fd, NodeMsg* msg);
    
    NodeMsg* creatFrameHd(HttpCtx* ctx, bool isEnd);

    void resetCtx(HttpCtx* ctx); 

    void getLine(const HttpFrameHd* frame,
        int line, Token* key, Token* val);

    bool findEnumField(const HttpFrameHd* frame,
        EnumHttpField enType, Token* val);

    bool findField(const HttpFrameHd* frame,
        const Token& name, Token* val);

private:
    HttpCtx* allocHttp();
    void freeHttp(HttpCtx*);

    int procFrameHd(int hd, NodeMsg* msg);

private:
    SockFrame* m_frame;
    HttpTool* m_tool;
    IHttpDealer* m_dealer[MAX_DEALER_SIZE];
};


#endif

