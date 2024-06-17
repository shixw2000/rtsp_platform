#ifndef __RTPDATA_H__
#define __RTPDATA_H__
#include<map>
#include<string>
#include"rtpproto.h"
#include"shareheader.h"
#include"llist.h"
#include"httpdata.h"


typedef std::string typeStr;

static const int MAX_SESS_ID_LEN = 64;
static const int DEF_SESS_NUM_LEN = 8;

static const int MAX_SDP_MEDIA_CNT = 8;
static const int MAX_STREAMS_CNT = 8;

static const int MAX_ENC_NAME_LEN = 32;
static const int MAX_URL_LEN = 256;
static const int MAX_TITLE_LEN = 64;
static const int MAX_COMMENT_LEN = 64;
static const int DEF_SDP_PATH_LEN = 64;
static const int MAX_PATH_SIZE = 256;
static const int MAX_RTCP_AUTH_CNT = 3;


struct SdpMedia;
struct FFStream;
struct InstGroup;
struct InstStream;
struct RtspCtx;

struct StrCmp {
    bool operator() (const char* s1, const char* s2) const;
};

struct AddrCmp {
    bool operator() (const SockAddr* addr1,
        const SockAddr* addr2) const;
};

struct AddrEqu {
    bool operator() (const SockAddr* addr1,
        const SockAddr* addr2) const;
};

typedef std::map<uint_32, const SockAddr*> typeMapSsrc;
typedef std::map<const SockAddr*, FFStream*, AddrCmp> typeMapAddr;
typedef std::map<const char*, InstGroup*, StrCmp> typeMapInst;
typedef std::map<const char*, RtspCtx*, StrCmp> typeMapSess;

struct SdpHeader {
    int m_version;
    char m_title[MAX_TITLE_LEN];
    char m_comment[MAX_COMMENT_LEN];
    char m_url[MAX_URL_LEN];
};

struct AVCodec {
    HttpCache m_config;
    int m_codec_type;
    int m_codec_id;
    int m_sample_rate;
    int m_payload;
    int m_bit_rate;
    int m_width;
    int m_height;
    int m_channels; 
    char m_enc_name[MAX_ENC_NAME_LEN];
};
    
struct SdpMedia {
    AVCodec m_codec;
    bool m_is_udp;
    int m_ttl;
    int m_port;
    int m_idx;
    char m_ip[DEF_IP_SIZE];
    char m_url[MAX_URL_LEN]; 
    char m_path[DEF_SDP_PATH_LEN];
};

struct SdpData {
    SdpHeader m_header;
    SdpMedia* m_media[MAX_SDP_MEDIA_CNT];
    int m_media_cnt;
};

struct TransAddr { 
    int m_rtp_fd;
    int m_rtcp_fd;
    uint_16 m_min_port;
    uint_16 m_max_port; 
    SockAddr m_rtp;
    SockAddr m_rtcp; 
    char m_ip[DEF_IP_SIZE]; 
}; 

struct SdesInfo {
    char m_cname[MAX_CNAME_SIZE];
};

struct RecvStatis {
    /* for receiver reports */ 
    uint_16 m_cycles;
    uint_16 m_max_seq;
    uint_32 m_last_expect_seq; // must be uint_32
    uint_32 m_cumm_lost;
    uint_32 m_cumm_recv;
    uint_32 m_cumm_cycles;

    /* for receive of rtp data */
    bool m_has_rtp;
    uint_32 m_jitter;   
    uint_32 m_rcv_octet_cnt; 
    uint_32 m_rcv_pkg_cnt;
    uint_32 m_last_rtp_unit;
    ClockTime m_last_rtp_ntp;

    /* for receive of sr */
    bool m_has_sr;
    uint_32 m_last_sr_unit;
    RtpTime m_last_sr_rtp;
    ClockTime m_last_rcv_sr_ntp;
    
    uint_32 m_first_sr_unit;
    RtpTime m_first_sr_rtp;
    ClockTime m_first_rcv_sr_ntp;
    
    uint_32 m_last_rcv_octet_cnt;
    ClockTime m_last_rr_ntp;
};

struct SendStatis {
    bool m_ready;
    bool m_has_send_sr;
    uint_32 m_base_unit;
    ClockTime m_first_ntp;

    /* for receive of rr */

    /* for sender of rtp*/
    uint_32 m_last_rtp_unit;
    ClockTime m_last_rtp_ntp;
    
    uint_32 m_snd_pkg_cnt;
    uint_32 m_snd_octet_cnt;
    
    /* for sender of sr */
    uint_32 m_last_snd_octet_cnt;
    ClockTime m_last_sr_ntp;
};

struct InstStat { 
    bool m_setup;
    uint_32 m_ssrc; 
    uint_32 m_seq;
    uint_32 m_base_seq;
    uint_32 m_base_unit;
    TransAddr m_addr;
    SdesInfo m_sdes;
    ClockTime m_first_ntp;
};

struct InstGroup {
    SdpData* m_sdp;
    InstStream* m_stream[MAX_STREAMS_CNT];
    int m_stream_cnt;
    char m_path[MAX_PATH_SIZE];
};

struct InstStream {
    HRoot m_publisher_list;
    HRoot m_player_list;
    const InstGroup* m_grp;
    const SdpMedia* m_media;
    InstStat m_stat;
    char m_path[MAX_PATH_SIZE];
    typeMapSsrc m_map_ssrc;
    typeMapAddr m_map_rtp_addr;
    typeMapAddr m_map_rtcp_addr;
}; 

struct FFStream {
    HList m_node;
    RtspCtx* m_rtsp;
    InstStream* m_inst;
    uint_32 m_ssrc;
    bool m_setup;
    bool m_is_publisher;
    TransAddr m_addr;
    SdesInfo m_sdes; 
    RecvStatis m_rcv_statis;
    SendStatis m_snd_statis; 
};

struct RtspCtx {
    FFStream m_stream[MAX_STREAMS_CNT];
    int m_stream_cnt;
    int m_sid_timeout;
    char m_sid[MAX_SESS_ID_LEN];
};

#endif

