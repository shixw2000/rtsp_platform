#include<cstdlib>
#include"service.h"
#include"shareheader.h"


extern void armSigs();

void usage(const char* prog) {
    LOG_ERROR("usage: %s [conf_file], examples:\n"
        "http_conf: %s <conf_file>\n"
        "http_conf: %s\n", 
        prog, prog, prog);
}

int httpConf(const char path[] = NULL) {
    int ret = 0;
    Service* svr = NULL;

    LOG_INFO("start http|");

    svr = new Service;

    do { 
        ret = svr->parseConf(path);
        if (0 != ret) {
            break;
        }

        ret = svr->init();
        if (0 != ret) {
            break;
        }

        ret = svr->prepare();
        if (0 != ret) {
            break;
        }

        svr->start();
        svr->wait();
    } while (0);

    svr->finish();
    delete svr;

    LOG_INFO("ret=%d| end http|", ret);
    
    return ret;
}

int testServer(int argc, char* argv[]) {
    int ret = 0;

    armSigs();

    if (2 == argc) {
        ret = httpConf(argv[1]);
    } else if (1 == argc) {
        ret = httpConf();
    } else {
        usage(argv[0]);
    }

    return ret;
}

