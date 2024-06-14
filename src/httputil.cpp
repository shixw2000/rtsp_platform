#include<cstring>
#include<cstdio>
#include<cstdarg>
#include"tokenutil.h"
#include"httpdata.h"
#include"httputil.h"
#include"cache.h"
#include"misc.h"
#include"msgcenter.h"
#include"shareheader.h"


struct HttpInfo {
    int m_code;
    const char* m_message;
};

static const char DEF_UNKNOWN[] = "UNKNOWN";

static const HttpInfo DEF_HTTP_VERSION_STR[] = {
    {HTTP_VER_10_VAL, "HTTP/1.0"},
    {HTTP_VER_11_VAL, "HTTP/1.1"},
    {RTSP_VER_10_VAL, "RTSP/1.0"},

    {0, NULL}
};

static const HttpInfo DEF_HTTP_STATUS_STR[] = {
    {RTSP_STATUS_CONTINUE,               "Continue"},
    {RTSP_STATUS_OK,                     "OK"},
    {RTSP_STATUS_CREATED,                "Created"},
    {RTSP_STATUS_LOW_ON_STORAGE_SPACE,   "Low on Storage Space"},
    {RTSP_STATUS_MULTIPLE_CHOICES,       "Multiple Choices"},
    {RTSP_STATUS_MOVED_PERMANENTLY,      "Moved Permanently"},
    {RTSP_STATUS_MOVED_TEMPORARILY,      "Moved Temporarily"},
    {RTSP_STATUS_SEE_OTHER,              "See Other"},
    {RTSP_STATUS_NOT_MODIFIED,           "Not Modified"},
    {RTSP_STATUS_USE_PROXY,              "Use Proxy"},
    {RTSP_STATUS_BAD_REQUEST,            "Bad Request"},
    {RTSP_STATUS_UNAUTHORIZED,           "Unauthorized"},
    {RTSP_STATUS_PAYMENT_REQUIRED,       "Payment Required"},
    {RTSP_STATUS_FORBIDDEN,              "Forbidden"},
    {RTSP_STATUS_NOT_FOUND,              "Not Found"},
    {RTSP_STATUS_METHOD,                 "Method Not Allowed"},
    {RTSP_STATUS_NOT_ACCEPTABLE,         "Not Acceptable"},
    {RTSP_STATUS_PROXY_AUTH_REQUIRED,    "Proxy Authentication Required"},
    {RTSP_STATUS_REQ_TIME_OUT,           "Request Time-out"},
    {RTSP_STATUS_GONE,                   "Gone"},
    {RTSP_STATUS_LENGTH_REQUIRED,        "Length Required"},
    {RTSP_STATUS_PRECONDITION_FAILED,    "Precondition Failed"},
    {RTSP_STATUS_REQ_ENTITY_2LARGE,      "Request Entity Too Large"},
    {RTSP_STATUS_REQ_URI_2LARGE,         "Request URI Too Large"},
    {RTSP_STATUS_UNSUPPORTED_MTYPE,      "Unsupported Media Type"},
    {RTSP_STATUS_PARAM_NOT_UNDERSTOOD,   "Parameter Not Understood"},
    {RTSP_STATUS_CONFERENCE_NOT_FOUND,   "Conference Not Found"},
    {RTSP_STATUS_BANDWIDTH,              "Not Enough Bandwidth"},
    {RTSP_STATUS_SESSION,                "Session Not Found"},
    {RTSP_STATUS_STATE,                  "Method Not Valid in This State"},
    {RTSP_STATUS_INVALID_HEADER_FIELD,   "Header Field Not Valid for Resource"},
    {RTSP_STATUS_INVALID_RANGE,          "Invalid Range"},
    {RTSP_STATUS_RONLY_PARAMETER,        "Parameter Is Read-Only"},
    {RTSP_STATUS_AGGREGATE,              "Aggregate Operation no Allowed"},
    {RTSP_STATUS_ONLY_AGGREGATE,         "Only Aggregate Operation Allowed"},
    {RTSP_STATUS_TRANSPORT,              "Unsupported Transport"},
    {RTSP_STATUS_UNREACHABLE,            "Destination Unreachable"},
    {RTSP_STATUS_INTERNAL,               "Internal Server Error"},
    {RTSP_STATUS_NOT_IMPLEMENTED,        "Not Implemented"},
    {RTSP_STATUS_BAD_GATEWAY,            "Bad Gateway"},
    {RTSP_STATUS_SERVICE,                "Service Unavailable"},
    {RTSP_STATUS_GATEWAY_TIME_OUT,       "Gateway Time-out"},
    {RTSP_STATUS_VERSION,                "RTSP Version not Supported"},
    {RTSP_STATUS_UNSUPPORTED_OPTION,     "Option not supported"},

    {0, NULL}
};

