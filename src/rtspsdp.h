#ifndef __RTSPSDP_H__
#define __RTSPSDP_H__
#include"rtspdata.h"


class SdpUtil {
public: 
    SdpData* creatSDP(const Token& path);
    void freeSDP(SdpData* stream);

    SdpMedia* creatMedia();
    void freeMedia(SdpMedia* media);

    int findMedia(SdpData* sdp, const Token& path);
    
    int prepareSDP(HttpCache* cache, const SdpData* sdp);

    int parseSDP(SdpData* sdp, Token* text);
    
private:
    int getPayloadType(const SdpMedia* media);

    int getProfileLevel(const SdpMedia* media);
    
    int writeSdpAddr(HttpCache* cache, const char ip[], int ttl);
    int writeSdpHeader(HttpCache* cache, const SdpHeader* header);
    
    int writeMediaType(HttpCache* cache, 
        const SdpMedia* media);
    
    int writeMediaAttr(HttpCache* cache, 
        const SdpMedia* media);
    
    int writeSdpMedia(HttpCache* cache, 
        const SdpMedia* media);

    void parseSdpLine(SdpData* sdp, 
        SdpParseStat& stat, char letter, Token& val);

    void parseConn(SdpData* sdp, 
        SdpParseStat& stat, Token& txt);

    void parseBitRate(SdpData* sdp, Token& txt);
    void parseTitle(SdpHeader* header, Token& txt);
    void parseDesc(SdpHeader* header, Token& txt);
    void parseAttr(SdpData* sdp, 
        SdpParseStat& stat, Token& txt);

    void parseControl(SdpData* sdp, 
        SdpParseStat& stat, Token& txt);

    void parseRtpmap(SdpData* sdp, 
        SdpParseStat& stat, Token& txt);

    void parseFmtp(SdpData* sdp, 
        SdpParseStat& stat, Token& txt);

    void parseFrameSize(SdpData* sdp, 
        SdpParseStat&, Token& txt);

    void postMedia(SdpMedia* media);
    
    void parseMedia(SdpData* sdp, 
        SdpParseStat& stat, Token& txt);

    bool getCodecID(AVCodec* codec, const Token& name);
};

#endif

