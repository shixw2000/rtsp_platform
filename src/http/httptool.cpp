#include<cstring>
#include<cstdlib>
#include"httpdata.h"
#include"httptool.h"
#include"httpcenter.h"
#include"cache.h"
#include"misc.h"
#include"tokenutil.h"
#include"httputil.h"
#include"shareheader.h"


#define CHK_TOKEN(t) if (TokenUtil::isEmpty(t)) return false
#define SET_STAT(ctx, val) ((ctx)->m_rd_stat = (val))
#define SET_ERR(ctx, err) ((ctx)->m_err_code = (err))
#define IS_VALID_STAT(ctx) (0 == (ctx)->m_err_code)

HttpTool::HttpTool(HttpCenter* center) 
    : m_center(center) {
} 

int HttpTool::parseData(int fd, HttpCtx* ctx, 
    const char* buf, int size) {
    bool bOk = true;
    Token token(buf, size);

    while (bOk && IS_VALID_STAT(ctx)) {
        switch (ctx->m_rd_stat) {
        case ENUM_RD_HTTP_HD:
            bOk = readHead(ctx, &token);
            break;

        case ENUM_RD_POST_HD:
            bOk = readPostHd(ctx);
            break;

        case ENUM_RD_PREPARE_BODY:
            bOk = prepareBody(ctx);
            break;

        case ENUM_RD_CONTENT:
            bOk = readBody(ctx, &token);
            break;

        case ENUM_RD_FRAME_COMPLETED:
            bOk = completeFrame(fd, ctx);
            break;

        case ENUM_RD_CHUNK_HD:
            bOk = readChunkHd(ctx, &token);
            break;

        case ENUM_RD_PREPARE_CHUNK:
            bOk = prepareChunk(ctx);
            break;

        case ENUM_RD_CHUNK_BODY:
            bOk = readChunk(ctx, &token);
            break;

        case ENUM_RD_CHUNK_CRLF:
            bOk = readChunkCRLF(ctx, &token);
            break;

        case ENUM_RD_CHUNK_TAIL:
            bOk = readChunkTail(ctx, &token);
            break;

        case ENUM_RD_CHUNK_COMPLETED:
            bOk = completeChunk(fd, ctx);
            break;

        case ENUM_RD_COMPLETED:
            bOk = completeMsg(fd, ctx);
            break;

        default:
            SET_ERR(ctx, ENUM_HTTP_ERR_RD_STAT);
            break;
        }
    }

    if (IS_VALID_STAT(ctx)) {
        return 0;
    } else {
        LOG_INFO("parse_data| rd_stat=%d| err_code=%d|"
            " origin_len=%d| left_len=%d| rd_len=%d|"
            " line_no=%d| content_len=%d|"
            " hd_size=%d| body_size=%d|"
            " frame_beg=%d|",
            ctx->m_rd_stat, ctx->m_err_code,
            size, token.m_len, ctx->m_rd_len,
            ctx->m_line_no, ctx->m_content_len,
            HttpUtil::totalRd(&ctx->m_head), 
            HttpUtil::totalRd(&ctx->m_body),
            ctx->m_frame_beg);

        return ctx->m_err_code;
    }
}

bool HttpTool::analyseHead(HttpCtx* ctx, 
    int line_start, int lines[], int lineCnt) {
    char* origin = NULL;
    char* psz = NULL;
    int ver = 0;
    int index = 0;
    bool bOk = false;
    Token t[2];
    Token token;

    origin = HttpUtil::data(&ctx->m_head);
    psz = origin + line_start;
    index = ctx->m_line_no * 2;

    for (int i=0; i<lineCnt; ++i) {
        token.set(psz, lines[i]);
        psz += lines[i]; 

        if (0 < ctx->m_line_no) {
            bOk = splitHeadLine(t[0], t[1], token);
            if (!bOk) {
                SET_ERR(ctx, ENUM_HTTP_ERR_FIELD);
                break;
            } 
        } else {
            bOk = splitCmdLine(t[0], t[1], token);
            if (!bOk) {
                SET_ERR(ctx, ENUM_HTTP_ERR_CMDLINE);
                break;
            }

            if (HttpUtil::findVersion(&t[0], &ver)) {
                ctx->m_http_ver = ver;
                ctx->m_is_req = false;

                t[0] = t[1];
                t[1] = token;
            } else if (HttpUtil::findVersion(&token, &ver)) {
                ctx->m_http_ver = ver;
                ctx->m_is_req = true;
            } else {
                SET_ERR(ctx, ENUM_HTTP_ERR_VERSION);
                break;
            }
        }

        TokenUtil::token2Range(&ctx->m_ranges[index],
                origin, &t[0]);
        TokenUtil::token2Range(&ctx->m_ranges[index+1], 
            origin, &t[1]);
        
        ++ctx->m_line_no;
        index += 2;
    } 
    
    return IS_VALID_STAT(ctx);
}

