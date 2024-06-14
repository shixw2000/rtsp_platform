#include"rtpproto.h"
#include"rtputil.h"
#include"encoder.h"
#include"misc.h"
#include"shareheader.h"
#include"rtpinst.h"


#define ENC_RULE_DEF(id, type, mem) \
    {id, OFFSET(type, mem), #type"."#mem}

static const EncRule RtpHdRule[] = {
    ENC_RULE_DEF(U_INT_2, RtpHdr, m_version),
    ENC_RULE_DEF(U_INT_1, RtpHdr, m_padding),
    ENC_RULE_DEF(U_INT_1, RtpHdr, m_ext),
    ENC_RULE_DEF(U_INT_4, RtpHdr, m_cc),
    ENC_RULE_DEF(U_INT_1, RtpHdr, m_marker),
    ENC_RULE_DEF(U_INT_7, RtpHdr, m_pt),
    ENC_RULE_DEF(U_INT_16, RtpHdr, m_seq),
    ENC_RULE_DEF(U_INT_32, RtpHdr, m_ts),
    ENC_RULE_DEF(U_INT_32, RtpHdr, m_ssrc)
};

static const EncRule RtcpHdRule[] = {
    ENC_RULE_DEF(U_INT_2, RtcpHdr, m_version),
    ENC_RULE_DEF(U_INT_1, RtcpHdr, m_padding),
    ENC_RULE_DEF(U_INT_5, RtcpHdr, m_rc),
    ENC_RULE_DEF(U_INT_8, RtcpHdr, m_pt),
    ENC_RULE_DEF(U_INT_16, RtcpHdr, m_length)
};

static const EncRule RtcpSenderRule[] = {
    ENC_RULE_DEF(U_INT_32, RtcpSenderInfo, m_ntp_high),
    ENC_RULE_DEF(U_INT_32, RtcpSenderInfo, m_ntp_low),
    ENC_RULE_DEF(U_INT_32, RtcpSenderInfo, m_rtp_ts),
    ENC_RULE_DEF(U_INT_32, RtcpSenderInfo, m_snd_pkt_cnt),
    ENC_RULE_DEF(U_INT_32, RtcpSenderInfo, m_snd_octet_cnt)
};

static const EncRule RtcpReportRule[] = {
    ENC_RULE_DEF(U_INT_32, RtcpReportBlk, m_ssrc),
    ENC_RULE_DEF(U_INT_8, RtcpReportBlk, m_frac_lost),
    ENC_RULE_DEF(INT_24, RtcpReportBlk, m_cumm_pkg_lost),
    ENC_RULE_DEF(U_INT_32, RtcpReportBlk, m_max_seq),
    ENC_RULE_DEF(U_INT_32, RtcpReportBlk, m_jitter),
    ENC_RULE_DEF(U_INT_32, RtcpReportBlk, m_ntp_lsr),
    ENC_RULE_DEF(U_INT_32, RtcpReportBlk, m_delay_lsr)
}; 

bool RtpTool::isRtcpPt(uint_8 pt) {
    if ((ENUM_RTCP_PT_SR <= pt &&
        ENUM_RTCP_PT_TOKEN >= pt) ||
        (ENUM_RTCP_PT_FIR <= pt &&
        ENUM_RTCP_PT_IJ >= pt)){
        return true;
    } else {
        return false;
    }
}

bool RtpTool::isVerValid(uint_8 ver) {
    return DEF_RTP_VERSION == ver;
}

bool RtpTool::isRtpHeader(const char* data, int size) {
    bool bOk = false;
    RtcpHdr hdr;
    Decoder dec(data, size);
    
    do {
        if (MIN_RTP_HDR_SIZE > size) {
            break;
        }

        bOk = dec.parse(RtcpHdRule, ARR_CNT(RtcpHdRule), &hdr);
        if (!bOk) {
            break;
        }

        if (!isVerValid(hdr.m_version)) {
            LOG_INFO("chk_rtp_hdr| ver=%u|"
                " msg=invalid version|",
                hdr.m_version);
            break;
        }

        if (isRtcpPt(hdr.m_pt)) {
            LOG_INFO("chk_rtp_hdr| payload=%u|"
                " msg=invalid payload|",
                hdr.m_pt);
            break;
        }
        
        return true;
    } while (false);

    return false;
}

bool RtpTool::isRtcpHeader(const char* data, int size) {
    int len = 0;
    bool bOk = false;
    RtcpHdr hdr;
    Decoder dec(data, size);
    
    do {
        if (MIN_RTCP_HDR_SIZE > size) {
            break;
        }

        bOk = dec.parse(RtcpHdRule, ARR_CNT(RtcpHdRule), &hdr);
        if (!bOk) {
            break;
        }

        if (!isVerValid(hdr.m_version)) {
            LOG_INFO("chk_rtcp_hdr| ver=%u|"
                " msg=invalid version|",
                hdr.m_version);
            break;
        }

        if (!isRtcpPt(hdr.m_pt)) {
            LOG_INFO("chk_rtcp_hdr| payload=%u|"
                " msg=invalid payload|",
                hdr.m_pt);
            break;
        }

        len = hdr.m_length << 2;
        if (len > dec.leftLen()) {
            LOG_INFO("chk_rtcp_hdr| hdr_length=%u|"
                " total=%d| msg=invalid length|",
                hdr.m_length, size);
            break;
        }

        return true;
    } while (false);

    return false;
}

bool RtpTool::parseRtpPkg(RtpPkg* pkg, Decoder* dec) {
    RtpHdr* hdr = &pkg->m_hdr; 
    bool bOk = false;
    int total = 0;
    int size = 0;
    int left = 0; 
    uint_16 len = 0;
    uint_8 pt = 0; 
    
    do { 
        MiscTool::bzero(pkg, sizeof(*pkg));

        bOk = dec->parse(RtpHdRule, ARR_CNT(RtpHdRule), hdr);
        if (!bOk) {
            break;
        }

        if (!isVerValid(hdr->m_version)) {
            break;
        }

        pt = hdr->m_pt | 0x80;
        if (isRtcpPt(pt)) {
            break;
        }

        for (uint_8 i=0; i<hdr->m_cc; ++i) {
            bOk = dec->parseU32(&hdr->m_csrc[i]);
            if (!bOk) {
                break;
            }
        }

        if (!bOk) {
            break;
        }

        if (hdr->m_ext) {
            bOk = dec->parseU16(&pkg->m_ext_type);
            if (!bOk) {
                break;
            }

            bOk = dec->parseU16(&len);
            if (!bOk) {
                break;
            }

            size = (int)len << 2;
            bOk = dec->parseStr(&pkg->m_ext_txt, size);
            if (!bOk) {
                break;
            }
        } 

        total = dec->size();
        left = dec->leftLen();
        
        if (hdr->m_padding) {
            pkg->m_pad_len = *(dec->data(total - 1));
            
            if (pkg->m_pad_len > left) {
                LOG_INFO("parse_rtp| total=%d| pad_len=%d|"
                    " left=%d| msg=invalid padding|",
                    total, pkg->m_pad_len, left);
                
                break;
            }
        } 

        pkg->m_pkg_size = total;
        pkg->m_payload_beg = dec->curPos();

        if (!(0 < pkg->m_pad_len)) {
            pkg->m_payload_size = left;
        } else {
            pkg->m_payload_size = left - pkg->m_pad_len;
        }

        dec->parseStr(NULL, left);
        
        return true;
    } while (false);

    return false;
}

void RtpTool::printRtpPkg(const char prompt[],
    int pkg_size, int blen, const RtpHdr* hdr) {
    char* psz = NULL;
    int len = 0;
    int max = 0;
    char tmp[256] = {0};

    if (0 < hdr->m_cc) {
        max = (int)sizeof(tmp);
        psz = tmp;

        len = MiscTool::strPrint(psz, max, "0x%x", hdr->m_csrc[0]);
        psz += len;
        max -= len;
        
        for (uint_8 i=1; i<hdr->m_cc; ++i) {
            len = MiscTool::strPrint(psz, max, ", 0x%x", 
                hdr->m_csrc[i]);

            psz += len;
            max -= len;
        }
    } else {
        tmp[0] = '\0';
    }

    LOG_INFO("%s_rtp_pkg| ver=%u| padding=%d| ext=%d|"
        " cc=%u| marker=%d| payload=%u| seq=0x%x|"
        " timestamp=%u| ssrc=0x%x| csrc=%s|"
        " pkg_size=%d| payload_size=%d|"
        " msg=print rtp pkg|",
        prompt,
        hdr->m_version, hdr->m_padding,
        hdr->m_ext, hdr->m_cc, hdr->m_marker,
        hdr->m_pt, hdr->m_seq,
        hdr->m_ts, hdr->m_ssrc, tmp, 
        pkg_size, blen); 
}

void RtpTool::printRtcpPkg(const char prompt[],
    int pkg_size, int blen,
    const RtcpHdr* hdr) {
    
    LOG_INFO("%s_rtcp_pkg| ver=%u| padding=%d| rc=%u|"
        " pkg_size=%d| payload_size=%d|"
        " msg=print rtcp pkg|", 
        prompt, hdr->m_version, 
        hdr->m_padding, hdr->m_rc,
        pkg_size, blen); 
}

void RtpTool::printSR(const char prompt[],
    uint_32 ssrc, const RtcpSenderInfo* sender) {
    LOG_INFO("%s_sr| s_ssrc=0x%x|"
        " s_ntp_hi=%u| s_ntp_low=%u|"
        " s_rtp_ts=%u| s_pkg_cnt=%u| s_byte_cnt=%u|",
        prompt, ssrc,
        sender->m_ntp_high, sender->m_ntp_low,
        sender->m_rtp_ts, sender->m_snd_pkt_cnt,
        sender->m_snd_octet_cnt);
}

void RtpTool::printRR(const char prompt[],
    uint_32 ssrc, const RtcpReportBlk* report) {
    LOG_INFO("%s_rr| s_ssrc=0x%x|"
        " r_ssrc=0x%x| r_frac_lost=%u| r_cumm_lost=%d|"
        " r_max_seq=0x%x| r_jitter=%u|"
        " r_lsr_ts=%u| r_lsr_delay=%u|",
        prompt, ssrc,
        report->m_ssrc,
        report->m_frac_lost, 
        report->m_cumm_pkg_lost,
        report->m_max_seq,
        report->m_jitter,
        report->m_ntp_lsr,
        report->m_delay_lsr);
}

void RtpTool::printSdes(const char prompt[],
    uint_32 ssrc, const SdesItem* item) {
    LOG_INFO("%s_sdes| s_ssrc=0x%x|"
        " item_type=%d| item_val=%.*s|",
        prompt,
        ssrc, item->m_type,
        item->m_desc.m_size, item->m_desc.m_str);
}

bool RtpTool::parseRtcpPkg(RtcpPkg* pkg, Decoder* dec) {
    RtcpHdr* hdr = &pkg->m_hdr;
    int total = 0;
    int size = 0;
    int len = 0;
    bool bOk = false;
    
    do { 
        MiscTool::bzero(pkg, sizeof(*pkg));
        
        bOk = dec->parse(RtcpHdRule, ARR_CNT(RtcpHdRule), hdr);
        if (!bOk) {
            break;
        }

        if (!isVerValid(hdr->m_version)) {
            break;
        }

        if (!isRtcpPt(hdr->m_pt)) {
            break;
        }

        size = hdr->m_length;
        size <<= 2;
        total = size + 4;
        
        if (size > dec->leftLen()) {
            break;
        }

        if (hdr->m_padding) {
            len = dec->curPos() + size;
            pkg->m_pad_len = *dec->data(len - 1);
            if (pkg->m_pad_len <= size) {
                size -= pkg->m_pad_len;
            } else {
                LOG_INFO("parse_rtcp| total=%d| pad_len=%d|"
                    " size=%d| msg=invalid padding|",
                    total, pkg->m_pad_len, size);
                
                break;
            }
        }

        pkg->m_pkg_size = total;
        dec->parseStr(&pkg->m_payload, size);
        
        if (0 < pkg->m_pad_len) {
            dec->parseStr(NULL, pkg->m_pad_len);
        }

        return true;
    } while (false);

    return false;
}

bool RtpTool::parseSenderInfo(RtcpSenderInfo* sender, Decoder* dec) {
    bool bOk = false;

    bOk = dec->parse(RtcpSenderRule, ARR_CNT(RtcpSenderRule), sender);
    return bOk;
}

bool RtpTool::parseReportBlk(RtcpReportBlk* blk, Decoder* dec) {
    bool bOk = false;

    bOk = dec->parse(RtcpReportRule, ARR_CNT(RtcpReportRule), blk);
    return bOk;
}

bool RtpTool::parseSdesChunk(
    SdesChunk* sdes, Decoder* dec) {
    bool bOk = false;
    int pos = 0;
    int padding = 0;
    SdesItem item;
    
    bOk = dec->parseU32(&sdes->m_src);
    if (!bOk) {
        return bOk;
    }
    
    while (!dec->isEnd()) {
        bOk = dec->parseU8(&item.m_type);
        if (!bOk) {
            break;
        }

        if (ENUM_SDES_END < item.m_type &&
            ENUM_SDES_PRIV >= item.m_type) {
            bOk = dec->parseStrU8(&item.m_desc);
            if (!bOk) {
                break;
            }
            
            if (MAX_ITEM_SIZE > sdes->m_item_cnt) {
                sdes->m_items[sdes->m_item_cnt] = item;
                ++sdes->m_item_cnt;
            }
        } else if (ENUM_SDES_END == item.m_type) {
            pos = dec->curPos();
            padding = 4 - (pos & 0x3);
            bOk = dec->parseStr(NULL, padding);
            break;
        } else {
            /* invalid sdes item type */
            bOk = false;
            break;
        }
    }

    return bOk;
}

bool RtpTool::parseSdes(RtcpSdes* sdes, uint_8 rc, Decoder* dec) {
    bool bOk = false;

    for (uint_8 i=0; i<rc; ++i) { 
        bOk = parseSdesChunk(&sdes->m_chunks[i], dec);
        if (!bOk) {
            return bOk;
        }
    }

    return dec->isEnd();
}

bool RtpTool::parseSR(RtcpSR* sr, uint_8 rc, Decoder* dec) {
    bool bOk = false;

    bOk = dec->parseU32(&sr->m_ssrc);
    if (!bOk) {
        return bOk;
    }

    bOk = parseSenderInfo(&sr->m_sender, dec);
    if (!bOk) {
        return bOk;
    }

    for (uint_8 i=0; i<rc; ++i) {
        bOk = parseReportBlk(&sr->m_reports[i], dec);
        if (!bOk) {
            return bOk;
        }
    }

    return dec->isEnd();
}

bool RtpTool::parseRR(RtcpRR* rr, uint_8 rc, Decoder* dec) {
    bool bOk = false;

    bOk = dec->parseU32(&rr->m_ssrc);
    if (!bOk) {
        return bOk;
    }

    for (uint_8 i=0; i<rc; ++i) {
        bOk = parseReportBlk(&rr->m_reports[i], dec);
        if (!bOk) {
            return bOk;
        }
    }

    return dec->isEnd();
}

bool RtpTool::parseByte(RtcpBye* bye, uint_8 rc, Decoder* dec) {
    int len = 0;
    bool bOk = false;

    for (uint_8 i=0; i<rc; ++i) { 
        bOk = dec->parseU32(&bye->m_srcs[i]);
        if (!bOk) {
            return bOk;
        }
    }

    len = dec->leftLen();
    if (0 < len) {
        bOk = dec->parseStrU16(&bye->m_reason);
        if (!bOk) {
            return bOk;
        }
    }

    return dec->isEnd();
}

bool RtpTool::parseApp(RtcpApp* app, Decoder* dec) {
    int len = 0;
    bool bOk = false; 

    bOk = dec->parseU32(&app->m_ssrc);
    if (!bOk) {
        return bOk;
    }

    bOk = dec->parseStr(&app->m_name, 4);
    if (!bOk) {
        return bOk;
    }

    len = dec->leftLen(); 
    if (0 < len) {
        bOk = dec->parseStr(&app->m_app, len);
        if (!bOk) {
            return bOk;
        }
    }

    return dec->isEnd();
}


bool RtpTool::genRtpHeader(Encoder* enc, uint_32 ssrc, 
    uint_32 ts, uint_16 seq, uint_8 payload, bool isEnd) {
    bool bOk = false;
    RtpHdr hdr;

    MiscTool::bzero(&hdr, sizeof(hdr));
    hdr.m_version = DEF_RTP_VERSION;
    hdr.m_ssrc = ssrc;
    hdr.m_ts = ts;
    hdr.m_seq = seq;
    hdr.m_pt = payload;
    hdr.m_marker = isEnd; 

    printRtpPkg("write", 0, 0, &hdr);

    bOk = enc->generate(RtpHdRule, ARR_CNT(RtpHdRule), &hdr);
    if (!bOk) {
        return bOk;
    }
    
    return bOk;
}

bool RtpTool::genRtcpHeader(Encoder* enc, uint_8 rc, uint_8 pt) {
    bool bOk = false;
    RtcpHdr hdr;

    MiscTool::bzero(&hdr, sizeof(hdr));
    hdr.m_version = DEF_RTP_VERSION;
    hdr.m_rc = rc;
    hdr.m_pt = pt;

    bOk = enc->generate(RtcpHdRule, ARR_CNT(RtcpHdRule), &hdr);
    if (!bOk) {
        return bOk;
    }
    
    return bOk;
}

bool RtpTool::genSender(FFStream* ff, 
    Encoder* enc, const ClockTime& ntp) {
    InstStream* inst = ff->m_inst;
    const SendStatis* statis = &ff->m_snd_statis;
    bool bOk = false;
    uint_32 ts = 0;
    RtcpSenderInfo sender;
    ClockTime delta;

    MiscTool::bzero(&sender, sizeof(sender)); 

    delta = ntp - statis->m_first_ntp;
    ts = RtpInstMng::ntp2Unit(inst->m_media, delta)
        + statis->m_base_unit;

    sender.m_snd_pkt_cnt = statis->m_snd_pkg_cnt;
    sender.m_snd_octet_cnt = statis->m_snd_octet_cnt;
    sender.m_rtp_ts = ts;
    sender.m_ntp_high = ntp.m_sec;
    sender.m_ntp_low = RtpInstMng::rescale(
        ntp.m_msec, DEF_MOD_U32, 1000); 

    RtpTool::printSR("write", inst->m_stat.m_ssrc, &sender); 
    
    bOk = enc->generate(RtcpSenderRule, ARR_CNT(RtcpSenderRule), &sender);
    if (!bOk) {
        return bOk;
    } 
    
    return bOk;
}

bool RtpTool::genReport(FFStream* ff, Encoder* enc,
    const ClockTime& ntp) {
    RecvStatis* statis = &ff->m_rcv_statis;
    bool bOk = false;
    uint_32 extended_max = 0;
    uint_32 expected_interval = 0;
    uint_32 lost_interval = 0;
    RtcpReportBlk report;
    ClockTime delta;
    
    MiscTool::bzero(&report, sizeof(report)); 

    report.m_ssrc = ff->m_ssrc; 

    extended_max = statis->m_cycles;
    extended_max = (extended_max << 16) + statis->m_max_seq;
    expected_interval = extended_max - statis->m_last_expect_seq; 

    statis->m_cumm_lost += expected_interval - statis->m_rcv_pkg_cnt;
    statis->m_cumm_cycles += statis->m_cycles; 
    statis->m_cumm_recv += statis->m_rcv_pkg_cnt;
    
    if (expected_interval > statis->m_rcv_pkg_cnt) {
        lost_interval = expected_interval - statis->m_rcv_pkg_cnt;
        report.m_frac_lost = (uint_8)RtpInstMng::rescale(lost_interval, 
            (1 << 8), expected_interval);
    } else {
        report.m_frac_lost = 0;
    }

    if (!(statis->m_cumm_lost >> 31)) {
        report.m_cumm_pkg_lost = statis->m_cumm_lost;
    } else {
        report.m_cumm_pkg_lost = 0;
    }
    
    report.m_max_seq = (statis->m_cumm_cycles << 16)
        + statis->m_max_seq;
    
    report.m_jitter = statis->m_jitter >> 4;

    if (statis->m_has_sr) {
        report.m_ntp_lsr = RtpInstMng::rtp2U32(statis->m_last_sr_rtp);

        delta = ntp - statis->m_last_rcv_sr_ntp;
        report.m_delay_lsr = RtpInstMng::ntp2U32(delta);
    } else {
        report.m_ntp_lsr = 0;
        report.m_delay_lsr = 0;
    } 

    statis->m_rcv_pkg_cnt = 0;
    statis->m_cycles = 0;
    statis->m_last_expect_seq = statis->m_max_seq;
    
    statis->m_last_rr_ntp = ntp;
    statis->m_last_rcv_octet_cnt = statis->m_rcv_octet_cnt;

    RtpTool::printRR("write", ff->m_inst->m_stat.m_ssrc, &report);
    
    bOk = enc->generate(RtcpReportRule, ARR_CNT(RtcpReportRule), &report);
    if (!bOk) {
        return bOk;
    }
    
    return bOk;
}

bool RtpTool::genChunk(Encoder* enc, uint_32 ssrc,
    uint_8 type, const AVal* desc) {
    bool bOk = false;
    int pos = 0;
    int padding = 0;

    bOk = enc->genU32(&ssrc);
    if (!bOk) {
        return bOk;
    } 
    
    bOk = enc->genU8(&type);
    if (!bOk) {
        return bOk;
    }

    bOk = enc->genStrU8(desc);
    if (!bOk) {
        return bOk;
    } 

    pos = enc->curPos();
    padding = 4 - (pos & 0x3);
    enc->genChar(padding, 0); 
    return bOk;
}

bool RtpTool::genBye(Encoder* enc, uint_32 ssrc,
    const AVal* reason) {
    bool bOk = false; 
      
    bOk = enc->genU32(&ssrc);
    if (!bOk) {
        return bOk;
    } 

    if (NULL != reason && 0 < reason->m_size) {
        bOk = enc->genStrU16(reason);
        if (!bOk) {
            return bOk;
        }
    }

    return bOk;
}

bool RtpTool::finishRtcp(Encoder* enc, int beg) {
    int len = 0;
    int end = 0;
    uint_16 size = 0;
    bool bOk = false;

    end = enc->curPos();
    
    if (beg < end) {
        len = end - beg;
        if (0 == (beg & 0x3) && 0 == (end & 0x3)) {
            len >>= 2;
            size = (uint_16)(len - 1);

            bOk = enc->writeU16(&size, beg + 2);
        } else {
            LOG_WARN("finish_rtcp| beg=%d| end=%d|"
                " len=%d| msg=invalid beg or end|",
                beg, end, len);
        }
    }

    return bOk;
}

