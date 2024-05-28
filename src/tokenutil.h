#ifndef __TOKENUTIL_H__
#define __TOKENUTIL_H__


struct Token;
struct Range;

class TokenUtil {
public: 
    static void token2Range(Range* r, 
        const char* start, const Token* t);

    static void range2Token(Token* t, 
        const char* start, const Range* r);

    static bool isEmpty(const Token* t);
    
    static void strip(Token* t);
    
    static bool startWith(const Token* t1, 
        const Token& substr, bool icase);
    
    static bool endWith(const Token* t1, 
        const Token& substr, bool icase);
    
    static bool strCmp(const Token* t1, 
        const Token& str, bool icase = true); 

    static void addToken(Token* dst, const Token& src);
    static void addChar(Token* dst, char c);
    
    static bool isSpace(int ch);
    static bool isCR(int ch);
    static bool isLF(int ch);
    static bool isDigit(int ch);
    static bool isHex(int ch);
    static int hex2Num(int ch);
    static bool toHexNum(int* pn, const Token* token);
    static bool toNum(int* pn, const Token* token);
    static int toStr(char* out, int max, int val);
    static int toHexStr(char* out, void* in, int inLen);
    
    static void skipOf(Token* t,
        const char* delim);
    
    static void skipLastOf(Token* t, 
        const char* delim);
    
    static void skip(Token* t, int n);

    static bool substr(Token* dst, 
        const Token* src, int beg, int len = -1);

    static void copy(char buf[], int max, const Token* t);
    
    static bool next(Token* ret, Token* token,
        const char* delim);

    static bool nextInt(int* ret, 
        Token* token, const char* delim);

    static int find(const Token* token, char c);
    static int rfind(const Token* token, char c); 

    static int findFirstOf(const Token* token, const char* delim);
    static int findLastOf(const Token* token, const char* delim);
    
    static bool split(Token* key, Token* val, 
        const Token* token, char c); 
};


#endif

