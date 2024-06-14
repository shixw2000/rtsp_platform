#include<linux/in.h>
#include<cstring>
#include<cstdio>
#include"rtspres.h"
#include"httpdata.h"
#include"rtspdata.h"
#include"shareheader.h"
#include"tokenutil.h"
#include"rtspsdp.h"
#include"misc.h"
#include"socktool.h"
#include"bitset.h"
#include"rtpcenter.h"
#include"sockframe.h"
#include"cache.h"
#include"rtpinst.h"
#include"rtputil.h"


bool StrCmp::operator() (const char* s1,
    const char* s2) const {
    return strcmp(s1, s2) < 0;
}

bool AddrCmp::operator() (const SockAddr* addr1,
    const SockAddr* addr2) const {
    struct sockaddr_in* sock1 = (struct sockaddr_in*)addr1->m_addr;
    struct sockaddr_in* sock2 = (struct sockaddr_in*)addr2->m_addr;

    if (sock1->sin_port == sock2->sin_port) {
        return sock1->sin_addr.s_addr <
            sock2->sin_addr.s_addr;
    } else {
        return sock1->sin_port < sock2->sin_port;
    }
}

bool AddrEqu::operator()(const SockAddr* addr1,
    const SockAddr* addr2) const {
    struct sockaddr_in* sock1 = (struct sockaddr_in*)addr1->m_addr;
    struct sockaddr_in* sock2 = (struct sockaddr_in*)addr2->m_addr;

    return sock1->sin_port == sock2->sin_port &&
        sock1->sin_addr.s_addr == sock2->sin_addr.s_addr;
}

RtspRes::RtspRes(SockFrame* frame, RtpCenter* rtp_center) 
    : m_frame(frame), m_rtp_center(rtp_center) {
    m_util = new SdpUtil;
    m_bitSet = new BitSet(0x8000, 0xf000);
}

RtspRes::~RtspRes() {
    InstGroup* grp = NULL;
    
    for (typeItr itr=m_datas.begin(); itr != m_datas.end(); ++itr) {
        grp = itr->second;

        freeGrp(grp);
    }

    m_datas.clear();
}

