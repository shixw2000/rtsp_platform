#ifndef __HTTPTOOL_H__
#define __HTTPTOOL_H__


struct Range;
struct Token;
struct HttpCtx;
class HttpCenter;

enum EnumHttpErr {
    ENUM_HTTP_ERR_BEGIN = 10000,

    ENUM_HTTP_ERR_HD,
    ENUM_HTTP_ERR_CMDLINE,
    ENUM_HTTP_ERR_ALLOC_HD,
    ENUM_HTTP_ERR_ALLOC_BODY,
    ENUM_HTTP_ERR_FIELD,
    ENUM_HTTP_ERR_VERSION,
    ENUM_HTTP_ERR_ZERO_LINE,
    ENUM_HTTP_ERR_CONTENT_LEN,
    ENUM_HTTP_ERR_CHUNK_LEN,
    ENUM_HTTP_ERR_CHUNK_BODY,
    ENUM_HTTP_ERR_RD_STAT,
    ENUM_HTTP_ERR_NEWLINE,
    ENUM_HTTP_ERR_DISPATCH,

    ENUM_HTTP_ERR_END = 19999,
};

class HttpTool { 
    enum EnumRdHttp {
        ENUM_RD_HTTP_HD = 0,
        ENUM_RD_POST_HD,
        
        ENUM_RD_PREPARE_BODY,
        ENUM_RD_CONTENT,
        
        ENUM_RD_CHUNK_HD,
        ENUM_RD_PREPARE_CHUNK,
        ENUM_RD_CHUNK_BODY,
        ENUM_RD_CHUNK_CRLF,
        ENUM_RD_CHUNK_TAIL,
        
        ENUM_RD_FRAME_COMPLETED,
        ENUM_RD_CHUNK_COMPLETED,
        ENUM_RD_COMPLETED,

        ENUM_RD_END
    }; 
    
public:
    HttpTool(HttpCenter* center);

    int parseData(int fd, HttpCtx* ctx, const char* buf, int size); 
    
    bool isChunk(const Token* key, const Token* val);

    bool isEncoded(const Token* key, 
        const Token* val, int* ptype);
    
    bool isContentLen(const Token* key); 

    bool splitCmdLine(Token& t1, Token& t2, Token& token);

    bool splitHeadLine(Token& key, Token& val, Token& token);
    
private:
    bool isValidHead(HttpCtx* ctx, int lineNo, int total);

    bool parseChunkLen(HttpCtx* ctx, Token* t, int* plen);
    bool parseContentLen(HttpCtx* ctx, Token* t, int* plen);
    
    bool readCRLF(HttpCtx* ctx, Token* src, bool end);

    bool readBodyData(HttpCtx* ctx, Token* src); 

    bool analyseHead(HttpCtx* ctx, int start,
        int lines[], int lineCnt);

    bool readHead(HttpCtx* ctx, Token* t); 
    bool readPostHd(HttpCtx* ctx);
    
    bool prepareBody(HttpCtx* ctx);
    bool readBody(HttpCtx* ctx, Token* src); 
    
    bool readChunkHd(HttpCtx* ctx, Token* src);
    bool prepareChunk(HttpCtx* ctx);
    bool readChunk(HttpCtx* ctx, Token* src); 
    bool readChunkCRLF(HttpCtx* ctx, Token* src);
    bool readChunkTail(HttpCtx* ctx, Token* src); 

    bool completeFrame(int fd, HttpCtx* ctx);
    bool completeChunk(int fd, HttpCtx* ctx);
    bool completeMsg(int fd, HttpCtx* ctx);
    bool dispatchFrame(int fd, HttpCtx* ctx, bool bEnd);

private:
    HttpCenter* m_center;
};

#endif

