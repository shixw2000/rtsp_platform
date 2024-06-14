#include"rtpcenter.h"
#include"rtputil.h"
#include"msgcenter.h"
#include"rtpdata.h"
#include"sockframe.h"
#include"misc.h"
#include"encoder.h"
#include"socktool.h"
#include"udpcenter.h" 
#include"rtspdata.h"
#include"cache.h"
#include"rtpinst.h"


RtpCenter::RtpCenter(SockFrame* frame) {
    m_frame = frame; 
    
    m_rtcp_base = new RtcpSvc(this);
    m_rtp_base = new RtpSvc(this);
}

RtpCenter::~RtpCenter() { 
    if (NULL != m_rtp_base) {
        delete m_rtp_base;
    }

    if (NULL != m_rtcp_base) {
        delete m_rtcp_base;
    }
} 

int RtpCenter::regRtp(int fd, InstStream* stream) {
    int ret = 0;

    ret = m_frame->regUdp(fd, m_rtp_base, (long)stream);
    return ret;
}

int RtpCenter::regRtcp(int fd, InstStream* stream) {
    int ret = 0;

    ret = m_frame->regUdp(fd, m_rtcp_base, (long)stream);
    return ret;
}

int RtpCenter::procRtp(int hd, NodeMsg* msg) { 
    Cache* cache = NULL;
    const char* origin = NULL;
    InstStream* inst = NULL;
    FFStream* ff = NULL;
    const SockAddr* addr = NULL;
    int ret = 0;
    int oSize = 0; 
    bool bOk = false;
    RtpPkg pkg;
    Decoder dec;
    SockName name; 
    ClockTime ntp;

    inst = (InstStream*)m_frame->getExtra(hd);
    addr = MsgCenter::getUdpAddr(msg); 
    SockTool::addr2IP(&name, addr);

    RtpInstMng::getNtpTime(ntp);

    ff = RtpInstMng::findByRtp(inst, addr);
    if (NULL == ff || !ff->m_is_publisher) { 
        LOG_INFO("proc_rtp| addr=%s:%d|"
            " msg=unreg rtp publisher|",
            name.m_ip, name.m_port);

        return -1;
    }

    origin = MsgCenter::getCurr(msg);
    oSize = MsgCenter::getLeft(msg);
    dec.set(origin, oSize);

    bOk = RtpTool::parseRtpPkg(&pkg, &dec);
    if (!bOk) {
        return -1;
    } 

    RtpTool::printRtpPkg("proc_recv", pkg.m_pkg_size,
        pkg.m_payload_size, &pkg.m_hdr);

    bOk = validSsrc(ff, pkg.m_hdr.m_ssrc);
    if (!bOk) {
        return -1;
    }

    cache = MsgCenter::getCache(msg);
    procRtpData(inst, ff, ntp, &pkg, cache); 
    
    return ret;
}

int RtpCenter::parseRtp(int fd, const char* buf, int size,
    const SockAddr* addr) {
    NodeMsg* pMsg = NULL;
    char* psz = NULL; 
    int ret = 0;
    bool bOk = false;
    SockName name;

    do {
        bOk = RtpTool::isRtpHeader(buf, size);
        if (!bOk) {
            SockTool::addr2IP(&name, addr);
            
            LOG_INFO("read_rtp| fd=%d| size=%d|"
                " ip=%s:%d|"
                " msg=read invalid rtp header",
                fd, size, name.m_ip, name.m_port);
            break;
        }

        pMsg = MsgCenter::allocUdpMsg(size);
        MsgCenter::setUdpAddr(pMsg, *addr);
        
        psz = MsgCenter::getMsg(pMsg);

        MiscTool::bcopy(psz, buf, size);
        ret = m_frame->dispatch(fd, pMsg);
        if (0 != ret) {
            break;
        }
    } while (false);
    
    return 0;
}

