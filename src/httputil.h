#ifndef __HTTPUTIL_H__
#define __HTTPUTIL_H__


struct Cache;
struct NodeMsg;
struct Token;
struct HttpCache; 

static const int DEF_HTTP_CACHE_GROW_LEN = 128;
static const int DEF_HTTP_FORMAT_SIZE = 1024;

class HttpUtil {
public:
    static void init(HttpCache* cache);
    static void release(HttpCache* cache);

    static int addToken(HttpCache* cache, 
        const Token& t);
    
    static int addStr(HttpCache* cache, 
        const char* src, int len = -1);

    static int addChar(HttpCache* cache, char ch);

    static int addInt(HttpCache* cache, int val);

    static int addFormat(HttpCache* cache,
        const char* format, ...);

    static int addRspParam(HttpCache* cache, 
        int ver, int retcode);
    
    static int addReqParam(HttpCache* cache, 
        const Token& method, 
        const Token& url, int ver);

    static int addField(HttpCache* cache, 
        const Token& key, const Token& val);

    static int addFieldInt(HttpCache* cache, 
        const Token& key, int val);

    static int addEnumField(HttpCache* cache, 
        EnumHttpField type, const Token& val);

    static int addEnumFieldInt(HttpCache* cache, 
        EnumHttpField type, int val);

    static int endField(HttpCache* cache); 

    static int totalRd(HttpCache* cache);
    static char* data(HttpCache* cache);
    static const char* data(const HttpCache* cache);
    
    static NodeMsg* genHttpHd(
        HttpCache* head[], int hsize,
        const char* body, int len);

    static NodeMsg* genHttpCache(
        HttpCache* head[], int hsize,
        Cache* cache, int size, int pos = 0);
    
    static void splitUrl(const Token* uri, Token* schema, 
        Token* auth, Token* ip, int* port, Token* path);

    static const char* getStatusStr(int code);
    static const char* getFieldStr(int field);
    static int getFieldType(const Token* token);

    static const char* getVersionStr(int ver);
    static bool findVersion(const Token* t, int* pVer);
  
    static bool prepareBuf(HttpCache* cache, int len);

private:
    static bool grow(HttpCache* cache, 
        int capacity, bool extended);
};

#endif

