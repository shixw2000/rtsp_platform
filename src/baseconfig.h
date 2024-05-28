#ifndef __BASECONFIG_H__
#define __BASECONFIG_H__
#include"config.h"


struct GlobalConf {
    unsigned m_rd_thresh; 
    unsigned m_wr_thresh;
    unsigned m_rd_timeout;
    unsigned m_wr_timeout;
    int m_first_send_cnt;
    int m_cli_cnt;
    unsigned m_prnt_mask;
    int m_log_level;
    int m_log_stdin;
    int m_log_size;
    int m_role; // 0: client, 1: server,
    typeStr m_log_dir;
    typeStr m_method;
    typeStr m_url;
    typeStr m_body;
    typeStr m_body_text;
    AddrInfo m_addr;
};

static const char SEC_LOG[] = "log";
static const char SEC_SERVER[] = "server";
static const char SEC_CLIENT[] = "client";
static const char KEY_LISTEN_ADDR[] = "listen_addr";
static const char KEY_CONN_ADDR[] = "conn_addr";
static const char KEY_ROLE[] = "role";

class BaseConfig { 
public: 
    int parseFile(const char* file, GlobalConf& conf);
    
private:
    void clear(GlobalConf& conf);
    int parseConf(GlobalConf& conf); 
    void dspConf(GlobalConf& conf);

    int parseLog(GlobalConf& conf);
    int parseParam(GlobalConf& conf);

private:
    Config m_parser;
};

extern int readFile(const char* file, typeStr& text);

#endif