int RtpCenter::procRtcp(int hd, NodeMsg* msg) {
    InstStream* inst = NULL;
    FFStream* ff = NULL;
    const SockAddr* addr = NULL;
    const char* origin = NULL;
    int ret = 0;
    int oSize = 0;
    bool bOk = false;
    bool bfirst = true;
    RtcpPkg pkg;
    Decoder dec;
    SockName name;
    ClockTime ntp;

    inst = (InstStream*)m_frame->getExtra(hd);
    addr = MsgCenter::getUdpAddr(msg); 
    SockTool::addr2IP(&name, addr);

    RtpInstMng::getNtpTime(ntp);

    ff = RtpInstMng::findByRtcp(inst, addr);
    if (NULL == ff) { 
        LOG_INFO("proc_rtcp| addr=%s:%d|"
            " msg=unreg rtcp address|",
            name.m_ip, name.m_port);

        return -1;
    } 

    origin = MsgCenter::getCurr(msg);
    oSize = MsgCenter::getLeft(msg);

    dec.set(origin, oSize); 
    while (!dec.isEnd()) {
        bOk = RtpTool::parseRtcpPkg(&pkg, &dec);
        if (!bOk) {
            return -1;
        }

        if (bfirst) { 
            bfirst = false;

            if (ENUM_RTCP_PT_RR != pkg.m_hdr.m_pt &&
                ENUM_RTCP_PT_SR != pkg.m_hdr.m_pt) {
                LOG_INFO("proc_rtcp| payload=%d|"
                    " msg=invalid rtcp|",
                    pkg.m_hdr.m_pt);
                return -1;
            }
        }

        procRtcpBody(ff, ntp, &pkg);
    }

    return ret;
}

int RtpCenter::parseRtcp(int fd, const char* buf, int size,
    const SockAddr* addr) {
    NodeMsg* pMsg = NULL;
    char* psz = NULL; 
    int ret = 0;
    bool bOk = false;
    SockName name;

    do {
        bOk = RtpTool::isRtcpHeader(buf, size);
        if (!bOk) {
            SockTool::addr2IP(&name, addr);
            
            LOG_INFO("read_rtcp| fd=%d| size=%d|"
                " ip=%s:%d|"
                " msg=read invalid rtcp header",
                fd, size, name.m_ip, name.m_port);
            break;
        }

        pMsg = MsgCenter::allocUdpMsg(size);
        MsgCenter::setUdpAddr(pMsg, *addr);
        
        psz = MsgCenter::getMsg(pMsg);

        MiscTool::bcopy(psz, buf, size);
        ret = m_frame->dispatch(fd, pMsg);
        if (0 != ret) {
            break;
        }
    } while (false);
    
    return 0;
}

void RtpCenter::closeSvr(int fd) {
    LOG_INFO("close_rtp_svr| fd=%d|", fd);
}

void RtpCenter::publishRtp(InstStream* inst, 
    const ClockTime& ntp, NodeMsg* msg) {
    HList* node = NULL;
    FFStream* ff = NULL;

    for_each_hlist(node, &inst->m_player_list) {
        ff = (FFStream*)node;

        sendRtp(ff, ntp, msg);
    } 
}

void RtpCenter::handleRtp(InstStream* inst, 
    const ClockTime& ntp, const RtpPkg* pkg,
    Cache* cache) {
    NodeMsg* msg = NULL;
    int size = pkg->m_payload_beg + pkg->m_payload_size;
    int pos = pkg->m_payload_beg;
    
    msg = genRtpMsg(inst, ntp, pkg->m_hdr.m_pt,
        pkg->m_hdr.m_marker, 
        cache, size, pos); 
 
    publishRtp(inst, ntp, msg); 
    
    MsgCenter::freeMsg(msg);
}

int RtpCenter::procRtcpBody(FFStream* ff,
    const ClockTime& ntp, const RtcpPkg* pkg) {
    int ret = 0;

    RtpTool::printRtcpPkg("proc_recv", pkg->m_pkg_size, 
        pkg->m_payload.m_size, &pkg->m_hdr);
    
    switch (pkg->m_hdr.m_pt) {
    case ENUM_RTCP_PT_RR:
        ret = procRR(ff, pkg);
        break;

    case ENUM_RTCP_PT_SR:
        ret = procSR(ff, ntp, pkg);
        break;

    case ENUM_RTCP_PT_SDES:
        ret = procSdes(ff, pkg);
        break;

    case ENUM_RTCP_PT_BYE:
        ret = procBye(ff, pkg);
        break;

    case ENUM_RTCP_PT_APP:
        ret = procApp(ff, pkg);
        break;

    default:
        break;
    }
    
    return ret;
}

