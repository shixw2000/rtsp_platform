#include<cstdlib>
#include<cstring>
#include"service.h"
#include"baseconfig.h"
#include"sockframe.h"
#include"misc.h"
#include"httpcenter.h"
#include"isockapi.h"
#include"rtspcenter.h"
#include"httputil.h"
#include"msgcenter.h"
#include"demo.h"


class HttpSvr : public ISockSvr { 
public:
    HttpSvr(HttpCenter* center,
        const GlobalConf* conf); 
    
    virtual int onNewSock(int parentId, 
        int newId, AccptOption& opt);

    virtual void onListenerClose(int hd); 
    virtual void onClose(int hd);

    virtual int process(int hd, NodeMsg* msg);

    virtual int parseData(int fd, const char* buf, 
        int size, const SockAddr* addr);

private:
    HttpCenter* m_center;
    const GlobalConf* m_conf;
    unsigned m_pkg_cnt;
};

class HttpCli : public ISockCli { 
public: 
    HttpCli(SockFrame* frame, HttpCenter* center,
        const GlobalConf* conf);
    
    virtual ~HttpCli();

    virtual int onConnOK(int hd, ConnOption& opt);

    virtual void onConnFail(long, int);
    
    virtual void onClose(int hd);

    virtual int process(int hd, NodeMsg* msg);

    virtual int parseData(int fd, const char* buf, 
        int size, const SockAddr* addr);

    void onTimerPerSec();

    void sendHttpReq(int hd, int cnt);

    NodeMsg* genHttpReq();

private:
    SockFrame* m_frame;
    TimerObj* m_timer_1sec;
    HttpCenter* m_center;
    unsigned m_seconds;
    unsigned m_pkg_cnt;
    const GlobalConf* m_conf;
};

HttpSvr::HttpSvr(HttpCenter* center,
    const GlobalConf* conf)
    : m_center(center), m_conf(conf) {
    m_pkg_cnt = 0;
} 

int HttpSvr::onNewSock(int, int, AccptOption& opt) {
    HttpCtx* ctx = NULL;
    int ret = 0;

    ctx = m_center->creatSock();
    if (NULL != ctx) {
        opt.m_extra = (long)ctx;
        opt.m_rd_thresh = m_conf->m_rd_thresh;
        opt.m_wr_thresh = m_conf->m_wr_thresh;
    } else {
        ret = -1;
    }

    return ret;
}

void HttpSvr::onListenerClose(int hd) {
    m_center->onListenerClose(hd);
}

void HttpSvr::onClose(int hd) {
    m_center->onClose(hd);
}

int HttpSvr::process(int hd, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->process(hd, msg);

    ++m_pkg_cnt;

    if (!(m_pkg_cnt & m_conf->m_prnt_mask)) {
        LOG_INFO("proc_msg| pkg_cnt=%u|", m_pkg_cnt);
    }
    
    return ret;
}

int HttpSvr::parseData(int fd, const char* buf, 
    int size, const SockAddr*) {
    int ret = 0;

    ret = m_center->parseData(fd, buf, size);
    return ret;
}

static bool _timerOneSec(long p1, long) {
    HttpCli* cli = (HttpCli*)p1;

    cli->onTimerPerSec();
    return true;
}

HttpCli::HttpCli(SockFrame* frame, 
    HttpCenter* center,
    const GlobalConf* conf) 
    : m_frame(frame), m_center(center),
    m_conf(conf) {
    m_seconds = 0;
    m_pkg_cnt = 0;

    m_timer_1sec = m_frame->allocTimer();
    m_frame->setParam(m_timer_1sec, &_timerOneSec,
        (long)this);
    
    m_frame->startTimer(m_timer_1sec, 0, 1);
}

HttpCli::~HttpCli() {
    if (NULL != m_timer_1sec) {
        m_frame->freeTimer(m_timer_1sec);
        m_timer_1sec = NULL;
    }
}

void HttpCli::onClose(int hd) { 
    m_center->onClose(hd);
}

void HttpCli::onConnFail(long extra, int err) {
    m_center->onConnFail(extra, err);
}

int HttpCli::onConnOK(int hd, ConnOption& opt) {
    int ret = 0;
    
    ret = m_center->onConnOK(hd);
    if (0 == ret) {
        opt.m_rd_thresh = m_conf->m_rd_thresh;
        opt.m_wr_thresh = m_conf->m_wr_thresh;
        
        sendHttpReq(hd, m_conf->m_first_send_cnt);
    }
    
    return ret;
}

void HttpCli::sendHttpReq(int hd, int cnt) {
    NodeMsg* msg = NULL;
    NodeMsg* ref = NULL;
    
    msg = genHttpReq();
    for (int i=0; i<cnt; ++i) {
        ref = MsgCenter::refMsg(msg);
        m_frame->sendMsg(hd, ref);
    }

    MsgCenter::freeMsg(msg);
}