bool RtspRes::add(InstGroup* grp) {
    typeConstItr itr; 

    if (NULL != grp && !TokenUtil::isNULL(grp->m_path[0])) { 
        itr = m_datas.find(grp->m_path);
        if (m_datas.end() == itr) {
            m_datas[ grp->m_path ] = grp;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void RtspRes::del(const char path[]) {
    typeItr itr; 

    itr = m_datas.find(path);
    if (m_datas.end() != itr) {
        m_datas.erase(itr);
    } 
}

bool RtspRes::exist(const char path[]) const {
    typeConstItr itr; 
        
    itr = m_datas.find(path);
    if (m_datas.end() != itr) {
        return true;
    } else { 
        return false;
    } 
}

InstGroup* RtspRes::findGrp(const char path[]) {
    typeConstItr itr; 
        
    itr = m_datas.find(path);
    if (m_datas.end() != itr) {
        return itr->second;
    } else { 
        return NULL;
    } 
}

InstStream* RtspRes::findInst(const char path[]) {
    InstGroup* grp = NULL;
    InstStream* inst = NULL;
    Token token(path);

    for (typeConstItr itr=m_datas.begin();
        itr != m_datas.end(); ++itr) {
        grp = itr->second;
        for (int i=0; i<grp->m_stream_cnt; ++i) {
            inst = grp->m_stream[i];
            if (TokenUtil::strCmp(&token, inst->m_path, false)) {
                return inst;
            }
        }
    }

    return NULL;
}

InstGroup* RtspRes::creatGrp() {
    InstGroup* grp = NULL;

    grp = (InstGroup*)CacheUtil::mallocAlign(sizeof(InstGroup)); 
    MiscTool::bzero(grp, sizeof(InstGroup));
    
    return grp;
}

void RtspRes::freeGrp(InstGroup* grp) {
    if (NULL != grp) { 
        if (NULL != grp->m_sdp) {
            m_util->freeSDP(grp->m_sdp);
            grp->m_sdp = NULL;
        }

        for (int i=0; i<grp->m_stream_cnt; ++i) {
            if (NULL != grp->m_stream[i]) {
                freeInst(grp->m_stream[i]);
                grp->m_stream[i] = NULL;
            }
            
        }

        CacheUtil::freeAlign(grp);
    }
}

InstStream* RtspRes::creatInst() {
    InstStream* stream = NULL;

    stream = new InstStream;
    MiscTool::bzero(&stream->m_stat, sizeof(stream->m_stat));
    MiscTool::bzero(stream->m_path, sizeof(stream->m_path));

    initHRoot(&stream->m_publisher_list);
    initHRoot(&stream->m_player_list);

    stream->m_grp = NULL;
    stream->m_media = NULL;

    return stream;
} 

void RtspRes::freeInst(InstStream* inst) {
    InstStat* stat = NULL;
    
    if (NULL != inst) { 
        stat = &inst->m_stat;
        if (stat->m_setup) {
            m_frame->closeData(stat->m_addr.m_rtp_fd);
            m_frame->closeData(stat->m_addr.m_rtcp_fd);

            stat->m_setup = false;
        }
        
        delete inst;
    }
}

int RtspRes::addGrp(const char ip[], const char path[],
    const char cname[], Token* txt) {
    InstGroup* grp = NULL;
    SdpData* sdp = NULL;
    int ret = 0;
    bool bOk = false;

    bOk = exist(path);
    if (bOk) {
        /* already exists */
        return 0;
    }
    
    do {
        grp = creatGrp();
        if (NULL == grp) {
            ret = -1;
            break;
        }
        
        sdp = m_util->creatSDP();
        if (NULL == sdp) {
            ret = -1;
            break;
        }

        grp->m_sdp = sdp;
        MiscTool::strCpy(grp->m_path, path, sizeof(grp->m_path));
    
        ret = m_util->parseSDP(sdp, txt);
        if (0 != ret) {
            break;
        }

        ret = prepareInst(grp, sdp);
        if (0 != ret) {
            break;
        }

        ret = setupGrp(grp, ip, cname);
        if (0 != ret) {
            break;
        }
    
        add(grp);
        return ret;
    } while (false);

    if (NULL != grp) {
        freeGrp(grp);
    }

    return ret;
}

void RtspRes::delGrp(const char path[]) {
    InstGroup* grp = NULL;

    grp = findGrp(path);
    if (NULL != grp) {
        del(path);

        freeGrp(grp);
    } 
}

int RtspRes::prepareInst(InstGroup* grp, SdpData* sdp) {
    const SdpMedia* media = NULL;
    InstStream* inst = NULL;
    int len = 0; 
    
    for (int i=0; i<sdp->m_media_cnt; ++i) {
        media = sdp->m_media[i];
        
        inst = creatInst();
        if (NULL != inst) {
            inst->m_grp = grp;
            inst->m_media = media; 
            
            len = MiscTool::strCpy(inst->m_path, grp->m_path,
                sizeof(inst->m_path));
            inst->m_path[len++] = DEF_URL_SEP_CHAR;
            MiscTool::strCpy(&inst->m_path[len], media->m_path,
                sizeof(inst->m_path) - len);
            
            grp->m_stream[grp->m_stream_cnt++] = inst;
        } else {
            return -1;
        } 
    }

    return 0;
}

int RtspRes::genSdp(HttpCache* cache, const char path[]) {
    InstGroup* grp = NULL;
    int ret = 0;

    do { 
        grp = findGrp(path);
        if (NULL == grp) {
            ret = -1;
            break;
        } 

        ret = m_util->genSDP(cache, grp->m_sdp);
        if (0 != ret) {
            break;
        }
    } while (false);

    return ret;
}

int RtspRes::bindFF(FFStream* ff, InstStream* inst) {
    TransAddr* addr = &ff->m_addr; 
    SockName name; 

    if (inst->m_stat.m_setup) { 
        SockTool::assign(name, addr->m_ip, addr->m_min_port); 
        SockTool::ip2Addr(&addr->m_rtp, &name);

        SockTool::assign(name, addr->m_ip, addr->m_max_port); 
        SockTool::ip2Addr(&addr->m_rtcp, &name); 

        addr->m_rtp_fd = inst->m_stat.m_addr.m_rtp_fd;
        addr->m_rtcp_fd = inst->m_stat.m_addr.m_rtcp_fd;

        ff->m_inst = inst; 
        return 0;
    } else {
        return -1;
    }
}

void RtspRes::delFF(FFStream* ff) {
    if (NULL != ff && NULL != ff->m_inst) {
        RtpInstMng::removeFF(ff->m_inst, ff);

        ff->m_inst = NULL;
    }
} 

int RtspRes::setupGrp(InstGroup* grp,
    const char ip[], const char cname[]) {
    InstStream* inst = NULL;
    InstStat* stat = NULL;
    int ret = 0;

    for (int i=0; i<grp->m_stream_cnt; ++i) {
        inst = grp->m_stream[i];
        stat = &inst->m_stat; 

        ret = bindInst(inst, ip);
        if (0 != ret) {
            break;
        }

        setupInst(inst, cname);

        m_rtp_center->regRtp(stat->m_addr.m_rtp_fd, inst); 
        m_rtp_center->regRtcp(stat->m_addr.m_rtcp_fd, inst);
    }

    return ret;
}

void RtspRes::setupInst(InstStream* inst, const char cname[]) {
    InstStat* stat = &inst->m_stat; 
    
    MiscTool::strCpy(stat->m_sdes.m_cname, cname, MAX_CNAME_SIZE); 

    MiscTool::getRand(&stat->m_base_seq, sizeof(stat->m_base_seq));
    stat->m_seq = stat->m_base_seq;

    MiscTool::getRand(&stat->m_base_unit, sizeof(stat->m_base_unit));
    RtpInstMng::getNtpTime(stat->m_first_ntp);

    stat->m_ssrc = RtpInstMng::genSsrc(inst);
    RtpInstMng::addSsrc(inst, stat->m_ssrc, &stat->m_addr.m_rtp); 
    stat->m_setup = true;
}

int RtspRes::setupFF(FFStream* ff, InstStream* inst,
    const char ip[], int min_port, int max_port) {
    TransAddr* addr = &ff->m_addr;
    int ret = 0;

    MiscTool::bzero(ff, sizeof(*ff));

    MiscTool::strCpy(addr->m_ip, ip, sizeof(addr->m_ip));
    addr->m_min_port = min_port;
    addr->m_max_port = max_port;

    ret = bindFF(ff, inst);
    if (0 != ret) {
        return ret;
    }

    ff->m_setup = true;
    RtpInstMng::addFF(inst, ff);
    return ret;
}

int RtspRes::creatUniUdp(int* pfd,
    const char ip[], int port) {
    int ret = 0;
    SockName name;

    SockTool::assign(name, ip, port);
    ret = SockTool::openUdpName(pfd, name);
    return ret;
}
    
int RtspRes::creatMultiUdp(int* pfd,
    const char my_ip[], const char multi_ip[], 
    int port) {
    int ret = 0;

    ret = SockTool::openMultiUdp(pfd, port,
        my_ip, multi_ip);
    return ret;
} 

int RtspRes::bindInst(InstStream* inst, const char ip[]) {
    TransAddr* addr = &inst->m_stat.m_addr;
    int ret = 0;
    int rtpPort = 0;
    int fd[2] = {0};
    SockName name;

    rtpPort = m_bitSet->nextPort();
    while (0 < rtpPort) {
        ret = creatUniUdp(&fd[0], ip, rtpPort);
        if (0 == ret) {
            ret = creatUniUdp(&fd[1], ip, rtpPort + 1);
            if (0 == ret) {
                break;
            } else {
                SockTool::closeSock(fd[0]);
            }
        }

        rtpPort = m_bitSet->nextPort();
    }

    if (0 < rtpPort) { 
        addr->m_rtp_fd = fd[0];
        addr->m_rtcp_fd = fd[1];
        
        addr->m_min_port = rtpPort;
        addr->m_max_port = rtpPort + 1;

        SockTool::assign(name, ip, rtpPort);
        SockTool::ip2Addr(&addr->m_rtp, &name);

        SockTool::assign(name, ip, rtpPort + 1);
        SockTool::ip2Addr(&addr->m_rtcp, &name);
        
        MiscTool::strCpy(addr->m_ip, ip, sizeof(addr->m_ip));
        return 0;
    } else {
        return -1;
    }
} 