bool RtpCenter::validSsrc(FFStream* ff, uint_32 ssrc) {
    InstStream* inst = ff->m_inst;
    bool bOk = false;

    
    if (ff->m_ssrc == ssrc) {
        bOk = true;
    } else if (!RtpInstMng::existAddr(inst, ssrc)) {
        if (0 < ff->m_ssrc) {
            RtpInstMng::delSsrc(inst, ff->m_ssrc);
        }
        
        ff->m_ssrc = ssrc;
        RtpInstMng::addSsrc(inst, ssrc, &ff->m_addr.m_rtp);
        bOk = true;
    } else {
        /* conflict ssrc */
    }

    return bOk;
}

int RtpCenter::procSR(FFStream* ff, const ClockTime& ntp, 
    const RtcpPkg* pkg) {
    RecvStatis* statis = &ff->m_rcv_statis;
    int ret = 0;
    bool bOk = false;
    RtcpSR sr; 
    Decoder dec;

    MiscTool::bzero(&sr, sizeof(sr));
    
    dec.set(pkg->m_payload.m_str, pkg->m_payload.m_size);

    bOk = RtpTool::parseSR(&sr, pkg->m_hdr.m_rc, &dec);
    if (!bOk) {
        return -1;
    }
    
    RtpTool::printSR("proc", sr.m_ssrc, &sr.m_sender);

    bOk = validSsrc(ff, sr.m_ssrc);
    if (!bOk) {
        return -1;
    } 

    statis->m_last_sr_rtp.m_sec = sr.m_sender.m_ntp_high;
    statis->m_last_sr_rtp.m_frac = sr.m_sender.m_ntp_low; 
    statis->m_last_sr_unit = sr.m_sender.m_rtp_ts;
    statis->m_last_rcv_sr_ntp = ntp;

    if (!statis->m_has_sr) {
        statis->m_has_sr = true;
        
        statis->m_first_sr_unit = statis->m_last_sr_unit;
        statis->m_first_sr_rtp = statis->m_last_sr_rtp;
        statis->m_first_rcv_sr_ntp = statis->m_last_rcv_sr_ntp;
    }

    return ret;
}

int RtpCenter::procRR(FFStream* ff, const RtcpPkg* pkg) { 
    int ret = 0; 
    bool bOk = false;
    RtcpRR rr;
    Decoder dec;

    MiscTool::bzero(&rr, sizeof(rr));
    
    dec.set(pkg->m_payload.m_str, pkg->m_payload.m_size);

    bOk = RtpTool::parseRR(&rr, pkg->m_hdr.m_rc, &dec);
    if (!bOk) {
        return -1;
    }
    
    RtpTool::printRR("proc", rr.m_ssrc, &rr.m_reports[0]);

    bOk = validSsrc(ff, rr.m_ssrc);
    if (!bOk) {
        return -1;
    }

    return ret;
}

int RtpCenter::procSdes(FFStream* ff, const RtcpPkg* pkg) {
    SdesChunk* chunk = NULL;
    SdesItem* item = NULL;
    int ret = 0;
    int len = 0;
    bool bOk = false;
    RtcpSdes sdes;
    Decoder dec;

    MiscTool::bzero(&sdes, sizeof(sdes));
    
    dec.set(pkg->m_payload.m_str, pkg->m_payload.m_size);
    chunk = &sdes.m_chunks[0];
    item = &chunk->m_items[0];

    bOk = RtpTool::parseSdes(&sdes, pkg->m_hdr.m_rc, &dec);
    if (!bOk) {
        return -1;
    }

    RtpTool::printSdes("proc", chunk->m_src, item);

    len = item->m_desc.m_size;
    if (MAX_CNAME_SIZE < len) {
        len = MAX_CNAME_SIZE;
    }
    
    MiscTool::strCpy(ff->m_sdes.m_cname, item->m_desc.m_str, len); 
    return ret;
}

