#ifndef __H_CLIENT
#define __H_CLIENT

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
#include <list>


#include "../utils.hpp"


using namespace std;
using namespace protocols;
using namespace parsers;

class User{
    SOCKET * socketUDP, * socketTCP;
    string _uid;
    string _ip;
    string _port;
    int try_time = 0;
    string _maxTime;

public:
    User(int argc, char** argv);

private:
    void read_input();
    void parse_arguments(int argc, char** argv);
    void Start(string input);
    void Try(string input);
    void Show_trials();
    void Scoreboard();
    void Quit();
    void Exit();
    void Debug(string input);
    void connectUDP(string ip, string port);
    void connectTCP(string ip, string port);
};

#endif