#ifndef __RTPCENTER_H__
#define __RTPCENTER_H__
#include"shareheader.h"
#include"rtpproto.h"
#include"rtpdata.h"


struct RtspCtx;
class SockFrame;
class ISockBase;
class Encoder;
class Decoder;

class RtpCenter {
public:
    RtpCenter(SockFrame* frame); 
    ~RtpCenter(); 

    int regRtp(int fd, InstStream* stream);
    int regRtcp(int fd, InstStream* stream);

    bool validSsrc(FFStream* ff, uint_32 ssrc);

    int procRtp(int hd, NodeMsg* msg);

    int parseRtp(int fd, const char* buf, int size,
        const SockAddr* addr);

    int procRtcp(int hd, NodeMsg* msg);

    int parseRtcp(int fd, const char* buf, int size,
        const SockAddr* addr);

    void closeSvr(int fd);

    void publishRtp(InstStream* inst, 
        const ClockTime& ntp, NodeMsg* msg);

    void handleRtp(InstStream* inst, 
        const ClockTime& ntp, const RtpPkg* pkg,
        Cache* cache);
    
    int procRtcpBody(FFStream* ff, const ClockTime& ntp, 
        const RtcpPkg* pkg);

    int procSR(FFStream* ff, const ClockTime& ntp, 
        const RtcpPkg* pkg);

    int procRR(FFStream* ff, const RtcpPkg* pkg);

    int procSdes(FFStream* ff, const RtcpPkg* pkg);

    int procBye(FFStream* ff, const RtcpPkg* pkg);

    int procApp(FFStream* ff, const RtcpPkg* pkg);

    bool genSR(FFStream* ff, Encoder* enc,
        const ClockTime& ntp);
    
    bool genRR(FFStream* ff, Encoder* enc,
        const ClockTime& ntp);
    
    bool genSdes(Encoder* enc, const InstStat* stat);

    void procRtpData(InstStream* inst, FFStream* ff, 
        const ClockTime& ntp, const RtpPkg* pkg,
        Cache* cache);

    int calcSdesLen(const char cname[]);
    
    NodeMsg* genRtpMsg(InstStream* inst, 
        const ClockTime& ntp, uint_8 payload, bool isEnd,
        Cache* cache, int size, int pos);
    
    void sendSR(FFStream* ff, const ClockTime& ntp);
    void sendRR(FFStream* ff, const ClockTime& ntp);
    void sendRtp(FFStream* ff, const ClockTime& ntp, NodeMsg* msg);

    void sendRtpMsg(FFStream* ff, NodeMsg* msg);
    void sendRtcpMsg(FFStream* ff, NodeMsg* msg);

    void updateRtpSend(FFStream* ff, const ClockTime& ntp);
    
    void updateRtpRecv(FFStream* ff, 
        const ClockTime& ntp, const RtpPkg* pkg);

private:
    SockFrame* m_frame;
    ISockBase* m_rtp_base;
    ISockBase* m_rtcp_base; 
};

#endif