int RtpCenter::procBye(FFStream* ff, const RtcpPkg* pkg) {
    InstStream* inst = ff->m_inst;
    int ret = 0;
    bool bOk = false;
    RtcpBye bye;
    Decoder dec;

    MiscTool::bzero(&bye, sizeof(bye));
    
    dec.set(pkg->m_payload.m_str, pkg->m_payload.m_size);

    bOk = RtpTool::parseByte(&bye, pkg->m_hdr.m_rc, &dec);
    if (bOk) {
        LOG_INFO("print_bye| pkg_size=%d| hdr_len=%d|"
            " body_len=%d| padding=%d| pad_len=%d|"
            " payload=%d| cc=%d| ssrc=0x%x|"
            " reason=%.*s|",
            pkg->m_pkg_size, pkg->m_hdr.m_length, 
            pkg->m_payload.m_size, 
            pkg->m_hdr.m_padding, pkg->m_pad_len,
            pkg->m_hdr.m_pt, pkg->m_hdr.m_rc, 
            bye.m_srcs[0],
            bye.m_reason.m_size, bye.m_reason.m_str);
    } else {
        return -1;
    }

    RtpInstMng::delSsrc(inst, bye.m_srcs[0]);

    return ret;
}

int RtpCenter::procApp(FFStream* ff, const RtcpPkg* pkg) {
    int ret = 0;
    bool bOk = false;
    RtcpApp app;
    Decoder dec;

    MiscTool::bzero(&app, sizeof(app));
    
    dec.set(pkg->m_payload.m_str, pkg->m_payload.m_size);

    bOk = RtpTool::parseApp(&app, &dec);
    if (!bOk) {
        ret = -1;
    }

    return ret;
} 

bool RtpCenter::genSR(FFStream* ff, Encoder* enc,
    const ClockTime& ntp) {
    int beg = 0;
    bool bOk = false;
    
    beg = enc->curPos();
    
    bOk = RtpTool::genRtcpHeader(enc, 0, ENUM_RTCP_PT_SR);
    if (!bOk) {
        return bOk;
    }

    bOk = enc->genU32(&ff->m_inst->m_stat.m_ssrc);
    if (!bOk) {
        return bOk;
    }

    bOk = RtpTool::genSender(ff, enc, ntp);
    if (!bOk) {
        return bOk;
    }

    bOk = RtpTool::finishRtcp(enc, beg);
    if (!bOk) {
        return bOk;
    }

    if (RtpInstMng::validCname(&ff->m_inst->m_stat)) {
        bOk = genSdes(enc, &ff->m_inst->m_stat);
        if (!bOk) {
            return bOk;
        }
    }
    
    return bOk;
}

bool RtpCenter::genRR(FFStream* ff, Encoder* enc,
    const ClockTime& ntp) {
    int beg = 0;
    bool bOk = false;

    beg = enc->curPos();
    
    bOk = RtpTool::genRtcpHeader(enc, 1, ENUM_RTCP_PT_RR);
    if (!bOk) {
        return bOk;
    }

    bOk = enc->genU32(&ff->m_inst->m_stat.m_ssrc);
    if (!bOk) {
        return bOk;
    }

    bOk = RtpTool::genReport(ff, enc, ntp);
    if (!bOk) {
        return bOk;
    }

    bOk = RtpTool::finishRtcp(enc, beg);
    if (!bOk) {
        return bOk;
    }

    if (RtpInstMng::validCname(&ff->m_inst->m_stat)) {
        bOk = genSdes(enc, &ff->m_inst->m_stat);
        if (!bOk) {
            return bOk;
        }
    }
    
    return bOk;
}

bool RtpCenter::genSdes(Encoder* enc, const InstStat* stat) {
    int beg = 0;
    bool bOk = false;
    AVal desc; 
    
    desc.m_str = stat->m_sdes.m_cname;
    desc.m_size = MiscTool::strLen(desc.m_str, MAX_CNAME_SIZE);
    
    beg = enc->curPos();

    bOk = RtpTool::genRtcpHeader(enc, 1, ENUM_RTCP_PT_SDES);
    if (!bOk) {
        return bOk;
    }
    
    bOk = RtpTool::genChunk(enc, stat->m_ssrc, ENUM_SDES_CNAME, &desc);
    if (!bOk) {
        return bOk;
    }

    bOk = RtpTool::finishRtcp(enc, beg);
    if (!bOk) {
        return bOk;
    }
        
    return bOk;
}

