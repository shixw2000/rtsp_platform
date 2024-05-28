#ifndef __MSGCENTER_H__
#define __MSGCENTER_H__


struct NodeMsg;
struct Cache;

class MsgCenter {
public:
    template<typename T>
    static T* getBody(NodeMsg* msg) {
        char* psz = getMsg(msg);
        
        return reinterpret_cast<T*>(psz);
    }
    
    static NodeMsg* allocFrameHd(int line);
    static NodeMsg* allocFrameMid();
    static NodeMsg* allocHttpMsg(int total);
    static NodeMsg* allocMsg(int type, int total);
    
    static void freeMsg(NodeMsg* pb);
    static NodeMsg* refMsg(NodeMsg* pb);

    static char* getMsg(NodeMsg* msg);
    static int getMsgSize(NodeMsg* msg);
    
    static int getType(NodeMsg* msg);

    static void setExtraCache(NodeMsg* pb, 
        Cache* cache, int size);

    static Cache* getExtraCache(NodeMsg* pb);
    static int getExtraSize(NodeMsg* pb); 
    static int getExtraPos(NodeMsg* pb); 
    static char* getExtraData(NodeMsg* pb);

    static void setExtraPos(NodeMsg* pb, int pos);

private:
    static void freeFrameHdCb(char* psz); 
    static void freeFrameMidCb(char* psz); 
};

#endif

