#ifndef __DEMO_H__
#define __DEMO_H__
#include"ihttpdealer.h"


struct Cache;
struct HttpFrameHd;
class HttpCenter;

class TestDemo : public IHttpDealer {
public:
    TestDemo(HttpCenter* center);

    virtual int procFrameHd(int hd, HttpCtx* ctx, NodeMsg* msg);
    
    virtual int procFrameMid(int hd, HttpCtx* ctx, NodeMsg* msg);

    virtual HttpCtx* allocHttpEnv();
    virtual void freeHttpCtx(HttpCtx*);

    virtual bool bindCtx(int fd, HttpCtx*);

    void writeBody(unsigned seq, Cache* cache, 
        int size, bool truncated);

    void writeBytes(unsigned seq, const char* psz, 
        int size, bool truncated);

private:
    HttpCenter* m_center;
}; 

#endif

