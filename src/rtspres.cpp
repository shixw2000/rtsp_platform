#include<cstring>
#include<cstdio>
#include"rtspres.h"
#include"httpdata.h"
#include"rtspdata.h"
#include"shareheader.h"
#include"httputil.h"
#include"tokenutil.h"
#include"cache.h"
#include"rtspsdp.h"


RtspRes::RtspRes() {
    m_util = new SdpUtil;
}

RtspRes::~RtspRes() {
    if (NULL != m_util) {
        delete m_util;
    }
}

bool RtspRes::addRes(SdpData* sdp) {
    bool found = false;
    typeStr strKey;

    if (NULL != sdp &&
        DEF_NULL_CHAR != sdp->m_header.m_path[0]) { 
        found = existRes(sdp->m_header.m_path);
        if (!found) {
            strKey.assign(sdp->m_header.m_path);
            m_datas[strKey] = sdp;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void RtspRes::delRes(const Token& path) {
    itrSdp itr;
    typeStr strKey;
    
    if (!TokenUtil::isEmpty(&path)) { 
        strKey.assign(path.m_str, path.m_len);
        
        itr = m_datas.find(strKey);
        if (m_datas.end() != itr) {
            m_datas.erase(itr);
        }
    }
}

bool RtspRes::existRes(const Token& path) {
    constItrSdp itr;
    typeStr strKey;
    
    if (!TokenUtil::isEmpty(&path)) { 
        strKey.assign(path.m_str, path.m_len);
        
        itr = m_datas.find(strKey);
        if (m_datas.end() != itr) {
            return true;
        } else { 
            return false;
        }
    } else {
        return false;
    }
}

SdpData* RtspRes::findRes(const Token& path) {
    constItrSdp itr;
    typeStr strKey;

    if (!TokenUtil::isEmpty(&path)) { 
        strKey.assign(path.m_str, path.m_len);
        
        itr = m_datas.find(strKey);
        if (m_datas.end() != itr) {
            return itr->second;
        } else { 
            return NULL;
        }
    } else {
        return NULL;
    }
}

SdpMedia* RtspRes::findMedia(const Token& path) {
    SdpData* sdp = NULL;
    constItrSdp itr;
    int index = 0;

    if (!TokenUtil::isEmpty(&path)) { 
        for (itr=m_datas.begin(); itr!=m_datas.end(); ++itr) {
            sdp = itr->second;
            
            index = m_util->findMedia(sdp, path);
            if (0 <= index) {
                return sdp->m_media[index];
            }
        }
    }
    
    return NULL;
}

int RtspRes::prepareSDP(HttpCache* cache, const Token& path) {
    SdpData* sdp = NULL;

    sdp = findRes(path);
    if (NULL != sdp) {
        m_util->prepareSDP(cache, sdp);
        return 0;
    } else {
        return -1;
    } 
}

int RtspRes::parseSDP(const Token& path, Token& text) {
    SdpData* sdp = NULL;
    bool found = false;
    int ret = 0;

    found = existRes(path);
    if (!found) {
        sdp = m_util->creatSDP(path);
        if (NULL != sdp) {
            ret = m_util->parseSDP(sdp, &text);
            if (0 == ret) {
                addRes(sdp);
            } else {
                m_util->freeSDP(sdp);
            }
        } else {
            ret = -1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

