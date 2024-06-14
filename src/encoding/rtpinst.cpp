#include"rtpinst.h"
#include"misc.h"
#include"tokenutil.h" 

ClockTime operator-(const ClockTime& t1, const ClockTime& t2) {
    ClockTime res = t1;

    res -= t2; 
    return res;
}

ClockTime& operator-=(ClockTime& t1, const ClockTime& t2) {
    if (t1.m_sec > t2.m_sec) {
        if (t1.m_msec >= t2.m_msec) {
            t1.m_sec -= t2.m_sec;
            t1.m_msec -= t2.m_msec;
        } else {
            t1.m_sec -= t2.m_sec + 1;
            t1.m_msec += 1000 - t2.m_msec;
        }
    } else if (t1.m_sec == t2.m_sec) {
        t1.m_sec = 0;
        
        if (t1.m_msec > t2.m_msec) {
            t1.m_msec -= t2.m_msec;
        } else {
            t1.m_msec = 0;
        }
    } else {
        /* time is monotonic increasing */ 
        t1.m_sec += ~t2.m_sec;
        if (t1.m_msec >= t2.m_msec) {
            ++t1.m_sec;
            t1.m_msec -= t2.m_msec;
        } else {
            t1.m_msec += 1000 - t2.m_msec;
        }
    }

    return t1;
}

ClockTime operator+(const ClockTime& t1, const ClockTime& t2) {
    ClockTime res = t1;

    res += t2; 
    return res;
}

ClockTime& operator+=(ClockTime& t1, const ClockTime& t2) {
    t1.m_sec += t2.m_sec;
    t1.m_msec += t2.m_msec;

    if (1000 <= t1.m_msec) {
        ++t1.m_sec;
        t1.m_msec -= 1000;
    }

    return t1;
}

void RtpInstMng::getNtpTime(ClockTime& ntp) {
    MiscTool::getTime(&ntp); 
    
    ntp.m_sec += NTP_OFFSET_SEC;
}

uint_32 RtpInstMng::rescale(uint_64 a, uint_64 b, uint_64 c) {
    return a * b / c;
}

uint_32 RtpInstMng::rtp2U32(const RtpTime& rtp) {
    uint_32 u = 0;
    
    u = (rtp.m_sec & 0xFFFF) << 16;
    u |= (rtp.m_frac >> 16) & 0xFFFF;

    return u;
}

uint_32 RtpInstMng::ntp2U32(const ClockTime& ntp) {
    uint_32 u = 0;
    
    u = (ntp.m_sec & 0xFFFF) << 16;
    u |= ((ntp.m_msec << 16) / 1000) & 0xFFFF;

    return u;
}

uint_32 RtpInstMng::ntp2Unit(const SdpMedia* media, 
    const ClockTime& ntp) {
    uint_32 base_time = media->m_codec.m_sample_rate;
    uint_32 v = 0;

    v = ntp.m_sec * base_time;
    v += ntp.m_msec * base_time / 1000;
    return v;
}

bool RtpInstMng::existAddr(InstStream* inst, uint_32 ssrc) {
    typeMapSsrc::const_iterator itr;

    itr = inst->m_map_ssrc.find(ssrc);
    if (inst->m_map_ssrc.end() != itr) {
        return true;
    } else {
        return false;
    }
}

