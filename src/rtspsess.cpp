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


SessionCtx* RtspSess::creatSess(const Token& sid,
    const Token& path) {
    SessionCtx* ptr = NULL;

    ptr = (SessionCtx*)CacheUtil::mallocAlign(sizeof(SessionCtx));
    MiscTool::bzero(ptr, sizeof(SessionCtx));

    TokenUtil::copy(ptr->m_sid, sizeof(ptr->m_sid), &sid);
    TokenUtil::copy(ptr->m_path, sizeof(ptr->m_path), &path);
    return ptr;
}

FFStream* RtspSess::creatStream() {
    FFStream* s = NULL;

    s = (FFStream*)CacheUtil::mallocAlign(sizeof(FFStream));
    MiscTool::bzero(s, sizeof(FFStream));
    return s;
}

void RtspSess::freeSess(SessionCtx* sess) {
    FFStream* s = NULL;
    
    if (NULL != sess) {
        for (int i=0; i<sess->m_stream_cnt; ++i) {
            s = sess->m_stream[i];
            freeStream(s);

            sess->m_stream[i] = NULL;
        }

        CacheUtil::freeAlign(sess);
    }
}

void RtspSess::freeStream(FFStream* s) {
    if (NULL != s) {
        CacheUtil::freeAlign(s);
    }
}

SessionCtx* RtspSess::findSess(const Token& sid) {
    constItrSess itr;
    typeStr strKey;

    if (!TokenUtil::isEmpty(&sid)) { 
        strKey.assign(sid.m_str, sid.m_len);
        
        itr = m_sess.find(strKey);
        if (m_sess.end() != itr) {
            return itr->second;
        } else { 
            return NULL;
        }
    } else {
        return NULL;
    }
}

bool RtspSess::isSessPath(SessionCtx* sess, const Token& path) {
    return TokenUtil::strCmp(&path, sess->m_path, false);
}

FFStream* RtspSess::findStream(SessionCtx* sess, const Token& path) {
    const SdpMedia* media = NULL;
    FFStream* s = NULL;
    Token dst;
    Token needle;

    dst.set(sess->m_path);

    do { 
        if (dst.m_len > path.m_len) {
            break;
        }
        
        needle.set(path.m_str, dst.m_len); 
        if (!TokenUtil::strCmp(&dst, needle, false)) {
            break;
        }

        if (dst.m_len == path.m_len) {
            /* match resource path */
            return sess->m_stream[0];
        } else if (DEF_URL_SEP_CHAR != path.m_str[dst.m_len]) {
            break;
        } else {
            /* find sub path */
        }
        
        /* partial match */
        TokenUtil::substr(&needle, &path, dst.m_len + 1);
        if (TokenUtil::isEmpty(&needle)) {
            break;
        } 

        for (int i=0; i<sess->m_stream_cnt; ++i) {
            s = sess->m_stream[i];
            media = s->m_media;
            
            dst.set(media->m_path);
            if (TokenUtil::strCmp(&dst, needle, false)) {
                return s;
            }
        }
    } while (false);
    
    return NULL;
}

SessionCtx* RtspSess::prepareSess(const Token& sid,
    const Token& path) {
    SessionCtx* sess = NULL;
    constItrSess itr;
    typeStr strKey;

    if (!TokenUtil::isEmpty(&sid)) { 
        strKey.assign(sid.m_str, sid.m_len);
        
        itr = m_sess.find(strKey);
        if (m_sess.end() != itr) {
            return itr->second;
        } else { 
            sess = creatSess(sid, path);
            if (NULL != sess) {
                m_sess[strKey] = sess;
            }

            return sess;
        }
    } else {
        return NULL;
    }
}

bool RtspSess::existSess(const Token& sid) {
    constItrSess itr;
    typeStr strKey;

    if (!TokenUtil::isEmpty(&sid)) { 
        strKey.assign(sid.m_str, sid.m_len);
        
        itr = m_sess.find(strKey);
        if (m_sess.end() != itr) {
            return true;
        } else { 
            return false;
        }
    } else {
        return false;
    }
}

bool RtspSess::addSess(SessionCtx* ctx) {
    bool found = false;
    typeStr strKey;

    if (NULL != ctx &&
        DEF_NULL_CHAR != ctx->m_sid[0]) { 
        found = existSess(ctx->m_sid);
        if (!found) {
            strKey.assign(ctx->m_sid);
            m_sess[strKey] = ctx;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void RtspSess::delSess(const Token& sid) {
    itrSess itr;
    typeStr strKey;

    if (!TokenUtil::isEmpty(&sid)) { 
        strKey.assign(sid.m_str, sid.m_len);
        
        itr = m_sess.find(strKey);
        if (m_sess.end() != itr) {
            m_sess.erase(itr);
        }
    }
}