static const HttpInfo DEF_HTTP_FIELD_NAME[] = 
{ 
    {ENUM_HTTP_CONTENT_LENGTH, "Content-Length"},
    {ENUM_HTTP_SESSION, "Session"}, 
    {ENUM_HTTP_CONTENT_TYPE, "Content-Type"},
    {ENUM_HTTP_CONTENT_BASE, "Content-Base"},
    {ENUM_HTTP_LOCATION, "Location"},
    
    {ENUM_HTTP_SERVER, "Server"}, 
    {ENUM_HTTP_SEQ, "CSeq"},
    {ENUM_HTTP_ACCEPT, "Accept"},
    {ENUM_HTTP_DATE, "Date"},
    {ENUM_HTTP_TRANSPORT, "Transport"},
    {ENUM_HTTP_PUBLIC, "Public"},

    {ENUM_HTTP_WWW_AUTH, "WWW-Authenticate"},
    {ENUM_HTTP_KEEP_ALIVE, "Keep-Alive"},
    {ENUM_HTTP_CONNECTION, "Connection"},
    {ENUM_HTTP_TRANSFER_ENCODING, "Transfer-Encoding"},
    {ENUM_HTTP_CONTENT_ENCODING, "Content-Encoding"},

    {0, NULL}
};

const char* HttpUtil::getStatusStr(int code) {
    const HttpInfo* p = NULL;

    for (p = DEF_HTTP_STATUS_STR; NULL != p->m_message; ++p) {
        if (code == p->m_code) {
            return p->m_message;
        }
    }

    return DEF_UNKNOWN;
}

const char* HttpUtil::getFieldStr(int field) {
    const HttpInfo* p = NULL;

    for (p = DEF_HTTP_FIELD_NAME; NULL != p->m_message; ++p) {
        if (field == p->m_code) {
            return p->m_message;
        }
    }

    return DEF_UNKNOWN;
}

int HttpUtil::getFieldType(const Token* token) {
    const HttpInfo* p = NULL;

    for (p = DEF_HTTP_FIELD_NAME; NULL != p->m_message; ++p) {
        if (TokenUtil::strCmp(token, p->m_message, true)) {
            return p->m_code;
        }
    }

    return -1;
}

const char* HttpUtil::getVersionStr(int ver) {
    const HttpInfo* p = NULL;

    for (p = DEF_HTTP_VERSION_STR; NULL != p->m_message; ++p) {
        if (ver == p->m_code) {
            return p->m_message;
        }
    }

    return DEF_UNKNOWN;
}

bool HttpUtil::findVersion(const Token* t, int* pVer) {
    const HttpInfo* p = NULL;

    for (p = DEF_HTTP_VERSION_STR; NULL != p->m_message; ++p) {
        if (TokenUtil::strCmp(t, p->m_message, true)) {
            if (NULL != pVer) {
                *pVer = p->m_code;
            }

            return true;
        }
    }

    return false;
}

void HttpUtil::splitUrl(const Token* uri, Token* schema, 
    Token* auth, Token* ip, int* pPort, Token* path) {
    int pos = 0;
    Token token(*uri);
    Token t[4];
    int port = 0;

    TokenUtil::strip(&token);

    pos = TokenUtil::find(&token, DEF_URL_SEP_CHAR);
    if (0 < pos && DEF_COLON_CHAR == token.m_str[pos-1]
        && pos + 1 < token.m_len &&
        DEF_URL_SEP_CHAR == token.m_str[pos+1]) {
        /* find a schema :// */
        TokenUtil::substr(&t[0], &token, 0, pos-1);
        
        TokenUtil::skip(&token, pos+2); 

        /* get path */
        pos = TokenUtil::findFirstOf(&token, "/?");
        if (0 <= pos) { 
            TokenUtil::substr(&t[1], &token, pos); 

            token.set(token.m_str, pos);
        } else {
            /* no path */
        } 
    } else {
        /* no schema, only path */
        t[1] = token;
        token.set(NULL, 0);
    } 
    
    /* get auth */
    pos = TokenUtil::find(&token, DEF_AT_CHAR);
    if (0 <= pos) {
        TokenUtil::substr(&t[2], &token, 0, pos);
        
        TokenUtil::skip(&token, pos+1);
    }

    /* get host, port */
    pos = TokenUtil::find(&token, DEF_COLON_CHAR);
    if (0 <= pos) {
        TokenUtil::substr(&t[3], &token, 0, pos);
        TokenUtil::skip(&token, pos+1);

        TokenUtil::toNum(&port, &token);
    } else {
        t[3] = token;
    }

    TokenUtil::skipOf(&t[3], "[");
    TokenUtil::skipLastOf(&t[3], "]");

    if (NULL != schema) {
        *schema = t[0];
    }

    if (NULL != path) {
        *path = t[1];
    }

    if (NULL != auth) {
        *auth = t[2];
    }

    if (NULL != ip) {
        *ip = t[3];
    }

    if (NULL != pPort) {
        *pPort = port;
    }
}

