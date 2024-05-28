#ifndef __IHTTPDEALER_H__
#define __IHTTPDEALER_H__


struct NodeMsg;
struct HttpCtx;

class IHttpDealer {
public:
    virtual ~IHttpDealer() {}

    virtual int procFrameHd(int hd, HttpCtx* ctx, NodeMsg* msg) = 0;
    
    virtual int procFrameMid(int hd, HttpCtx* ctx, NodeMsg* msg) = 0;

    virtual HttpCtx* allocHttpEnv() = 0;
    virtual void freeHttpCtx(HttpCtx*) = 0;
    
    virtual bool bindCtx(int fd, HttpCtx*) = 0;
};

#endif

