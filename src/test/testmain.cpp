

extern int testServer(int argc, char* argv[]);

static int test(int argc, char* argv[]) {
    int ret = 0; 

    ret = testServer(argc, argv); 
    return ret;
}

int main(int argc, char* argv[]) {
    int ret = 0;

    ret = test(argc, argv);

    return ret;
}