bool HttpTool::readPostHd(HttpCtx* ctx) {
    const char* origin = NULL;
    int clen = 0;
    int index = 0;
    bool bOk = false;
    Token key;
    Token val;
    
    if (!(0 < ctx->m_line_no)) {
        SET_ERR(ctx, ENUM_HTTP_ERR_ZERO_LINE);
        return false;
    }
    
    /* try to find len line */
    origin = HttpUtil::data(&ctx->m_head);
    index = 2;
    for (int i=1; i<ctx->m_line_no; ++i, index+=2) {
        TokenUtil::range2Token(&key, origin,
                &ctx->m_ranges[index]);
        TokenUtil::range2Token(&val, origin, 
            &ctx->m_ranges[index+1]);

        if (isContentLen(&key)) {
            bOk = parseContentLen(ctx, &val, &clen);
            if (bOk) {
                ctx->m_content_len = clen;
                break;
            } else {
                SET_ERR(ctx, ENUM_HTTP_ERR_CONTENT_LEN);
                return false;
            } 
        } else if (isChunk(&key, &val)) {
            ctx->m_is_chunk = true;
            break;
        } else {
            continue;
        }
    } 

    if (!ctx->m_is_chunk) {
        ctx->m_frame_beg = 0; 
        ctx->m_rd_len = 0;

        if (0 < ctx->m_content_len) {
            SET_STAT(ctx, ENUM_RD_PREPARE_BODY);
        } else {
            SET_STAT(ctx, ENUM_RD_COMPLETED);
        }
    } else {
        ctx->m_frame_beg = 0;
        ctx->m_rd_len = 0;
        ctx->m_content_len = 0;

        SET_STAT(ctx, ENUM_RD_CHUNK_HD);
    }

    return true;
}

bool HttpTool::prepareBody(HttpCtx* ctx) {
    bool bOk = false;
    int len = 0;

    len = ctx->m_content_len - ctx->m_frame_beg; 
    if (MAX_BODY_SIZE < len) {
        len = MAX_BODY_SIZE;
    }

    bOk = HttpUtil::prepareBuf(&ctx->m_body, len); 
    if (bOk) {
        ctx->m_rd_len = len;
        SET_STAT(ctx, ENUM_RD_CONTENT);
    } else {
        SET_ERR(ctx, ENUM_HTTP_ERR_ALLOC_BODY);
    }

    return bOk;
}

bool HttpTool::isValidHead(HttpCtx* ctx, 
    int lineNo, int total) {
    if (MAX_LINE_CNT > ctx->m_line_no + lineNo
        && MAX_HEAD_SIZE > 
        HttpUtil::totalRd(&ctx->m_head) + total) {
        return true;
    } else {
        return false;
    }
}

bool HttpTool::parseContentLen(HttpCtx* ctx,
    Token* t, int* plen) {
    int len = 0;
    bool bOk = false;

    bOk = TokenUtil::toNum(&len, t);
    if (bOk) {
        if (0 <= len && MAX_CONTENT_SIZE > len) {
            *plen = len; 
        } else {
            SET_ERR(ctx, -1);
        }
    } else {
        SET_ERR(ctx, -1);
    }

    return IS_VALID_STAT(ctx);
}

bool HttpTool::parseChunkLen(HttpCtx* ctx,
    Token* t, int* plen) {
    int len = 0;
    bool bOk = false;

    bOk = TokenUtil::toHexNum(&len, t);
    if (bOk) {
        if (0 <= len  && MAX_CONTENT_SIZE >
            len + ctx->m_content_len) {
            *plen = len; 

            bOk = true;
        } else {
            bOk = false;
        }
    } 

    return bOk;
}

