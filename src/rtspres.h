#ifndef __RTSPRES_H__
#define __RTSPRES_H__
#include<map>
#include"rtspdata.h"


struct Token;
struct HttpCache;
struct SdpData;
class SdpUtil;

typedef std::map<typeStr, SdpData*> typeSdp;
typedef typeSdp::iterator itrSdp;
typedef typeSdp::const_iterator constItrSdp;

class RtspRes {
public:
    RtspRes();
    ~RtspRes();
    
    bool addRes(SdpData* sdp);
    void delRes(const Token& path);
    
    bool existRes(const Token& path);
    SdpData* findRes(const Token& path);
    SdpMedia* findMedia(const Token& path); 

    int prepareSDP(HttpCache* cache, const Token& path);

    int parseSDP(const Token& path, Token& text);

private:
    SdpUtil* m_util;
    typeSdp m_datas; 
};

#endif

