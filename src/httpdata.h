#ifndef __HTTPDATA_H__
#define __HTTPDATA_H__


static const int MAX_LINE_CNT = 32;
static const int DEF_CACHE_EXTENDED_SIZE = 32;
static const int MAX_CHUNK_HD_LEN = 10;
static const int MAX_HEAD_SIZE = 1024 * 5;
static const int MAX_BODY_SIZE = 1024 * 1024 * 10;
static const int MAX_CONTENT_SIZE = 1024 * 1024 * 1024;

static const char DEF_SPACE_DELIM[] = " \r\n\t\f\v";
static const char DEF_HTTP_NEWLINE[] = "\r\n";
static const int DEF_HTTP_NEWLINE_CNT = 2;
static const char DEF_URL_SEP_CHAR = '/';
static const char DEF_QUESTION_MARK_CHAR = '?';
static const char DEF_COLON_CHAR = ':';
static const char DEF_COMMA_CHAR = ',';
static const char DEF_SEMICOLON_CHAR = ';';
static const char DEF_AT_CHAR = '@';
static const char DEF_BLANK_CHAR = ' ';
static const char DEF_NULL_CHAR = '\0';
static const char DEF_EQUATION_CHAR = '=';
static const char DEF_CR_CHAR = '\r';
static const char DEF_LF_CHAR = '\n';

enum EnumHttpVersion {
    HTTP_VER_10_VAL = 10000,
    HTTP_VER_11_VAL = 10001,
    RTSP_VER_10_VAL = 20000,
};

enum EnumHttpField {
    ENUM_HTTP_CONTENT_LENGTH = 0,
    ENUM_HTTP_SESSION, 
    ENUM_HTTP_CONTENT_TYPE,
    ENUM_HTTP_CONTENT_BASE, 
    ENUM_HTTP_LOCATION,
    
    ENUM_HTTP_SERVER, 
    ENUM_HTTP_SEQ,
    ENUM_HTTP_ACCEPT,
    ENUM_HTTP_DATE,
    ENUM_HTTP_TRANSPORT,
    ENUM_HTTP_PUBLIC,

    ENUM_HTTP_WWW_AUTH, 

    ENUM_HTTP_KEEP_ALIVE,
    ENUM_HTTP_CONNECTION,
    ENUM_HTTP_TRANSFER_ENCODING,
    ENUM_HTTP_CONTENT_ENCODING,
    
    ENUM_HTTP_FIELD_END
};

enum RTSPStatusCode {
    RTSP_STATUS_CONTINUE             =100,
    RTSP_STATUS_OK                   =200,
    RTSP_STATUS_CREATED              =201,
    RTSP_STATUS_LOW_ON_STORAGE_SPACE =250,
    RTSP_STATUS_MULTIPLE_CHOICES     =300,
    RTSP_STATUS_MOVED_PERMANENTLY    =301,
    RTSP_STATUS_MOVED_TEMPORARILY    =302,
    RTSP_STATUS_SEE_OTHER            =303,
    RTSP_STATUS_NOT_MODIFIED         =304,
    RTSP_STATUS_USE_PROXY            =305,
    RTSP_STATUS_BAD_REQUEST          =400,
    RTSP_STATUS_UNAUTHORIZED         =401,
    RTSP_STATUS_PAYMENT_REQUIRED     =402,
    RTSP_STATUS_FORBIDDEN            =403,
    RTSP_STATUS_NOT_FOUND            =404,
    RTSP_STATUS_METHOD               =405,
    RTSP_STATUS_NOT_ACCEPTABLE       =406,
    RTSP_STATUS_PROXY_AUTH_REQUIRED  =407,
    RTSP_STATUS_REQ_TIME_OUT         =408,
    RTSP_STATUS_GONE                 =410,
    RTSP_STATUS_LENGTH_REQUIRED      =411,
    RTSP_STATUS_PRECONDITION_FAILED  =412,
    RTSP_STATUS_REQ_ENTITY_2LARGE    =413,
    RTSP_STATUS_REQ_URI_2LARGE       =414,
    RTSP_STATUS_UNSUPPORTED_MTYPE    =415,
    RTSP_STATUS_PARAM_NOT_UNDERSTOOD =451,
    RTSP_STATUS_CONFERENCE_NOT_FOUND =452,
    RTSP_STATUS_BANDWIDTH            =453,
    RTSP_STATUS_SESSION              =454,
    RTSP_STATUS_STATE                =455,
    RTSP_STATUS_INVALID_HEADER_FIELD =456,
    RTSP_STATUS_INVALID_RANGE        =457,
    RTSP_STATUS_RONLY_PARAMETER      =458,
    RTSP_STATUS_AGGREGATE            =459,
    RTSP_STATUS_ONLY_AGGREGATE       =460,
    RTSP_STATUS_TRANSPORT            =461,
    RTSP_STATUS_UNREACHABLE          =462,
    RTSP_STATUS_INTERNAL             =500,
    RTSP_STATUS_NOT_IMPLEMENTED      =501,
    RTSP_STATUS_BAD_GATEWAY          =502,
    RTSP_STATUS_SERVICE              =503,
    RTSP_STATUS_GATEWAY_TIME_OUT     =504,
    RTSP_STATUS_VERSION              =505,
    RTSP_STATUS_UNSUPPORTED_OPTION   =551,
};

struct Cache;

struct Range {
    int m_beg;
    int m_len;
};

struct Token {
    char* m_str;
    int m_len;

    Token();
    Token(const char* str, int size = -1);
    void set(const char* str, int size = -1);
};

struct HttpCache {
    Cache* m_cache;
    int m_capacity;
    int m_size;
};

struct HttpCtx { 
    HttpCache m_head;
    HttpCache m_body;
    Range m_ranges[MAX_LINE_CNT * 2];
    int m_err_code;
    int m_rd_stat;
    int m_line_no;
    int m_rd_len;
    int m_http_ver;
    int m_frame_beg;
    int m_content_len; 
    bool m_is_chunk;
    bool m_is_req;
    char m_tmp[MAX_CHUNK_HD_LEN];
}; 

#endif

