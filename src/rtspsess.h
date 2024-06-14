#ifndef __RTSPSESS_H__
#define __RTSPSESS_H__
#include"rtpdata.h"


struct Token;

class RtspSess {
    typedef typeMapSess::iterator itrSess;
    typedef typeMapSess::const_iterator constItrSess;
    
public:
    RtspCtx* findSess(const char sid[]);
    bool existSess(const char sid[]);
    bool addSess(RtspCtx* ctx);
    void delSess(const char sid[]);

    void genSess(char sid[], int max);

private:
    typeMapSess m_sess;
};

#endif

