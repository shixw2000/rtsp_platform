#ifndef __IHTTPDEALER_H__
#define __IHTTPDEALER_H__


struct NodeMsg;

class IHttpDealer {
public:
    virtual ~IHttpDealer() {}

    virtual bool procFrameHd(int hd, NodeMsg* msg) = 0;
};

#endif

