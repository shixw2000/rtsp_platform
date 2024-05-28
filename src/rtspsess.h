#ifndef __RTSPSESS_H__
#define __RTSPSESS_H__
#include<map>
#include"rtspdata.h"


struct Token;
struct HttpCache;
struct SessionCtx;

typedef std::map<typeStr, SessionCtx*> typeSess;
typedef typeSess::iterator itrSess;
typedef typeSess::const_iterator constItrSess;

class RtspSess {
public:
    SessionCtx* creatSess(const Token& sid, const Token& path);
    
    FFStream* creatStream();
    void freeSess(SessionCtx* sess);
    void freeStream(FFStream* s);
    
    SessionCtx* findSess(const Token& sid);

    bool isSessPath(SessionCtx* sess, const Token& path);
    FFStream* findStream(SessionCtx* sess, const Token& path);
    
    SessionCtx* prepareSess(const Token& sid, const Token& path);
    bool existSess(const Token& sid);
    bool addSess(SessionCtx* ctx);
    void delSess(const Token& sid); 

private:
    typeSess m_sess;
};

#endif