const SockAddr* RtpInstMng::findBySsrc(
    InstStream* inst, uint_32 ssrc) {
    typeMapSsrc::const_iterator itr;

    itr = inst->m_map_ssrc.find(ssrc);
    if (inst->m_map_ssrc.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

bool RtpInstMng::addSsrc(InstStream* inst, uint_32 ssrc,
    const SockAddr* addr) {
    typeMapSsrc::const_iterator itr;

    itr = inst->m_map_ssrc.find(ssrc);
    if (inst->m_map_ssrc.end() == itr) {
        inst->m_map_ssrc[ssrc] = addr;
        return true;
    } else {
        return false;
    }
}

void RtpInstMng::delSsrc(InstStream* inst, uint_32 ssrc) {
    typeMapSsrc::iterator itr;

    itr = inst->m_map_ssrc.find(ssrc);
    if (inst->m_map_ssrc.end() != itr) {
        inst->m_map_ssrc.erase(itr);
    }
}

FFStream* RtpInstMng::findByRtp(InstStream* inst,
    const SockAddr* addr) {
    typeMapAddr::const_iterator itr;

    itr = inst->m_map_rtp_addr.find(addr);
    if (inst->m_map_rtp_addr.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

FFStream* RtpInstMng::findByRtcp(InstStream* inst,
    const SockAddr* addr) {
    typeMapAddr::const_iterator itr;

    itr = inst->m_map_rtcp_addr.find(addr);
    if (inst->m_map_rtcp_addr.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

bool RtpInstMng::match(const char path[], InstStream* inst) {
    Token token(path);
    
    if (TokenUtil::strCmp(&token, inst->m_path, false) ||
        TokenUtil::strCmp(&token, inst->m_grp->m_path, false)) {
        return true;
    } else {
        return false;
    }
}

void RtpInstMng::addFF(InstStream* inst, FFStream* ff) { 
    TransAddr* addr = &ff->m_addr; 
    
    inst->m_map_rtp_addr[ &addr->m_rtp ] = ff;
    inst->m_map_rtcp_addr[ &addr->m_rtcp ] = ff;
}

void RtpInstMng::removeFF(InstStream* inst, FFStream* ff) { 
    deactivateFF(inst, ff);
    
    inst->m_map_rtp_addr.erase(&ff->m_addr.m_rtp);
    inst->m_map_rtcp_addr.erase(&ff->m_addr.m_rtcp);
    inst->m_map_ssrc.erase(ff->m_ssrc);
}

void RtpInstMng::setPublisher(FFStream* ff, bool is_publisher) {
    ff->m_is_publisher = is_publisher;
}

void RtpInstMng::activateFF(InstStream* inst, FFStream* ff) { 
    if (hlistUnhashed(&ff->m_node)) {
        if (!ff->m_is_publisher) {
            hlistAdd(&inst->m_player_list, &ff->m_node);
        } else {
            hlistAdd(&inst->m_publisher_list, &ff->m_node);
        }
    }
}

void RtpInstMng::deactivateFF(InstStream*, FFStream* ff) {
    if (!hlistUnhashed(&ff->m_node)) {
        hlistDel(&ff->m_node);
    }
}

bool RtpInstMng::validCname(const InstStat* stat) {
    return !TokenUtil::isNULL(stat->m_sdes.m_cname[0]);
}

bool RtpInstMng::validSeq(RecvStatis* statis, uint_16 seq) {
    uint_16 delta = 0; 

    if (statis->m_has_rtp) {
        delta = seq - statis->m_max_seq;
        if (delta < MAX_DROPOUT) {
            if (seq < statis->m_max_seq) {
                ++statis->m_cycles;
            }

            statis->m_max_seq = seq;
            return true;
        } else if (delta <= RTP_SEQ_MOD - MAX_MISORDER) {
            /* may be restart program */
            MiscTool::bzero(statis, sizeof(*statis));
            return false;
        } else {
            /* duplicate or reordered pkg */
            return true;
        }
    } else {
        statis->m_max_seq = seq;
        statis->m_last_expect_seq = seq-1;
        statis->m_has_rtp = true;
        
        return true;
    }
}

bool RtpInstMng::chkRecv(const RecvStatis* statis, const ClockTime& ntp) {
    uint_32 bytes = 0;
    bool bOk = false;
    ClockTime delta;
    
    delta = ntp - statis->m_last_rr_ntp;
    if (DEF_RTCP_RR_INTERVAL_SEC <= delta.m_sec) {
        bytes = statis->m_rcv_octet_cnt - statis->m_last_rcv_octet_cnt;
        bytes = bytes * RTCP_TX_RATIO_NUM / RTCP_TX_RATIO_DEN;
        
        if (DEF_RTCP_MIN_SIZE <= bytes) {
            bOk = true;
        }
    }

    return bOk;
}

bool RtpInstMng::chkSend(const SendStatis* statis, const ClockTime& ntp) {
    uint_32 bytes = 0;
    bool bOk = false;
    ClockTime delta;

    if (statis->m_has_send_sr) {
        delta = ntp - statis->m_last_sr_ntp;
        if (DEF_RTCP_SR_INTERVAL_SEC <= delta.m_sec) {
            bytes = statis->m_snd_octet_cnt - statis->m_last_snd_octet_cnt;
            bytes = bytes * RTCP_TX_RATIO_NUM / RTCP_TX_RATIO_DEN;
    
            if (DEF_RTCP_MIN_SIZE <= bytes) {
                bOk = true;
            }
        }
    } else {
        bOk = true;
    }

    return bOk;
}

uint_32 RtpInstMng::genSsrc(InstStream* inst) {
    uint_32 ssrc = 0;
    bool found = false;
    
    while (true) {
        MiscTool::getRand(&ssrc, sizeof(ssrc));
        if (0 < ssrc) {
            found = existAddr(inst, ssrc);
            if (!found) {
                break;
            }
        }
    }

    return ssrc;
} 

