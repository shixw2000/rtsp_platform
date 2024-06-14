#ifndef __RTPUTIL_H__
#define __RTPUTIL_H__
#include"rtpproto.h"
#include"rtpdata.h"


class Decoder;
class Encoder;

class RtpTool {
public: 
    static bool isRtpHeader(const char* data, int size);
    static bool isRtcpHeader(const char* data, int size);

    static bool isVerValid(uint_8 ver);
    static bool isRtcpPt(uint_8 pt); 
    
    static bool parseRtpPkg(RtpPkg* rtp, Decoder* dec);
    static bool parseRtcpPkg(RtcpPkg* rtcp, Decoder* dec);
    
    static bool parseSenderInfo(RtcpSenderInfo* sender, Decoder* dec);
    static bool parseReportBlk(RtcpReportBlk* blk, Decoder* dec);
    static bool parseSdesChunk(SdesChunk* sdes, Decoder* dec);

    static bool parseSdes(RtcpSdes* sdes, uint_8 rc, Decoder* dec); 
    static bool parseSR(RtcpSR* sr, uint_8 rc, Decoder* dec);
    static bool parseRR(RtcpRR* rr, uint_8 rc, Decoder* dec);
    static bool parseByte(RtcpBye* bye, uint_8 rc, Decoder* dec);
    static bool parseApp(RtcpApp* app, Decoder* dec); 

    static bool genRtpHeader(Encoder* enc, 
        uint_32 ssrc, uint_32 ts, uint_16 seq, 
        uint_8 payload, bool isEnd);
    
    static bool genRtcpHeader(Encoder* enc, 
        uint_8 rc, uint_8 pt);
    
    static bool genSender(FFStream* ff, 
        Encoder* enc, const ClockTime& ntp);
    
    static bool genReport(FFStream* ff,
        Encoder* enc, const ClockTime& ntp); 

    static bool genChunk(Encoder* enc, uint_32 ssrc,
        uint_8 type, const AVal* desc);
    
    static bool genBye(Encoder* enc, uint_32 ssrc,
        const AVal* reason);

    static void printRtpPkg(const char prompt[],
        int pkg_size, int blen, const RtpHdr* hdr);

    static void printRtcpPkg(const char prompt[],
        int pkg_size, int blen, const RtcpHdr* hdr);

    static void printSR(const char prompt[],
        uint_32 ssrc, const RtcpSenderInfo* sender);

    static void printRR(const char prompt[],
        uint_32 ssrc, const RtcpReportBlk* report);

    static void printSdes(const char prompt[],
        uint_32 ssrc, const SdesItem* item);
    
    static bool finishRtcp(Encoder* enc, int beg); 
};

#endif

