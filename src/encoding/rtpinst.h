#ifndef __RTPINST_H__
#define __RTPINST_H__
#include"rtpdata.h"
#include"rtpproto.h"
#include"shareheader.h"


class RtpInstMng {
public:
    static FFStream* findByRtp(InstStream* inst, const SockAddr* addr);
    static FFStream* findByRtcp(InstStream* inst, const SockAddr* addr);
    static const SockAddr* findBySsrc(InstStream* inst, uint_32 ssrc);
    static bool existAddr(InstStream* inst, uint_32 ssrc);

    static bool match(const char path[], InstStream* inst);

    static bool addSsrc(InstStream* inst, uint_32 ssrc,
        const SockAddr* addr);

    static void delSsrc(InstStream* inst, uint_32 ssrc) ;

    static uint_32 genSsrc(InstStream* inst);

    static void addFF(InstStream* inst, FFStream* ff);
    static void removeFF(InstStream* inst, FFStream* ff);

    static void setPublisher(FFStream* ff, bool is_publisher);

    static void activateFF(InstStream* inst, FFStream* ff); 
    static void deactivateFF(InstStream* inst, FFStream* ff);

    static bool validCname(const InstStat* stat);
    
    static bool validSeq(RecvStatis* statis, uint_16 seq);

    static bool chkRecv(const RecvStatis* statis, const ClockTime& ntp); 
    static bool chkSend(const SendStatis* statis, const ClockTime& ntp); 

    static uint_32 rescale(uint_64 a, uint_64 b, uint_64 c);

    static void getNtpTime(ClockTime& ntp);
    static uint_32 rtp2U32(const RtpTime& t);
    static uint_32 ntp2U32(const ClockTime& ntp);
    static uint_32 ntp2Unit(const SdpMedia* media, 
        const ClockTime& ntp);
};

ClockTime operator-(const ClockTime& t1, const ClockTime& t2);
ClockTime operator+(const ClockTime& t1, const ClockTime& t2); 
ClockTime& operator-=(ClockTime& t1, const ClockTime& t2);
ClockTime& operator+=(ClockTime& t1, const ClockTime& t2);

#endif

