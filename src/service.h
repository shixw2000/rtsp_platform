#ifndef __SERVICE_H__
#define __SERVICE_H__
#include"shareheader.h"


struct GlobalConf;
class SockFrame;
class HttpSvr;
class HttpCli;
class HttpCenter;
class IHttpDealer;

class Service {
public:
    Service();
    ~Service();
    
    int init(); 
    void finish(); 

    int parseConf(const char* conf_file = NULL);
    int prepare();

    void addSvr(const char ip[], int port);
    void addCli(const char ip[], int port);

    void start();
    void stop();
    void wait(); 

private:
    void resetConf(GlobalConf* conf);

private:
    GlobalConf* m_config;
    SockFrame* m_frame;
    HttpSvr* m_http_svr;
    HttpCli* m_http_cli;
    IHttpDealer* m_dealer;
    IHttpDealer* m_demo;
    HttpCenter* m_center;
};

#endif

