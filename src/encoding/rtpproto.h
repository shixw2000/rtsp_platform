#ifndef __RTPPROTO_H__
#define __RTPPROTO_H__


typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef unsigned long long uint_64;
typedef int int_32;
typedef short int_16;

static const uint_8 DEF_RTP_VERSION = 0x2;
static const uint_32 MAX_RTP_CNT = 16;
static const uint_32 MAX_RTCP_CNT = 32;
static const int MAX_CNAME_SIZE = 256;
static const int MAX_ITEM_SIZE = 10;
static const int MIN_RTP_HDR_SIZE = 12;
static const int MIN_RTCP_HDR_SIZE = 4;

static const uint_32 DEF_RTCP_MIN_SIZE = 28;
static const int MIN_SR_SIZE = 28;
static const int MIN_RR_SIZE = 32;
static const int DEF_REPORT_BLK_SIZE = 24;

static const uint_32 NTP_OFFSET_SEC = 2208988800U;
static const uint_32 DEF_RTCP_SR_INTERVAL_SEC = 5;
static const uint_32 DEF_RTCP_RR_INTERVAL_SEC = 1;
static const uint_64 DEF_MOD_U32 = (0x1ULL << 32);
static const int MAX_DROPOUT    = 3000;
static const int MAX_MISORDER   = 100;
static const int MIN_SEQUENTIAL = 1;
static const int RTP_SEQ_MOD = (1 << 16);

/* 0.5% bandwidth */
static const int RTCP_TX_RATIO_NUM = 5;
static const int RTCP_TX_RATIO_DEN = 1000;

#define SAFE_STR(x) (!!(x) ? (x) : "")

enum EnumPayloadType {
    ENUM_RTCP_PT_FIR = 192,
    ENUM_RTCP_PT_NACK = 193,
    ENUM_RTCP_PT_SMPTETC = 194,
    ENUM_RTCP_PT_IJ = 195,
    
    ENUM_RTCP_PT_SR = 200,
    ENUM_RTCP_PT_RR = 201,
    ENUM_RTCP_PT_SDES = 202,
    ENUM_RTCP_PT_BYE = 203,
    ENUM_RTCP_PT_APP = 204,
    ENUM_RTCP_PT_TOKEN = 210,
};

enum EnumSdesType {
    ENUM_SDES_END = 0,
    ENUM_SDES_CNAME,
    ENUM_SDES_NAME,
    ENUM_SDES_EMAIL,
    ENUM_SDES_PHONE,
    ENUM_SDES_LOC = 5,
    ENUM_SDES_TOOL,
    ENUM_SDES_NOTE,
    ENUM_SDES_PRIV,
};

struct AVal {
    union {
        const char* m_str;
        const uint_8* m_data;
    };
    int m_size;
};

struct AVRational{
    int m_num; ///< numerator
    int m_den; ///< denominator
};

struct RtpTime {
    uint_32 m_sec;
    uint_32 m_frac;
};

struct RtpHdr {
    bool m_padding;    // check if padding: 1
    bool m_ext;        // check if has extention: 1
    bool m_marker;    // marker of the boundary of msg: 1
    uint_8 m_cc;         // count of csrc: 4
    uint_8 m_version;   // version, fix with 2 
    uint_8 m_pt;         // payload type: 7
    uint_16 m_seq;       // seq number incremented by one for each package 
    uint_32 m_ts;        // monotonical timestamp of the package 
    uint_32 m_ssrc;      //
    uint_32 m_csrc[MAX_RTP_CNT]; 
};

struct RtpPkg {
    RtpHdr m_hdr;
    int m_pkg_size;
    int m_pad_len;
    int m_payload_beg;
    int m_payload_size;
    uint_16 m_ext_type;
    AVal m_ext_txt;
};

struct RtcpHdr {
    bool m_padding;    // check if padding: 1
    uint_16 m_length;   // count of  32 bits followed: 16
    uint_8 m_version;   // version, fix with: 2
    uint_8 m_rc;         // count of reception blocks: 5
    uint_8 m_pt;         // payload type: 8
};

struct RtcpPkg {
    RtcpHdr m_hdr;
    int m_pad_len;
    int m_pkg_size;
    AVal m_payload;
};

struct RtcpSenderInfo {
    uint_32 m_ntp_high;
    uint_32 m_ntp_low;
    uint_32 m_rtp_ts;
    uint_32 m_snd_pkt_cnt;
    uint_32 m_snd_octet_cnt;
};

struct RtcpReportBlk {
    uint_32 m_ssrc;      // ssrc of this block belongs to
    int_32 m_cumm_pkg_lost; // cummulative no of pkg lost
    uint_32 m_max_seq;  // max seq no received
    uint_32 m_jitter;   // interarrival jitter
    uint_32 m_ntp_lsr;   // last sr pkg from this source
    uint_32 m_delay_lsr;    // delay since last sr
    uint_8 m_frac_lost; // fraction lost since last sr/rr
};

struct RtcpSR {
    uint_32 m_ssrc;      // ssrc of sender
    RtcpSenderInfo m_sender;
    RtcpReportBlk m_reports[MAX_RTCP_CNT];
};

struct RtcpRR {
    uint_32 m_ssrc;      // ssrc of sender
    RtcpReportBlk m_reports[MAX_RTCP_CNT];
};

struct SdesItem {
    AVal m_desc;
    uint_8 m_type;
};

struct SdesChunk {
    uint_32 m_src;    // type == 0 indicates end of item
    int m_item_cnt;
    SdesItem m_items[MAX_ITEM_SIZE];
};

struct RtcpSdes {
    SdesChunk m_chunks[MAX_RTCP_CNT];
};

struct RtcpBye {
    uint_32 m_srcs[MAX_RTCP_CNT];      // ssrc defined by rc
    AVal m_reason;
};

struct RtcpApp {
    uint_32 m_ssrc;
    AVal m_name;
    AVal m_app;
};

#endif