int HttpUtil::totalRd(HttpCache* cache) {
    return cache->m_size;
}

char* HttpUtil::data(HttpCache* cache) {
    if (NULL != cache->m_cache) {
        return CacheUtil::data(cache->m_cache);
    } else {
        return NULL;
    }
}

const char* HttpUtil::data(const HttpCache* cache) {
    if (NULL != cache->m_cache) {
        return CacheUtil::data((Cache*)cache->m_cache);
    } else {
        return NULL;
    }
}

NodeMsg* HttpUtil::genHttpHd(
    HttpCache* head[], int hsize,
    const char* body, int blen) {
    NodeMsg* msg = NULL;
    char* dst = NULL;
    const char* src = NULL;
    int total = 0; 
    int hlen = 0;
    int len = 0;

    if (0 < hsize) {
        /* add contont-length field */
        addEnumFieldInt(head[hsize-1], 
            ENUM_HTTP_CONTENT_LENGTH, blen);
        
        /* add end line */
        endField(head[hsize-1]);

        for (int i=0; i<hsize; ++i) {
            hlen += totalRd(head[i]);
        }
    }

    total = hlen + blen;
    if (0 < total) {
        msg = MsgCenter::allocHttpMsg(total);
        dst = MsgCenter::getMsg(msg);

        if (0 < hlen) {
            for (int i=0; i<hsize; ++i) {
                len = totalRd(head[i]);
                if (0 < len) {
                    src = data(head[i]); 
                    MiscTool::bcopy(dst, src, len);

                    dst += len;
                }
            } 
        }

        if (0 < blen) {
            MiscTool::bcopy(dst, body, blen);

            dst += blen;
        }
    }

    return msg;
}

NodeMsg* HttpUtil::genHttpCache(
    HttpCache* head[], int hsize,
    Cache* cache, int size, int pos) {
    NodeMsg* msg = NULL;
    Cache* ref = NULL;
    char* dst = NULL;
    const char* src = NULL;
    int hlen = 0;
    int blen = 0;
    int len = 0;

    if (0 <= pos && pos < size) {
        blen = size - pos;
    } else {
        blen = 0;
    }

    if (0 < hsize) {
        /* add contont-length field */
        addEnumFieldInt(head[hsize-1], 
            ENUM_HTTP_CONTENT_LENGTH, blen);
        
        /* add end line */
        endField(head[hsize-1]);

        for (int i=0; i<hsize; ++i) {
            hlen += totalRd(head[i]);
        }
    }

    if (0 < hlen) {
        msg = MsgCenter::allocHttpMsg(hlen);
        dst = MsgCenter::getMsg(msg); 

        for (int i=0; i<hsize; ++i) {
            len = totalRd(head[i]);
            if (0 < len) {
                src = data(head[i]); 
                MiscTool::bcopy(dst, src, len);

                dst += len;
            }
        } 

        if (0 < blen) {
            ref = CacheUtil::ref(cache);
            MsgCenter::setCache(msg, ref, size, true); 
            MsgCenter::setMsgPos(msg, pos, true);
        }
    }

    return msg;
}

int HttpUtil::addField(HttpCache* cache, 
    const Token& key, const Token& val) {
    int total = 0; 

    total += addToken(cache, key);
    total += addStr(cache, ": ");
    total += addToken(cache, val);
    total += addStr(cache, DEF_HTTP_NEWLINE);
    return total;
}

int HttpUtil::addFieldInt(HttpCache* cache, 
    const Token& key, int val) {
    int total = 0;

    total += addToken(cache, key);
    total += addStr(cache, ": ");
    total += addInt(cache, val);
    total += addStr(cache, DEF_HTTP_NEWLINE);
    return total;
}

int HttpUtil::addEnumField(HttpCache* cache, 
    EnumHttpField type, const Token& val) {
    const char* psz = NULL;

    psz = getFieldStr(type);
    return addField(cache, psz, val);
}

int HttpUtil::addEnumFieldInt(HttpCache* cache, 
    EnumHttpField type, int val) {
    const char* psz = NULL;

    psz = getFieldStr(type);
    return addFieldInt(cache, psz, val);
}