int HttpCli::process(int hd, NodeMsg*) {
    int ret = 0;

    ++m_pkg_cnt;

    sendHttpReq(hd, 1);
    
    if (!(m_pkg_cnt & m_conf->m_prnt_mask)) {
        LOG_INFO("proc_msg| pkg_cnt=%u| now=%u|", 
            m_pkg_cnt, m_seconds);
    }
    
    return ret;
}

int HttpCli::parseData(int fd, const char* buf,
    int size, const SockAddr*) {
    int ret = 0;

    ret = m_center->parseData(fd, buf, size);
    return ret;
}

void HttpCli::onTimerPerSec() {
    ++m_seconds;
} 

NodeMsg* HttpCli::genHttpReq() {
    NodeMsg* msg = NULL;
    HttpCache* pa[1] = {NULL};
    HttpCache http;

    HttpUtil::init(&http);
    
    HttpUtil::addReqParam(&http,
        m_conf->m_method.c_str(), 
        m_conf->m_url.c_str(), 
        HTTP_VER_11_VAL);

    pa[0] = &http;
    msg = HttpUtil::genHttpHd(pa, 1, 
        m_conf->m_body_text.data(),
        m_conf->m_body_text.size());

    HttpUtil::release(&http);
    return msg;
} 

Service::Service() {
    m_frame = NULL;
    m_http_svr = NULL;
    m_http_cli = NULL;
    m_center = NULL;
    m_dealer = NULL;
    m_demo = NULL;

    m_config = new GlobalConf;
    
    resetConf(m_config);
}

Service::~Service() {
    delete m_config;
}

void Service::resetConf(GlobalConf* conf) {
    conf->m_rd_thresh = 0;
    conf->m_wr_thresh = 0;
    conf->m_rd_timeout = 60;
    conf->m_wr_timeout = 60;
    conf->m_cli_cnt = 0;
    conf->m_prnt_mask = (1 << 10);
}

int Service::init() {
    int ret = 0; 

    do { 
        ret = ConfLog(m_config->m_log_dir.c_str());
        if (0 != ret) {
            break;
        }

        SockFrame::creat(); 

        m_frame = SockFrame::instance(); 

        m_center = new HttpCenter(m_frame); 
        ret = m_center->init();
        if (0 != ret) {
            break;
        }
        
        m_http_svr = new HttpSvr(m_center, m_config);
        m_http_cli = new HttpCli(m_frame, m_center, m_config);

        m_dealer = new RtspHandler(m_frame, m_center);
        m_center->addDealer(m_dealer); 
        
        m_demo = new TestDemo(m_frame, m_center);
        m_center->addDealer(m_demo); 
    } while (0);

    return ret;
}

int Service::parseConf(const char* conf_file) {
    int ret = 0;
    BaseConfig conf; 

    ret = conf.parseFile(conf_file, *m_config);
    return ret;
}

void Service::finish() {
    if (NULL != m_http_svr) {
        delete m_http_svr;
        m_http_svr = NULL;
    } 

    if (NULL != m_http_cli) {
        delete m_http_cli;
        m_http_cli = NULL;
    }

    if (NULL != m_center) {
        m_center->finish();
        
        delete m_center;
        m_center = NULL;
    }

    if (NULL != m_dealer) {
        delete m_dealer;
        m_dealer = NULL;
    }

    if (NULL != m_demo) {
        delete m_demo;
        m_demo = NULL;
    }
    
    if (NULL != m_frame) {
        SockFrame::destroy(m_frame);
        m_frame = NULL;
    }
}

void Service::addSvr(const char ip[], int port) { 
    m_frame->creatSvr(ip, port, m_http_svr, 0);
}

void Service::addCli(const char ip[], int port) {
    HttpCtx* ctx = NULL;

    ctx = m_center->creatSock();
    
    m_frame->creatCli(ip, port, m_http_cli, (long)ctx);
}

int Service::prepare() {
    int ret = 0;

    if (0 == m_config->m_role) { 
        ret = readFile(m_config->m_body.c_str(),
            m_config->m_body_text);
        if (0 == ret) {
            for (int i=0; i<m_config->m_cli_cnt; ++i) {
                addCli(m_config->m_addr.m_ip.c_str(), 
                    m_config->m_addr.m_port);
            }
        }
    } else {
        addSvr(m_config->m_addr.m_ip.c_str(), m_config->m_addr.m_port);
    }

    return ret;
}

void Service::start() { 
    m_frame->setTimeout(m_config->m_rd_timeout, m_config->m_wr_timeout);

    m_frame->start(); 
}

void Service::wait() {
    m_frame->wait();
}

void Service::stop() {
    m_frame->stop();
}