bool HttpTool::readHead(HttpCtx* ctx, Token* t) {
    char* psz = NULL;
    int total = 0;
    int pos = 0;
    int lastChar = 0;
    int line_start = 0;
    int lineNo = 0;
    int lines[MAX_LINE_CNT] = {0};

    CHK_TOKEN(t);

    line_start = HttpUtil::totalRd(&ctx->m_head);
    if (0 < ctx->m_rd_len) { 
        lines[0] = ctx->m_rd_len;
        
        psz = HttpUtil::data(&ctx->m_head); 
        lastChar = psz[line_start  - 1]; 

        line_start -= ctx->m_rd_len;
    } 

    /* mark origin str */
    psz = t->m_str;
    
    while (!TokenUtil::isEmpty(t)) { 
        pos = TokenUtil::find(t, DEF_LF_CHAR);
        if (0 <= pos) {
            if (0 < pos) {
                lastChar = t->m_str[pos-1];
            } 
            
            /* find a line ended with '\r\n' */
            if (TokenUtil::isCR(lastChar)) {

                /* reset last */
                lastChar = 0;
                
                ++pos; 
                total += pos;
                lines[lineNo] += pos;
                TokenUtil::skip(t, pos);
                
                if (DEF_HTTP_NEWLINE_CNT < lines[lineNo]) { 
                    ++lineNo;
                    lines[lineNo] = 0;

                    if (!isValidHead(ctx, lineNo, total)) {
                        /* too many head lines */
                        SET_ERR(ctx, ENUM_HTTP_ERR_HD);
                        break;
                    }
                } else {
                    /* find an empty line */
                    lines[lineNo] = 0;
                    SET_STAT(ctx, ENUM_RD_POST_HD);
                    break;
                } 
            } else {
                /* invalid '\n' without '\r' */
                SET_ERR(ctx, ENUM_HTTP_ERR_NEWLINE);
                break;
            }
        } else {
            total += t->m_len;
            lines[lineNo] += t->m_len; 
            TokenUtil::skip(t, t->m_len);

            if (!isValidHead(ctx, lineNo, total)) { 
                SET_ERR(ctx, ENUM_HTTP_ERR_HD);
            } 

            break;
        }
    } 

    if (0 < total && IS_VALID_STAT(ctx)) { 
        if (0 < HttpUtil::addStr(&ctx->m_head, psz, total)) {
            ctx->m_rd_len = lines[lineNo];
            if (0 < lineNo) {
                analyseHead(ctx, line_start, lines, lineNo);
            } 
        } else {
            SET_ERR(ctx, ENUM_HTTP_ERR_ALLOC_HD);
        }
    }

    return IS_VALID_STAT(ctx);
}

bool HttpTool::readBodyData(HttpCtx* ctx, Token* src) {
    int len = 0;
    int rdlen = 0;

    rdlen = HttpUtil::totalRd(&ctx->m_body);
    
    if (rdlen < ctx->m_rd_len && 
        !TokenUtil::isEmpty(src)) {
        len = ctx->m_rd_len - rdlen;
        if (len > src->m_len) {
            len = src->m_len;
        }

        HttpUtil::addStr(&ctx->m_body, src->m_str, len); 
        TokenUtil::skip(src, len);
    }

    return !(HttpUtil::totalRd(&ctx->m_body) < ctx->m_rd_len);
}

bool HttpTool::readBody(HttpCtx* ctx, Token* t) {
    bool completed = false; 

    CHK_TOKEN(t);
    
    completed = readBodyData(ctx, t);
    if (completed) {
        if (ctx->m_frame_beg + ctx->m_rd_len
            < ctx->m_content_len) {
            SET_STAT(ctx, ENUM_RD_FRAME_COMPLETED);
        } else {
            SET_STAT(ctx, ENUM_RD_COMPLETED);
        }
    } 

    return true;
}

bool HttpTool::readChunk(HttpCtx* ctx, Token* t) {
    bool completed = false;

    CHK_TOKEN(t);
    
    completed = readBodyData(ctx, t);
    if (completed) { 
        if (ctx->m_frame_beg + ctx->m_rd_len
            < ctx->m_content_len) {
            ctx->m_rd_len = 0;
            
            SET_STAT(ctx, ENUM_RD_PREPARE_CHUNK);
        } else { 
            ctx->m_rd_len = 0;
            SET_STAT(ctx, ENUM_RD_CHUNK_CRLF);
        }
    }

    return true;
}

