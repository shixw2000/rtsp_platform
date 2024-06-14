#ifndef __DEMO_H__
#define __DEMO_H__
#include"ihttpdealer.h"


struct Cache;
struct HttpFrameHd;
class HttpCenter;
class SockFrame;

class TestDemo : public IHttpDealer {
public:
    TestDemo(SockFrame* frame, HttpCenter* center);

    virtual bool procFrameHd(int hd, NodeMsg* msg);

    bool chkAlive(const HttpFrameHd* body);
    void rspHttpFrame(int hd, int ver,
        bool isAlive, Cache* body, int blen);
    
    void writeBody(unsigned seq, Cache* cache, 
        int size, bool truncated);

    void writeBytes(unsigned seq, const char* psz, 
        int size, bool truncated);

private:
    SockFrame* m_frame;
    HttpCenter* m_center;
}; 

#endif

