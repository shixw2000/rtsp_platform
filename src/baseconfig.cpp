#include<cstring>
#include<cstdlib>
#include<cstdio>
#include<fstream>
#include"baseconfig.h"
#include"shareheader.h"


using namespace std;

int BaseConfig::parseFile(const char* file,
    GlobalConf& conf) {
    int ret = 0;

    do { 
        clear(conf);

        ret = m_parser.parseFile(file);
        if (0 != ret) {
            break;
        }
        
        ret = parseLog(conf);
        if (0 != ret) {
            break;
        }
    
        ret = parseParam(conf);
        if (0 != ret) {
            break;
        }

        ret = parseConf(conf);
        if (0 != ret) {
            break;
        } 

        dspConf(conf);
    } while (0);

    return ret;
}

void BaseConfig::clear(GlobalConf& conf) {
    conf.m_log_level = 2;
    conf.m_log_stdin = 1;
    conf.m_log_size = 10;
    
    conf.m_log_dir.clear();
    
    conf.m_rd_thresh = 0;
    conf.m_wr_thresh = 0;
    conf.m_rd_timeout = 0;
    conf.m_wr_timeout = 0;
}


int BaseConfig::parseLog(GlobalConf& conf) {
    int ret = 0;
    int n = 0;
    typeStr logDir;

    ret = m_parser.getToken(SEC_LOG, DEF_KEY_LOG_DIR, logDir);
    if (0 == ret) { 
        ret = ConfLog(logDir.c_str());
    } else {
        ret = ConfLog(NULL);
    }
    
    CHK_RET(ret);
    conf.m_log_dir = logDir;

    ret = m_parser.getNum(SEC_LOG, DEF_KEY_LOG_LEVEL_NAME, n);
    CHK_RET(ret); 
    conf.m_log_level = n;

    SetLogLevel(conf.m_log_level);

    ret = m_parser.getNum(SEC_LOG, DEF_KEY_LOG_STDIN_NAME, n);
    CHK_RET(ret); 
    conf.m_log_stdin = n;

    SetLogScreen(conf.m_log_stdin);

    ret = m_parser.getNum(SEC_LOG, DEF_KEY_LOG_FILE_SIZE, n);
    CHK_RET(ret); 
    conf.m_log_size = n;

    SetMaxLogSize(conf.m_log_size);

    return ret;
}


int BaseConfig::parseParam(GlobalConf& conf) {
    int ret = 0;
    int n = 0;

    ret = m_parser.getNum(GLOBAL_SEC, "rd_thresh", n);
    CHK_RET(ret); 
    conf.m_rd_thresh = n;

    ret = m_parser.getNum(GLOBAL_SEC, "wr_thresh", n);
    CHK_RET(ret); 
    conf.m_wr_thresh = n;

    ret = m_parser.getNum(GLOBAL_SEC, "rd_timeout", n);
    CHK_RET(ret); 
    conf.m_rd_timeout = n;

    ret = m_parser.getNum(GLOBAL_SEC, "wr_timeout", n);
    CHK_RET(ret); 
    conf.m_wr_timeout = n;

    ret = m_parser.getNum(GLOBAL_SEC, KEY_ROLE, n);
    CHK_RET(ret); 
    conf.m_role = n;

    ret = m_parser.getNum(GLOBAL_SEC, "print_order", n);
    CHK_RET(ret); 
    conf.m_prnt_mask = (1 << n) - 1;

    return ret;
}

int BaseConfig::parseConf(GlobalConf& conf) {
    int ret = 0;
    int n = 0;
    typeStr token;

    if (0 == conf.m_role) {
        ret = m_parser.getToken(SEC_CLIENT, KEY_CONN_ADDR, token); 
        CHK_RET(ret);

        ret = m_parser.getAddr(conf.m_addr, token);
        CHK_RET(ret);

        ret = m_parser.getNum(SEC_CLIENT, "cliCnt", n);
        CHK_RET(ret); 
        conf.m_cli_cnt = n;

        ret = m_parser.getNum(SEC_CLIENT, "first_send_cnt", n);
        CHK_RET(ret); 
        conf.m_first_send_cnt = n;

        ret = m_parser.getToken(SEC_CLIENT, "method", token);
        CHK_RET(ret); 
        conf.m_method = token;

        ret = m_parser.getToken(SEC_CLIENT, "url", token);
        CHK_RET(ret); 
        conf.m_url = token;

        ret = m_parser.getToken(SEC_CLIENT, "http_body", token);
        CHK_RET(ret); 
        conf.m_body = token; 
    } else if (1 == conf.m_role) {
        ret = m_parser.getToken(SEC_SERVER, KEY_LISTEN_ADDR, token);
        CHK_RET(ret);

        ret = m_parser.getAddr(conf.m_addr, token);
        CHK_RET(ret);
    } else {
        LOG_ERROR("parse_conf| role=%d|"
            " msg=invalid role<0|1>|",
            conf.m_role);
        return -1;
    } 

    return ret;
}

void BaseConfig::dspConf(GlobalConf& conf) {
    LOG_INFO("dsp_conf| role=%d| addr=%s:%d|",
        conf.m_role,
        conf.m_addr.m_ip.c_str(),
        conf.m_addr.m_port);
}

int readFile(const char* file, typeStr& text) {
    ifstream infile;
    int len = 0;

    infile.open(file, ios_base::in|ios_base::binary);
    if (infile.good()) {
        infile.seekg(0, ios_base::end);
        len = infile.tellg();
        infile.seekg(0, ios_base::beg);

        if (0 < len) {
            text.resize(len);
            infile.read(&text[0], len);
        }

        infile.close();
        return 0;
    } else {
        LOG_ERROR("read_file| file=%s| msg=read error|",
            file);
        return -1;
    }
}