bool HttpTool::readChunkCRLF(HttpCtx* ctx, Token* t) {
    CHK_TOKEN(t);
    
    return readCRLF(ctx, t, false);
}

bool HttpTool::readChunkTail(HttpCtx* ctx, Token* t) {
    CHK_TOKEN(t);
    
    return readCRLF(ctx, t, true);
}

bool HttpTool::readCRLF(HttpCtx* ctx, Token* src, bool end) {
    while (!TokenUtil::isEmpty(src)) {
        if (0 == ctx->m_rd_len) {
            if (TokenUtil::isCR(src->m_str[0])) {
                TokenUtil::skip(src, 1);
                ++ctx->m_rd_len;
            } else {
                SET_ERR(ctx, ENUM_HTTP_ERR_CHUNK_BODY);    
                break;
            }
        } else if (1 == ctx->m_rd_len) {
            if (TokenUtil::isLF(src->m_str[0])) {
                
                TokenUtil::skip(src, 1); 
                
                ctx->m_rd_len = 0; 
                
                if (!end) {
                    SET_STAT(ctx, ENUM_RD_CHUNK_HD);
                } else {
                    SET_STAT(ctx, ENUM_RD_COMPLETED);
                }

                break;
            } else {
                SET_ERR(ctx, ENUM_HTTP_ERR_CHUNK_BODY);    
                break;
            }
        } else {
            SET_ERR(ctx, -ENUM_HTTP_ERR_CHUNK_BODY);    
            break;
        }
    }

    return IS_VALID_STAT(ctx);
}

bool HttpTool::readChunkHd(HttpCtx* ctx, Token* src) {
    Token t;
    int last = 0;
    int len = 0;
    int clen = 0;
    int pos = 0;
    bool bOk = false;

    CHK_TOKEN(src);

    if (MAX_CHUNK_HD_LEN > ctx->m_rd_len) {
        len = MAX_CHUNK_HD_LEN - ctx->m_rd_len;
        if (len > src->m_len) {
            len = src->m_len;
        }

        MiscTool::bcopy(ctx->m_tmp + ctx->m_rd_len, src->m_str, len);

        ctx->m_rd_len += len;
        TokenUtil::skip(src, len);
    }
    
    t.set(ctx->m_tmp, ctx->m_rd_len);

    pos = TokenUtil::find(&t, DEF_LF_CHAR);
    if (0 < pos) { 
        last = t.m_str[ pos - 1 ];
        if (TokenUtil::isCR(last)) {
            
            /* go back of len */
            len = ctx->m_rd_len - (pos + 1);
            TokenUtil::skip(src, -len);
            
            /* ignore '\r\n' */
            t.set(ctx->m_tmp, pos -1);

            bOk = parseChunkLen(ctx, &t, &clen);
            if (bOk) { 
                if (0 < clen) {
                    ctx->m_content_len += clen;
                    ctx->m_rd_len = 0;
                    SET_STAT(ctx, ENUM_RD_PREPARE_CHUNK);
                } else {
                    /* the end zero chunk len */
                    ctx->m_rd_len = 0;
                    SET_STAT(ctx, ENUM_RD_CHUNK_TAIL);
                } 
            } else {
                SET_ERR(ctx, ENUM_HTTP_ERR_CHUNK_LEN);
            }
        } else {
            SET_ERR(ctx, ENUM_HTTP_ERR_CHUNK_LEN);
        }
    } else if (0 == pos) {
        /* invalid '\n' without '\r' */
        SET_ERR(ctx, ENUM_HTTP_ERR_CHUNK_LEN);
    } else if (MAX_CHUNK_HD_LEN > ctx->m_rd_len) {
        /* too short and not found, continue */
    } else {
        /* invalid chunk head line */
        SET_ERR(ctx, ENUM_HTTP_ERR_CHUNK_LEN);
    }

    return IS_VALID_STAT(ctx);
}

