#ifndef __RTSPDATA_H__
#define __RTSPDATA_H__
#include<string>
#include"httpdata.h"
#include"socktool.h"


typedef std::string typeStr;
typedef unsigned typeAddr;

static const int MAX_SESS_ID_LEN = 64;
static const int DEF_SESS_NUM_LEN = 8;
static const int MAX_STREAMS_CNT = 20;

static const int MAX_SDP_MEDIA_CNT = 20;
static const int MAX_ENC_NAME_LEN = 32;
static const int MAX_URL_LEN = 256;
static const int MAX_TITLE_LEN = 64;
static const int MAX_COMMENT_LEN = 64;
static const int DEF_SDP_PATH_LEN = 64;

static const char DEF_RTSP_FIELD_CONTENT_TYPE_VAL[] = "application/sdp";

struct SdpHeader {
    int m_version;
    char m_title[MAX_TITLE_LEN];
    char m_comment[MAX_COMMENT_LEN];
    char m_url[MAX_URL_LEN];
    char m_path[DEF_SDP_PATH_LEN];
};

struct AVCodec {
    HttpCache m_config;
    int m_codec_type;
    int m_codec_id;
    int m_payload;
    int m_bit_rate;
    int m_width;
    int m_height;
    int m_sample_rate;
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

struct ResData {
    SdpData* m_sdp;
    char m_id[DEF_SDP_PATH_LEN];
};

struct RtspTransport {
    int m_port_min;
    int m_port_max;
    int m_client_port_min;
    int m_client_port_max;
    int m_server_port_min;
    int m_server_port_max;
    int m_ttl;
    bool m_is_rtp_udp;
    bool m_is_unicast;
    char m_dest[MAX_URL_LEN];
    char m_source[MAX_URL_LEN];
};

struct RtspAuthInfo {
    int m_auth_type;
    char m_realm[256];
};

struct RtspReq {
    Token m_body;
    Token m_method;
    Token m_url;
    RtspTransport m_transport;
    RtspAuthInfo m_auth;
    Token m_accept;
    Token m_content_type;
    Token m_path;
    Token m_host;
    int m_port;
    int m_http_ver;
    int m_seq;
    int m_sid_timeout;
    char m_sid[MAX_SESS_ID_LEN];
};

struct RtspRsp {
    int m_errcode;
    HttpCache m_head;
    HttpCache m_body;
};

struct FFStream {
    const SdpMedia* m_media;
    bool m_on;
    int m_idx;
    int m_client_port_min;
    int m_client_port_max;
    int m_server_port_min;
    int m_server_port_max;
    typeAddr m_server_addr;
    typeAddr m_client_addr;
};

struct SessionCtx {
    FFStream* m_stream[MAX_STREAMS_CNT];
    bool m_is_publisher;
    int m_stream_cnt;
    int m_sid_timeout;
    char m_sid[MAX_SESS_ID_LEN];
    char m_path[DEF_SDP_PATH_LEN];
};


struct SdpParseStat {
    bool m_skip_media;
    int m_ttl;
    char m_ip[DEF_IP_SIZE];
};

struct RtspCtx {
    SessionCtx* m_sess;
    bool m_is_publisher;
    int m_seq; 
    char m_sid[MAX_SESS_ID_LEN];
    char m_path[DEF_SDP_PATH_LEN];
    char m_server_ip[DEF_IP_SIZE];
    char m_client_ip[DEF_IP_SIZE];
};

struct RtspEnv {
    HttpCtx m_http;
    RtspCtx m_rtsp;
};

#endif