int HttpUtil::addRspParam(HttpCache* cache, 
    int ver, int retcode) {
    const char* psz = NULL;
    int total = 0;
    
    psz = getVersionStr(ver);
    total += addStr(cache, psz);
    total += addChar(cache, DEF_BLANK_CHAR);
    
    total += addInt(cache, retcode);
    total += addChar(cache, DEF_BLANK_CHAR);
    
    psz = getStatusStr(retcode);
    total += addStr(cache, psz);
    total += addStr(cache, DEF_HTTP_NEWLINE);

    return total;
}

int HttpUtil::addReqParam(HttpCache* cache, 
    const Token& method, const Token& url, int ver) {
    const char* psz = NULL;
    int total = 0;
    
    total += addToken(cache, method);
    total += addChar(cache, DEF_BLANK_CHAR);
    
    total += addToken(cache, url);
    total += addChar(cache, DEF_BLANK_CHAR);
    
    psz = getVersionStr(ver);
    total += addStr(cache, psz);
    total += addStr(cache, DEF_HTTP_NEWLINE);

    return total;
}

int HttpUtil::endField(HttpCache* cache) {
    return addStr(cache, DEF_HTTP_NEWLINE);
}

void HttpUtil::init(HttpCache* cache) {
    MiscTool::bzero(cache, sizeof(HttpCache));
}

void HttpUtil::release(HttpCache* cache) {
    if (NULL != cache->m_cache) {
        CacheUtil::del(cache->m_cache);
    }
     
    MiscTool::bzero(cache, sizeof(HttpCache));
}

bool HttpUtil::grow(HttpCache* cache, 
    int capacity, bool extended) {
    Cache* c = NULL;
    bool bOk = false;

    if (0 < capacity && cache->m_capacity < capacity) {
        if (extended) {
            /* growth more 32 byte for next time */
            capacity += DEF_CACHE_EXTENDED_SIZE;
        }
        
        if (NULL != cache->m_cache) {
            c = CacheUtil::realloc(cache->m_cache, 
                capacity, cache->m_size); 
        } else {
            /* init state */
            c = CacheUtil::alloc(capacity); 
        }

        if (NULL != c) {
            cache->m_cache = c;
            cache->m_capacity = capacity;
            bOk = true;
        }
    } else {
        bOk = true;
    }

    return bOk;
}

bool HttpUtil::prepareBuf(HttpCache* cache, int len) {
    return grow(cache, cache->m_capacity + len, false);
}

int HttpUtil::addToken(HttpCache* cache, 
    const Token& t) {
    char* psz = NULL;
    int max = 0;
    bool bOk = false;
    
    if (0 < t.m_len) {
        max = cache->m_size + t.m_len;
        if (cache->m_capacity < max) {
            bOk = grow(cache, max, true);
            if (!bOk) {
                return 0;
            }
        }

        psz = data(cache);
        
        MiscTool::bcopy(psz + cache->m_size,
            t.m_str, t.m_len);
        
        cache->m_size += t.m_len;

        return t.m_len;
    } else {
        return 0;
    }
}

int HttpUtil::addStr(HttpCache* cache,
    const char* src, int len) {
    char* psz = NULL;
    int max = 0;
    bool bOk = false;
    
    if (NULL != src) {
        if (0 > len) {
            len = (int)strlen(src);
        }
        
        if (0 < len) {
            max = cache->m_size + len;
            if (cache->m_capacity < max) {
                bOk = grow(cache, max, true);
                if (!bOk) {
                    return 0;
                }
            }

            psz = data(cache);
            
            MiscTool::bcopy(psz + cache->m_size, src, len); 
            cache->m_size += len;
        } 
    } else {
        len = 0;
    }

    return len;
}

int HttpUtil::addChar(HttpCache* cache, char ch) {
    char* psz = NULL;
    int max = cache->m_size + 1;
    bool bOk = false;
    
    if (cache->m_capacity < max) {
        bOk = grow(cache, max, true);
        if (!bOk) {
            return 0;
        }
    }

    psz = data(cache);
    psz[cache->m_size++] = ch;
    return 1;
}

int HttpUtil::addInt(HttpCache* cache, int val) {
    int len = 0;
    char tmp[32] = {0};
    
    len = TokenUtil::toStr(tmp, sizeof(tmp), val); 

    addStr(cache, tmp, len);
    return len;
}

int HttpUtil::addFormat(HttpCache* cache,
    const char* format, ...) {
    int len = 0;
    va_list ap;
    char buf[DEF_HTTP_FORMAT_SIZE] = {0};

    va_start(ap, format);
    len = vsnprintf(buf, DEF_HTTP_FORMAT_SIZE,
        format, ap);
    va_end(ap);
    
    if (0 <= len && len < DEF_HTTP_FORMAT_SIZE) {
        addStr(cache, buf, len);
    } else {
        len = 0;
    }

    return len;
}

