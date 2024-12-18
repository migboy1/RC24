#ifndef __H_UTILS
#define __H_UTILS

#include <vector>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <cstring>
#include <string>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>



namespace parsers{
    std::pair<int, int> parse_nB_nW(std::string user_secret_key, std::string guess_secret_key);
    std::pair<time_t, std::string> getTime(int options);
    std::string random_secretkey();
    void CleanupTemporaryFiles(const char *directory,const char * d_name);
}

typedef struct SOCK{
    char owner[7]; /* Which application uses the socket: USER or SERVER */
    int fd;
    struct addrinfo * res;
    struct sockaddr_in addr;
} SOCKET;



namespace protocols{
    int sendUDP(SOCKET * s, std::string message);
    int sendTCP(SOCKET * s, std::string message, int nbytes);
    int sendTCP_SERVER(int clientfd, std::string message, int nbytes);

    int receiveUDP(SOCKET * s, char * message);
    int receiveTCP(SOCKET * s, char * message, int nbytes);

    int sendstatusUDP_START(SOCKET * s, std::string command, std::string status);
    int sendstatusUDP_TRY(SOCKET * s, std::string command, std::string status,int n_trial, int nB, int nW, std::string secret_key);
    int sendstatusUDP_QUIT(SOCKET * s, std::string command, std::string status, std::string secret_key);
    int sendstatusUDP_DBUG(SOCKET * s, std::string command, std::string status);
    
    int sendstatusTCP_ST(int clientfd, std::string command, std::string status, char * summaryFilePath, char * summaryFileName);
    int sendstatusTCP_SB(int clientfd, std::string command, std::string status, char *scoreFilePath, char * scoreFileName);
    void disconnect(SOCKET * s);
}

namespace checkers{
    int check_maxtime(int maxtime);
    int check_PLID(int PLID);
    int check_secretkey(std::string secret1, std::string secret2, std::string secret3, std::string secret4);
}

#endif