#ifndef __RTSPRES_H__
#define __RTSPRES_H__
#include<map>
#include"rtpdata.h"


struct Token;
struct HttpCache;
class SockFrame;
class BitSet;
class RtpCenter;
class SdpUtil;

class RtspRes {
    typedef typeMapInst::iterator typeItr;
    typedef typeMapInst::const_iterator typeConstItr;

public:
    RtspRes(SockFrame* frame, RtpCenter* rtp_center);
    ~RtspRes(); 

    int addGrp(const char ip[], const char path[],
        const char cname[], Token* token);
    
    int genSdp(HttpCache* cache, const char path[]); 

    bool exist(const char path[]) const;
    InstGroup* findGrp(const char path[]);
    InstStream* findInst(const char path[]);
    void delGrp(const char path[]); 

    int bindFF(FFStream* ff, InstStream* inst);

    void delFF(FFStream* ff); 
     
    int setupGrp(InstGroup* grp, const char ip[],
        const char cname[]);

    void setupInst(InstStream* inst, const char cname[]);

    int setupFF(FFStream* ff, InstStream* inst,
        const char ip[], int min_port, int max_port);
    
private: 
    int bindInst(InstStream* inst, const char ip[]);
    
    int creatUniUdp(int* pfd, const char ip[], int port);
    int creatMultiUdp(int* pfd, const char my_ip[],
        const char multi_ip[], int port); 
    
    int prepareInst(InstGroup* grp, SdpData* sdp); 

    bool add(InstGroup* grp);
    void del(const char path[]);

    InstStream* creatInst(); 
    void freeInst(InstStream* inst);
    
    InstGroup* creatGrp();
    void freeGrp(InstGroup* grp); 
    
private:
    SdpUtil* m_util;
    BitSet* m_bitSet;
    SockFrame* m_frame;
    RtpCenter* m_rtp_center;
    typeMapInst m_datas; 
};

#endif

