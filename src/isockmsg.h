#ifndef __ISOCKMSG_H__
#define __ISOCKMSG_H__
#include"httpdata.h"


/* message from here alignment with 1 byte */
#pragma pack(push, 1) 

#pragma pack(pop)


struct PreHeadHttp {
    int m_type;
} __attribute__((aligned(8)));

enum EnumHTTPType {
    ENUM_HTTP_FRAME_HD,
    ENUM_HTTP_FRAME_MID,
    ENUM_HTTP_MSG,
};

struct HttpFrameHd {
    Cache* m_head;
    Cache* m_body;
    bool m_is_req;
    bool m_is_end;
    bool m_is_chunk;
    unsigned m_seq;
    int m_http_ver;
    int m_content_len;
    int m_hd_size;
    int m_body_size;
    int m_line_no;
    Range m_hd_opts[0];
};

struct HttpFrameMid {
    Cache* m_body;
    bool m_is_end;
    bool m_is_chunk;
    unsigned m_seq;
    int m_frame_beg;
    int m_body_size;
};


#endif