int RtpCenter::calcSdesLen(const char cname[]) {
    int total = 4 + 4;

    total += MiscTool::strLen(cname, MAX_CNAME_SIZE);
    total += 4;
    return total;
}

void RtpCenter::procRtpData(InstStream* inst, 
    FFStream* ff, const ClockTime& ntp, 
    const RtpPkg* pkg, Cache* cache) {
    RecvStatis* statis = &ff->m_rcv_statis;
    bool bOk = false; 
    
    bOk = RtpInstMng::validSeq(statis, pkg->m_hdr.m_seq);
    if (!bOk) {
        return;
    } 
    
    updateRtpRecv(ff, ntp, pkg); 
    handleRtp(inst, ntp, pkg, cache);
}

NodeMsg* RtpCenter::genRtpMsg(InstStream* inst, 
    const ClockTime& ntp, uint_8 payload, bool isEnd,
    Cache* cache, int size, int pos) {
    InstStat* stat = &inst->m_stat;
    Cache* ref = NULL;
    char* psz = NULL;
    NodeMsg* msg = NULL;
    int total = 0;
    int len = 0;
    uint_32 seq = 0;
    uint_32 ts = 0;
    bool bOk = false;
    Encoder enc;
    ClockTime delta; 

    delta = ntp - stat->m_first_ntp;
    ts = RtpInstMng::ntp2Unit(inst->m_media, delta)
        + stat->m_base_unit;

    seq = ++stat->m_seq;
    
    total = MIN_RTP_HDR_SIZE;
    msg = MsgCenter::allocUdpMsg(total);
    
    psz = MsgCenter::getMsg(msg);
    enc.set(psz, total); 

    bOk = RtpTool::genRtpHeader(&enc, stat->m_ssrc,
        ts, seq, payload, isEnd);
    if (!bOk) {
        MsgCenter::freeMsg(msg);
        return NULL;
    } 

    len = enc.curPos();
    MsgCenter::setMsgSize(msg, len);

    if (0 <= pos && pos < size) { 
        /* ext body */
        ref = CacheUtil::ref(cache);
        MsgCenter::setCache(msg, ref, size, true);
        MsgCenter::setMsgPos(msg, pos, true);
    }

    return msg; 
}

void RtpCenter::sendSR(FFStream* ff, const ClockTime& ntp) {
    InstStat* stat = &ff->m_inst->m_stat;
    SendStatis* statis = &ff->m_snd_statis;
    char* psz = NULL;
    NodeMsg* msg = NULL;
    int total = 0;
    int size = 0;
    bool bOk = false;
    Encoder enc;

    if (!statis->m_ready) { 
        statis->m_first_ntp = ff->m_inst->m_stat.m_first_ntp;
        statis->m_base_unit = ff->m_inst->m_stat.m_base_unit;
        
        statis->m_ready = true;
    }

    total = MIN_SR_SIZE;
    if (RtpInstMng::validCname(stat)) {
        total += calcSdesLen(stat->m_sdes.m_cname);
    } 
    
    msg = MsgCenter::allocUdpMsg(total);
    psz = MsgCenter::getMsg(msg);

    enc.set(psz, total);
    
    bOk = genSR(ff, &enc, ntp);
    if (!bOk) {
        MsgCenter::freeMsg(msg);
        return;
    }

    size = enc.curPos();
    MsgCenter::setMsgSize(msg, size);

    sendRtcpMsg(ff, msg); 

    if (!statis->m_has_send_sr) {
        statis->m_has_send_sr = true;
    }
    
    statis->m_last_sr_ntp = ntp;
    statis->m_last_snd_octet_cnt = statis->m_snd_octet_cnt;
}

