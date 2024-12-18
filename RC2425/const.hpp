#ifndef __H_CONSTANT
#define __H_CONSTANT

#define FAIL -1
#define SUCCESS 0

#define ONGOING_GAME 1
#define N_ONGOING_GAME 0


#define USER "user"
#define SERVER "server"
#define MAX_INPUT_SIZE 64
#define MAX_ANSWER_SIZE 1024
#define MAX_ANSWER_SIZE_SCORE 2048
#define MAX_BUFF_SIZE 256
#define MAX_PATHNAME 64
#define MAX_DIRNAME 16
#define MAX_FILENAME 24
#define MAX_SECRET_SIZE 5

#define MAX_STRING_UDP 128

#define MAX_FILES 10

#define MODE_PLAY 1
#define MODE_DEBUG 2


#define IP_DEFAULT ""
#define PORT_DEFAULT "58035"
#define DSPORT_DEFAULT "58035"


/********************* REQUEST *********************/
#define USER_COMMAND_START "SNG"
#define USER_COMMAND_TRY "TRY"
#define USER_COMMAND_QUIT "QUT"
#define USER_COMMAND_DEBUG "DBG"
#define USER_COMMAND_SHOW_STIALS "STR"
#define USER_COMMAND_SCOREBOARD "SSB"

/********************* ANSWER *********************/
#define SERVER_COMMAND_START "RSG"
#define SERVER_COMMAND_TRY "RTR"
#define SERVER_COMMAND_QUIT "RQT"
#define SERVER_COMMAND_DEBUG "RDB"
#define SERVER_COMMAND_SHOW_TRIALS "RST"
#define SERVER_COMMAND_SCOREBOARD "RSS"

#define ONGAME "ON"
#define NONGAME "NON"
#define WIN "WIN"
#define TIMEOUT "TIMEOUT"
#define LOSS "FAIL"
#define QUIT "QUIT"

#define YES "YES"
#define NO "NO"

#define ERR "ERR"
#define OK "OK"
#define NOK "NOK"
#define DUP "DUP"
#define ETM "ETM"
#define INV "INV"
#define ENT "ENT"
#define ACT "ACT"
#define EMPTY "EMPTY"
#define FIN "FIN"


#endif