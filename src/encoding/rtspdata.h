#ifndef __RTSPDATA_H__
#define __RTSPDATA_H__
#include<string>
#include"httpdata.h"
#include"socktool.h"
#include"rtpdata.h"


static const char DEF_RTSP_FIELD_CONTENT_TYPE_VAL[] = "application/sdp";

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
    int m_seq;
    HttpCache m_head;
    HttpCache m_body;
}; 


#endif

