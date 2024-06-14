#include<cstring>
#include"misc.h"
#include"httpdata.h"
#include"tokenutil.h"


Token::Token(const char* str, int size) {
    set(str, size);
}

Token::Token() {
    m_str = NULL;
    m_len = 0;
}

void Token::set(const char* str, int size) {
    m_str = (char*)str;

    if (0 <= size) {
        m_len = size;
    } else {
        m_len = (int)strlen(str);
    }
} 

bool TokenUtil::isEmpty(const Token* t) {
    return (!(0 < t->m_len));
}

void TokenUtil::strip(Token* t) { 
    /* skip head */
    while (0 < t->m_len && isSpace(t->m_str[0])) {
        --t->m_len;
        ++t->m_str;
    }

    /* skip tail */
    while (0 < t->m_len && isSpace(t->m_str[ t->m_len-1 ])) {
        --t->m_len;
    }
}

void TokenUtil::skip(Token* t, int n) {
    t->m_str += n;
    t->m_len -= n;
}

void TokenUtil::skipOf(Token* t, const char* delim) {
    while (0 < t->m_len && NULL != strchr(delim, t->m_str[0])) {
        ++t->m_str;
        --t->m_len;
    }
}

void TokenUtil::skipLastOf(Token* t, const char* delim) { 
    while (0 < t->m_len && NULL != strchr(delim, 
        t->m_str[ t->m_len - 1 ])) {
        --t->m_len;
    }
}

bool TokenUtil::substr(Token* dst, const Token* src, int beg, int len) {
    if (0 <= beg && beg < src->m_len) {
        if (0 > len || beg + len > src->m_len) {
            len = src->m_len - beg;
        }

        dst->set(&src->m_str[beg], len);
        strip(dst);
        
        return !isEmpty(dst);
    } else {
        dst->set(NULL, 0);
        return false;
    }
}

void TokenUtil::copy(char buf[], int max, const Token* t) {
    if (0 < max) {
        --max;
        if (max > t->m_len) {
            max = t->m_len;
        }

        strncpy(buf, t->m_str, max);
        buf[max] = DEF_NULL_CHAR;
    }
}

bool TokenUtil::next(Token* ret, Token* token, const char* delim) {
    int len = 0;
    
    if (0 < token->m_len) { 
        /* skip delim chars */
        skipOf(token, delim);

        if (0 < token->m_len) {
            while (len < token->m_len && 
                NULL == strchr(delim, token->m_str[len])) {
                ++len;
            }

            ret->set(token->m_str, len);
            skip(token, len);
            
            strip(ret);
            return !isEmpty(ret);
        } 
    } 

    ret->set(NULL, 0);
    return false;
}

bool TokenUtil::nextInt(int* ret, 
    Token* token, const char* delim) {
    bool bOk = false;
    Token val;
    int n = 0;
    
    bOk = next(&val, token, delim);
    if (bOk) {
        bOk = toNum(&n, &val);
    }

    if (NULL != ret) {
        *ret = n;
    }

    return bOk;
}

bool TokenUtil::startWith(const Token* t1, 
    const Token& substr, bool icase) {
    if (t1->m_len >= substr.m_len) {
        if (!icase) {
            return 0 == strncmp(t1->m_str,
                substr.m_str, substr.m_len);
        } else {
            return 0 == strncasecmp(t1->m_str,
                substr.m_str, substr.m_len);
        }
    } else {
        return false;
    }
}

bool TokenUtil::endWith(const Token* t1, 
    const Token& substr, bool icase) {
    const char* psz = NULL;
    
    if (t1->m_len >= substr.m_len) {
        psz = t1->m_str + (t1->m_len - substr.m_len);
        
        if (!icase) {
            return 0 == strncmp(psz, substr.m_str, substr.m_len);
        } else {
            return 0 == strncasecmp(psz, substr.m_str, substr.m_len);
        }
    } else {
        return false;
    }
}

bool TokenUtil::strCmp(const Token* t1, const Token& str, bool icase) {
    if (t1->m_len == str.m_len) {
        if (!icase) {
            return 0 == strncmp(t1->m_str, str.m_str, t1->m_len);
        } else {
            return 0 == strncasecmp(t1->m_str, str.m_str, t1->m_len);
        }
    } else {
        return false;
    }
}

void TokenUtil::addToken(Token* dst, const Token& src) {
    if (0 < src.m_len) {
        MiscTool::bcopy(dst->m_str + dst->m_len,
            src.m_str, src.m_len);
        dst->m_len += src.m_len;
    }
}

void TokenUtil::addChar(Token* dst, char c) {
    dst->m_str[dst->m_len++] = c;
}

bool TokenUtil::isSpace(int ch) {
    return NULL != strchr(DEF_SPACE_DELIM, ch);
}

bool TokenUtil::isCR(int ch) {
    return DEF_CR_CHAR == ch;
}

bool TokenUtil::isLF(int ch) {
    return DEF_LF_CHAR == ch;
}

bool TokenUtil::isSeperator(int ch) {
    return DEF_URL_SEP_CHAR == ch;
}

bool TokenUtil::isNULL(int ch) {
    return DEF_NULL_CHAR == ch;
}

bool TokenUtil::isDigit(int ch) {
    return '0' <= ch && ch <= '9';
}

