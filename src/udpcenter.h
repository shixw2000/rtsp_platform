#ifndef __UDPCENTER_H__
#define __UDPCENTER_H__
#include"isockapi.h"


class RtpCenter;

class RtpSvc : public ISockBase {
public:
    RtpSvc(RtpCenter* center);
    
    virtual int parseData(int fd, 
        const char* buf, int size, 
        const SockAddr* addr);
    
    virtual int process(int hd, NodeMsg* msg);
    
    virtual void onClose(int hd);

private:
    RtpCenter* m_center;
};

class RtcpSvc : public ISockBase {
public:
    RtcpSvc(RtpCenter* center);
    
    virtual int parseData(int fd, 
        const char* buf, int size, 
        const SockAddr* addr);
    
    virtual int process(int hd, NodeMsg* msg);
    
    virtual void onClose(int hd);

private:
    RtpCenter* m_center;
};

#endif

