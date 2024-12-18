#ifndef __H_SERVER
#define __H_SERVER

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
#include <stdio.h>
#include <algorithm>
#include <time.h>

#include "../utils.hpp"
#include "../const.hpp"




class Server{
    typedef struct userlist{
        int PLID;
        std::string secret_key;
        int n_trial;
        time_t start_sec;
        std::vector<std::string> last_secret_keys;
        int max_playtime;
        std::string gameStatus; // ONGAME, NONGAME
        std::string mode; //PLAY, DEBUG
        char destinationfile[MAX_PATHNAME];
        std::string finishStatus; //WIN, FAIL, TIMEOUT, QUIT
        std::string restartGame; //YES, NO  --For start 2 time of same PLID
    } USERLIST;


    typedef struct scorelist{
        int score[MAX_FILES];
        char PLID[MAX_FILES][50];
        char col_code[MAX_FILES][50];
        int no_tries[MAX_FILES];
        int mode[MAX_FILES];
        int n_scores;
    } SCORELIST;

    bool m_verbose;
    std::string m_dsport;
    SOCKET * socketUDP, * socketTCP, * clientTCP;
    std::vector<USERLIST> user_list;
    int n_scoreboard = 0;

public:
    Server(int argc, char** argv);

private:
    void parse_arguments(int argc, char** argv);
    void initialize_connection();
    void connectUDP(std::string port);
    void connectTCP(std::string port);

    int create_dir(char * dirname);
    bool exists(const char* path);
    int delete_dir(char * dirname);
    int delete_file(char * pathname);
    void terminate();

    void receiveRequest();
    
    void handle_request_UDP(char *message);
    void handle_request_TCP(char *message, int client_fd);
    void handle_start(char *message);
    void handle_try(char * message);
    void score_dir(int PLID, int n_trial, std::string time_str, std::string secret_key, std::string mode);
    void handle_showtrails(char *message, int client_fd);
    void handle_scoreboard(int client_fd);
    void handle_quit(char *message);
    void handle_dbug(char *message);
    int FindTopScores(SCORELIST *list);
}; 


#endif