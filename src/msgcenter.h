#ifndef __MSGCENTER_H__
#define __MSGCENTER_H__
#include"shareheader.h"


class MsgCenter {
public:
    template<typename T>
    static T* getBody(NodeMsg* msg) {
        char* psz = getMsg(msg);
        
        return reinterpret_cast<T*>(psz);
    }
    
    static NodeMsg* allocFrameHd(int line);
    static NodeMsg* allocHttpMsg(int total);
    static NodeMsg* allocMsg(int type, int total);
    
    static void freeMsg(NodeMsg* pb);
    static NodeMsg* refMsg(NodeMsg* pb); 

    static int getTotal(NodeMsg* pb); 
    
    static char* getMsg(NodeMsg* msg, bool ext = false);
    static char* getCurr(NodeMsg* msg, bool ext = false);
    static int getLeft(NodeMsg* msg, bool ext = false);
    
    static int getMsgSize(NodeMsg* msg, bool ext = false);
    static int getMsgPos(NodeMsg* pb, bool ext = false);

    static void setMsgSize(NodeMsg* pb, int size, bool ext = false);
    static void setMsgPos(NodeMsg* pb, int pos, bool ext = false);
    static void skipMsgPos(NodeMsg* pb, int pos, bool ext = false); 
    
    static Cache* getCache(NodeMsg* pb, bool ext = false);
    static void setCache(NodeMsg* pb, Cache* cache,
        int size, bool ext = false);
    
    static int getType(NodeMsg* msg); 

    static NodeMsg* allocUdpMsg(int size);
    static NodeMsg* refUdpMsg(NodeMsg* msg);

    static int setUdpAddr(NodeMsg* msg, const SockAddr& addr);
    static const SockAddr* getUdpAddr(NodeMsg* msg);

private:
    static void freeFrameHdCb(char* psz); 
    static void freeFrameMidCb(char* psz); 
};

#endif

