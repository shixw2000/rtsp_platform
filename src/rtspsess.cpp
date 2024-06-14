#include<cstring>
#include<cstdio>
#include"httpdata.h"
#include"rtspdata.h"
#include"shareheader.h"
#include"httputil.h"
#include"tokenutil.h"
#include"cache.h"
#include"misc.h"
#include"rtspsess.h"
#include"rtspres.h"


RtspCtx* RtspSess::findSess(const char sid[]) {
    constItrSess itr;
        
    itr = m_sess.find(sid);
    if (m_sess.end() != itr) {
        return itr->second;
    } else { 
        return NULL;
    }
}

bool RtspSess::existSess(const char sid[]) {
    constItrSess itr;
        
    itr = m_sess.find(sid);
    if (m_sess.end() != itr) {
        return true;
    } else { 
        return false;
    }
}

bool RtspSess::addSess(RtspCtx* ctx) {
    const char* psz = NULL;
    bool found = false;
    typeStr strKey;

    if (NULL != ctx && !TokenUtil::isNULL(ctx->m_sid[0])) { 
        psz = ctx->m_sid;
        
        found = existSess(psz);
        if (!found) {
            m_sess[psz] = ctx;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void RtspSess::delSess(const char sid[]) {
    itrSess itr; 

    itr = m_sess.find(sid);
    if (m_sess.end() != itr) {
        m_sess.erase(itr);
    } 
}

void RtspSess::genSess(char sid[], int max) {
    bool found = false;
    char buf[DEF_SESS_NUM_LEN] = {0};
        
    if (DEF_SESS_NUM_LEN * 2 + 1 <= max) {
        do {
            MiscTool::getRand(buf, DEF_SESS_NUM_LEN);
            TokenUtil::toHexStr(sid, buf, DEF_SESS_NUM_LEN);
            found = existSess(sid);
        } while (found);
    } else {
        MiscTool::bzero(sid, max);
    }
}