void RtpCenter::sendRR(FFStream* ff, const ClockTime& ntp) {
    InstStat* stat = &ff->m_inst->m_stat;
    char* psz = NULL;
    NodeMsg* msg = NULL;
    int total = 0;
    int size = 0;
    bool bOk = false;
    Encoder enc;

    total = MIN_RR_SIZE;
    if (RtpInstMng::validCname(stat)) {
        total += calcSdesLen(stat->m_sdes.m_cname);
    } 
    
    msg = MsgCenter::allocUdpMsg(total);
    psz = MsgCenter::getMsg(msg);

    enc.set(psz, total);
    
    bOk = genRR(ff, &enc, ntp);
    if (!bOk) {
        MsgCenter::freeMsg(msg);
        return;
    }

    size = enc.curPos();
    MsgCenter::setMsgSize(msg, size);

    sendRtcpMsg(ff, msg); 
}

void RtpCenter::sendRtp(FFStream* ff,
    const ClockTime& ntp, NodeMsg* msg) {
    SendStatis* statis = &ff->m_snd_statis;
    NodeMsg* ref = NULL;
    int len = 0;

    if (!statis->m_ready) { 
        statis->m_first_ntp = ff->m_inst->m_stat.m_first_ntp;
        statis->m_base_unit = ff->m_inst->m_stat.m_base_unit;
        
        statis->m_ready = true;
    }
  
    ref = MsgCenter::refUdpMsg(msg);
    
    /* payload length */
    len = MsgCenter::getLeft(ref, true); 

    sendRtpMsg(ff, ref);
    
    statis->m_snd_octet_cnt += len;
    ++statis->m_snd_pkg_cnt;
    
    updateRtpSend(ff, ntp);
}

void RtpCenter::updateRtpSend(FFStream* ff, const ClockTime& ntp) {
    SendStatis* statis = &ff->m_snd_statis;
    bool bOk = false; 

    bOk = RtpInstMng::chkSend(statis, ntp);
    if (bOk) {
        sendSR(ff, ntp);
    }
}

void RtpCenter::updateRtpRecv(FFStream* ff, 
    const ClockTime& ntp, const RtpPkg* pkg) {
    RecvStatis* statis = &ff->m_rcv_statis;
    bool bOk = false; 
    uint_32 jitter = 0;
    uint_32 deltaUnit = 0;
    ClockTime deltaTm;

    statis->m_rcv_octet_cnt += pkg->m_payload_size;
    ++statis->m_rcv_pkg_cnt; 

    if (statis->m_has_rtp) {
        /* exclude the sampe ts by sampling delay */
        if (statis->m_last_rtp_unit != pkg->m_hdr.m_ts) {
            /* calc jitter */
            deltaTm = ntp - statis->m_last_rtp_ntp;
            deltaUnit = pkg->m_hdr.m_ts - statis->m_last_rtp_unit;
        
            jitter = RtpInstMng::ntp2Unit(ff->m_inst->m_media, deltaTm);
            if (jitter >= deltaUnit) {
                jitter -= deltaUnit;
            } else {
                jitter = deltaUnit - jitter;
            }
        
            statis->m_jitter += jitter - ((statis->m_jitter + 8) >> 4);

            statis->m_last_rtp_ntp = ntp;
            statis->m_last_rtp_unit = pkg->m_hdr.m_ts;
        }
    } else {
        statis->m_last_rtp_ntp = ntp;
        statis->m_last_rtp_unit = pkg->m_hdr.m_ts;
        statis->m_has_rtp = true;
    }
    
    bOk = RtpInstMng::chkRecv(statis, ntp);
    if (bOk) {
        sendRR(ff, ntp);
    }
}

void RtpCenter::sendRtpMsg(FFStream* ff, NodeMsg* msg) { 
    MsgCenter::setUdpAddr(msg, ff->m_addr.m_rtp);
    m_frame->sendMsg(ff->m_addr.m_rtp_fd, msg); 
}

void RtpCenter::sendRtcpMsg(FFStream* ff, NodeMsg* msg) {
    MsgCenter::setUdpAddr(msg, ff->m_addr.m_rtcp);
    m_frame->sendMsg(ff->m_addr.m_rtcp_fd, msg);
}