bool HttpTool::prepareChunk(HttpCtx* ctx) {
    int max = 0;
    bool bOk = false;
    int rdlen = 0;

    rdlen = HttpUtil::totalRd(&ctx->m_body);

    if (MAX_BODY_SIZE <= rdlen) {
        SET_STAT(ctx, ENUM_RD_CHUNK_COMPLETED);
    } else {
        max = ctx->m_content_len - ctx->m_frame_beg;
        if (MAX_BODY_SIZE < max) {
            max = MAX_BODY_SIZE;
        }

        max -= rdlen;
        bOk = HttpUtil::prepareBuf(&ctx->m_body, max); 
        if (bOk) {
            ctx->m_rd_len = rdlen + max;
            SET_STAT(ctx, ENUM_RD_CHUNK_BODY);
        } else {
            SET_ERR(ctx, ENUM_HTTP_ERR_ALLOC_BODY);
        }
    }
    
    return IS_VALID_STAT(ctx);
}

bool HttpTool::isChunk(const Token* key, const Token* val) {
    return TokenUtil::strCmp(key, "Transfer-Encoding", true)
        && TokenUtil::strCmp(val, "chunked", true);
}

bool HttpTool::isEncoded(const Token* key, 
    const Token* val, int* ptype) {
    int type = 0;
    
    if (TokenUtil::strCmp(key, "Content-Encoding", true)) {
        if (TokenUtil::strCmp(val, "gzip", true)) {
            type = 5001;
        } else if (TokenUtil::strCmp(val, "deflate", true)) {
            type = 5002;
        } else {
            type = 0;
        }
    }

    *ptype = type;
    return 0 != type;
}

bool HttpTool::isContentLen(const Token* key) {
    bool bOk = false;

    bOk = TokenUtil::strCmp(key, "Content-Length", true); 
    return bOk;
}

bool HttpTool::splitCmdLine(Token& t1,
    Token& t2, Token& token) {
    bool bOk = false; 
    
    bOk = TokenUtil::next(&t1, &token, " ");
    if (bOk) { 
        bOk = TokenUtil::next(&t2, &token, " "); 
        if (bOk) { 
            TokenUtil::strip(&token);
            if (!TokenUtil::isEmpty(&token)) {
                bOk = true;
            } else {
                bOk = false;
            }
        }
    }
    
    return bOk;
}

bool HttpTool::splitHeadLine(Token& key,
    Token& val, Token& token) {
    return TokenUtil::split(&key, &val, &token, ':');
}

bool HttpTool::dispatchFrame(int fd, HttpCtx* ctx, bool bEnd) {
    NodeMsg* msg = NULL;
    int ret = 0;

    msg = m_center->creatFrameHd(ctx, bEnd); 
    if (NULL != msg) {
        ret = m_center->dispatch(fd, msg);
        if (0 != ret) {
            SET_ERR(ctx, ENUM_HTTP_ERR_DISPATCH);
        }
    } else {
        SET_ERR(ctx, ENUM_HTTP_ERR_ALLOC_BODY);
    }

    return IS_VALID_STAT(ctx);
}

bool HttpTool::completeFrame(int fd, HttpCtx* ctx) {
    bool bOk = false;
    int rdlen = 0;

    bOk = dispatchFrame(fd, ctx, false);  
    if (bOk) {
        rdlen = HttpUtil::totalRd(&ctx->m_body);
        ctx->m_frame_beg += rdlen;
        ctx->m_rd_len = 0; 
        
        HttpUtil::release(&ctx->m_body); 
        SET_STAT(ctx, ENUM_RD_PREPARE_BODY);
    }
    
    return bOk;
}

bool HttpTool::completeChunk(int fd, HttpCtx* ctx) {
    bool bOk = false;
    int rdlen = 0;

    bOk = dispatchFrame(fd, ctx, false);  
    if (bOk) {
        rdlen = HttpUtil::totalRd(&ctx->m_body);
        ctx->m_frame_beg += rdlen;
        ctx->m_rd_len = 0; 

        HttpUtil::release(&ctx->m_body);
        SET_STAT(ctx, ENUM_RD_PREPARE_CHUNK);
    }

    return bOk;
}

bool HttpTool::completeMsg(int fd, HttpCtx* ctx) {
    bool bOk = false;
    unsigned seq = 0;

    bOk = dispatchFrame(fd, ctx, true);
    if (bOk) {
        seq = ctx->m_seq;
        m_center->resetCtx(ctx);
        ctx->m_seq = seq;

        SET_STAT(ctx, ENUM_RD_HTTP_HD);
    }

    return bOk;
}