bool TokenUtil::isHex(int ch) {
    return ('0' <= ch && ch <= '9') ||
        ('a' <= ch && ch <= 'f') ||
        ('A' <= ch && ch <= 'F');
}

int TokenUtil::hex2Num(int ch) {
    int n = (0xF & ch);

    if (0x40 & ch) {
        n += 9;
    }

    return n;
}

bool TokenUtil::toHexNum(int* pn, const Token* token) {
    int n = 0;
    int pos = 0;
    int ch = '\0';
    bool is_neg = false;

    if (0 < token->m_len) {
        ch = token->m_str[pos];
        if ('-' == ch) {
            is_neg = true;
            ++pos;
        } else if ('+' == ch) {
            ++pos;
        } else {
            /* not plus or minus */
        }

        while (pos < token->m_len) {
            ch = token->m_str[pos++];
            if (isHex(ch)) {
                ch = hex2Num(ch);

                n = (n << 4) + ch;
            } else {
                break;
            }
        }

        if (is_neg) {
            n = -n;
        }
    }
    
    *pn = n;
    return 0 < pos && pos == token->m_len;
}

bool TokenUtil::toNum(int* pn, const Token* token) {
    int n = 0;
    int pos = 0;
    int ch = '\0';
    bool is_neg = false;

    if (0 < token->m_len) {
        ch = token->m_str[pos];
        if ('-' == ch) {
            is_neg = true;
            ++pos;
        } else if ('+' == ch) {
            ++pos;
        } else {
            /* not plus or minus */
        }

        while (pos < token->m_len) {
            ch = token->m_str[pos++];
            if (isDigit(ch)) {
                ch -= '0';

                n = n * 10 + ch;
            } else {
                break;
            }
        }

        if (is_neg) {
            n = -n;
        }
    }
    
    *pn = n;
    return 0 < pos && pos == token->m_len;
}

int TokenUtil::toStr(char* out, int max, int val) {
    const int MAX_LEN = 32;
    char* psz = NULL;
    bool is_neg = false;
    int len = 0;
    int size = 0;
    int n = 0;
    char buf[MAX_LEN] = {0};

    if (1 < max) { 
        if (0 > val) { 
            val = -val;
            is_neg = true;
        }

        for (len=0; len<MAX_LEN && 10 <= val; ++len) {
            n = val / 10;
            val -= n * 10;

            buf[len] = (char)('0' + val);
            val = n;
        }

        if (len < MAX_LEN) {
            buf[len] = (char)('0' + val);
            ++len;
        } 

        size = max - 1;
        psz = out;
        if (is_neg && 0 < size) {
            *psz = '-';
            --size;
            ++psz;
        }
        
        while (0 < size && 0 < len) {
            *psz = buf[--len];
            ++psz;
            --size;
        }

        *psz = '\0';

        return (int)(psz - out);
    } else {
        return 0;
    }
}

int TokenUtil::toHexStr(char* out, void* in, int inLen) {
    static const char HEX_CHAR[] = "0123456789ABCDEF";
    unsigned char* pn = 0;
    int outLen = 0;
    int val = 0;
    
    for (pn = (unsigned char*)in;0 < inLen; ++pn, --inLen) {
        val = *pn;
        out[outLen++] = HEX_CHAR[ val >> 4 ];
        out[outLen++] = HEX_CHAR[ val & 0xF ];
    }

    out[outLen] = DEF_NULL_CHAR;
    return outLen;
}

int TokenUtil::find(const Token* token, char c) {
    for (int pos = 0; pos < token->m_len; ) {
        if (c != token->m_str[pos]) {
            ++pos;
        } else {
            return pos;
        }
    }

    return -1;
}

int TokenUtil::rfind(const Token* token, char c) {
    for (int pos = token->m_len - 1; pos >= 0; ) {
        if (c != token->m_str[pos]) {
            --pos;
        } else {
            return pos;
        }
    } 
    
    return -1;
}

int TokenUtil::findFirstOf(const Token* token, const char* delim) {
    for (int pos = 0; pos < token->m_len; ) {
        if (NULL == strchr(delim, token->m_str[pos])) {
            ++pos;
        } else {
            return pos;
        }
    }

    return -1;
}

int TokenUtil::findLastOf(const Token* token, const char* delim) {
    for (int pos = token->m_len-1; 0 <= pos; ) {
        if (NULL == strchr(delim, token->m_str[pos])) {
            --pos;
        } else {
            return pos;
        }
    }

    return -1;
}

bool TokenUtil::split(Token* key, Token* val, 
    const Token* token, char c) {
    int pos = 0;

    pos = TokenUtil::find(token, c); 
    if (0 < pos) {
        TokenUtil::substr(key, token, 0, pos);
        TokenUtil::substr(val, token, pos + 1);

        TokenUtil::strip(key);
        TokenUtil::strip(val); 
        
        return !isEmpty(key) && !isEmpty(val);
    } else {
        key->set(NULL, 0);
        val->set(NULL, 0);
        
        return false;
    }
}

void TokenUtil::token2Range(Range* r, 
    const char* start, const Token* t) { 
    r->m_beg = (int)(t->m_str - start);
    r->m_len = t->m_len;
}

void TokenUtil::range2Token(Token* t, 
    const char* start, const Range* r) {
    t->set(start + r->m_beg, r->m_len);
}

