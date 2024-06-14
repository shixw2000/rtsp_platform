#include"udpcenter.h"
#include"rtpcenter.h"


RtpSvc::RtpSvc(RtpCenter* center) 
    : m_center(center) {
}

int RtpSvc::parseData(int fd, 
    const char* buf, int size, 
    const SockAddr* addr) {
    int ret = 0;

    ret = m_center->parseRtp(fd, buf, size, addr);
    return ret;
}

int RtpSvc::process(int hd, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->procRtp(hd, msg);
    if (0 != ret) {
        ret = 0;
    }
    
    return ret;
}

void RtpSvc::onClose(int hd) {
    m_center->closeSvr(hd);
}

RtcpSvc::RtcpSvc(RtpCenter* center) 
    : m_center(center) {
}

int RtcpSvc::parseData(int fd, 
    const char* buf, int size, 
    const SockAddr* addr) {
    int ret = 0;

    ret = m_center->parseRtcp(fd, buf, size, addr);
    return ret;
}

int RtcpSvc::process(int hd, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->procRtcp(hd, msg);
    if (0 != ret) {
        ret = 0;
    }
    
    return ret;
}

void RtcpSvc::onClose(int hd) {
    m_center->closeSvr(hd);
}
