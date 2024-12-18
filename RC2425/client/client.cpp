#include "client.hpp"
#include "../utils.hpp"
#include "../const.hpp"


User::User(int argc, char** argv){
    _uid = "";
    
    parse_arguments(argc, argv);

    connectUDP(_ip, _port);
    
    read_input();
}


void User::read_input(){
    char input[MAX_INPUT_SIZE];

    while(true){
        printf("please insert the command: ");
        fgets(input, sizeof(input), stdin);

        char command[MAX_INPUT_SIZE] = {'\0'};
        sscanf(input, "%s", command);

        if(strcmp(command, "start") == 0){
            Start(input);
        }
        else if(strcmp(command, "try") == 0){
            Try(input);
        }
        else if(strcmp(command, "show_trials") == 0 || strcmp(command, "st") == 0){
            Show_trials();
        }
        else if(strcmp(command, "scoreboard") == 0 || strcmp(command, "sb") == 0){
            Scoreboard();
        }
        else if(strcmp(command, "quit") == 0){
            Quit();
        }
        else if(strcmp(command, "exit") == 0){
            Exit();
        }
        else if(strcmp(command, "debug") == 0){
            Debug(input);
        }
    }
}

void User::parse_arguments(int argc, char** argv) {
    extern char* optarg;
    int max_argc = 1;

    char c;
    while((c = getopt(argc, argv, "n:p:")) != -1) {
        switch(c) {
            case 'n':
                _ip = optarg;
                max_argc += 2;
                break;
            case 'p':
                _port = optarg;
                max_argc += 2;
                break;
            default: 
                fprintf(stderr, "Usage: %s [-n DSIP] [-p DSport]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(_ip.empty()){
        _ip = IP_DEFAULT;
    }
        
    if(_port.empty()){
        _port = PORT_DEFAULT;
    }
        
    if(max_argc < argc) {
        fprintf(stderr, "Usage: %s [-n DSIP] [-p DSport]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("IP: %s\n", _ip.c_str());
    printf("PORT: %s\n", _port.c_str());
}

void User::Start(string input){
    char answer[MAX_INPUT_SIZE] = {'\0'};
    string command;
    string message;
    int PLID;
    int max_playtime;
    
    std::istringstream iss(input);
    
    iss >> command >> PLID >> max_playtime;

    if (iss >> std::ws && iss.eof()){
        _maxTime = to_string(max_playtime);

        message = "SNG " +  to_string(PLID) + " " + _maxTime + "\n";

        if(sendUDP(socketUDP, message) == FAIL){
            printf("Message can not send to server\n");
            return;
        }

        if(receiveUDP(socketUDP, answer) == FAIL){
            printf("Message can not receive from server\n");
            return;
        }
        std::string r_cmd, r_status;

        std::istringstream iss(answer);

        iss >> r_cmd >> r_status;
        if (r_cmd == SERVER_COMMAND_START){
            if(r_status == OK){
                printf("New game started (max %d sec)\n", max_playtime);
                try_time = 0;
                _uid = to_string(PLID);
            }
            else if (r_status == NOK){
                printf("Player has ongoing game.\n");
                // if (_uid != ""){
                //     return;
                // }
                // _uid = "";
            }
            else if (r_status == ERR){
                printf("ERROR! Please check the format of insert\n");
            }
        }
    }
    else{
        printf("The format is wrong! Please insert again!\n");
    }
}


void User::Try(string input) {
    string message, command, guess1, guess2, guess3, guess4;
    char answer[MAX_ANSWER_SIZE] = {'\0'};

    std::istringstream iss(input);

    iss >> command >> guess1 >> guess2 >> guess3 >> guess4;
    
    if (iss >> std::ws && iss.eof()){
        if (try_time <= 8){
            try_time += 1;
            message = "TRY" + string(" ") + _uid + " " + string(guess1) + " " + string(guess2) + " " + string(guess3) + " " + string(guess4) + " " + to_string(try_time) + "\n";
            std::cout << message;
            if(sendUDP(socketUDP, message) == FAIL){
                printf("Message can not send to server");
                return;
            }

            if(receiveUDP(socketUDP, answer) == FAIL){
                printf("Message can not receive from server");
                return;
            }
            std::cout << answer;
            std::istringstream iss(answer);

            std::string r_command, r_status;
            int r_trytime, r_black, r_white;

            iss >> r_command >> r_status;

            if(r_command == SERVER_COMMAND_TRY){
                if(r_status == OK){
                    iss >> r_trytime >> r_black >> r_white;
                    printf("nB = %d, nW = %d\n", r_black, r_white);
                    if (r_black == 4){
                        printf("Congratulation! You won the game!!\n");
                    }
                }
                else if(r_status == DUP){
                    printf("You are repeated the same guess as previos\n");
                    try_time -= 1;
                }
                else if(r_status == INV){
                    printf("nT is not the expected value\n");
                }
                else if(r_status == NOK){
                    try_time -= 1;
                    printf("The game was terminated, can not use try function.\n");
                }
                else if(r_status == ENT){
                    string secret1, secret2, secret3, secret4;
                    iss >> secret1 >> secret2 >> secret3 >> secret4;
                    printf("This is the eighth trial, the game will finish and the secret key is %s %s %s %s\n", secret1.c_str(), secret2.c_str(), secret3.c_str(), secret4.c_str());
                }
                else if(r_status == ETM){
                    string secret1, secret2, secret3, secret4;
                    iss >> secret1 >> secret2 >> secret3 >> secret4;
                    printf("The time is over, the game will finish and the secret key is %s %s %s %s\n", secret1.c_str(), secret2.c_str(), secret3.c_str(), secret4.c_str());
                }
                else if(r_status == ERR){
                    try_time -= 1;
                    printf("The syntax of message was incorrect! Please try again!\n");
                }
            }
        }
    }
    else{
        printf("The format is wrong! Please insert again!\n");
    }
}

void User::Show_trials(){
    connectTCP(_ip, _port);

    string message;
    char answer[MAX_ANSWER_SIZE] = {'\0'};
    message = "STR" + string(" ") + _uid + "\n";
    if(sendTCP(socketTCP, message, message.size()) == FAIL){
        printf("Message can not send to server\n");
        return;
    }

    if(receiveTCP(socketTCP, answer, MAX_ANSWER_SIZE) == FAIL){ /*ONLY read the first line of tcp received*/
        printf("Message can not receive from server\n");
        return;
    }

    istringstream iss(answer);
    string r_command, r_status, fileName;
    int Fsize;
    string Fdata;

    iss >> r_command >> r_status ;
    if (r_command == "RST"){
        if(r_status == "ACT"){
            printf("The game is still ongoing.\n");
            iss >> fileName >> Fsize;
            getline(iss >> ws, Fdata, '\0');
            printf("received trials file: \"%s\" (%d bytes)\n", fileName.c_str(), Fsize);
        }
        else if(r_status == "FIN"){
            iss >> fileName >> Fsize ;
            getline(iss >> ws, Fdata, '\0');
            printf("received trials file: \"%s\" (%d bytes)\n", fileName.c_str(), Fsize);

        }
        else if(r_status == "NOK"){
            printf("No game has found.\n");
            disconnect(socketTCP);
            return;
        }

        char saveFilePath[MAX_PATHNAME]; /*Path to save file*/
        sprintf(saveFilePath, "client/STATE_%s.txt", _uid.c_str());

        FILE * saveFile = fopen(saveFilePath, "w");
        if (saveFile == NULL) {
            perror("Failed to open the received file for reading");
            return;
        }
        fwrite(Fdata.c_str(), sizeof(char), strlen(Fdata.c_str()), saveFile);
        fclose(saveFile);

        saveFile = fopen(saveFilePath, "r");
        char buffer_line[MAX_BUFF_SIZE];
        while(fgets(buffer_line, sizeof(buffer_line), saveFile) != NULL){
            printf("%s",buffer_line);
        }
        fclose(saveFile);

    }

    disconnect(socketTCP);
}

void User::Scoreboard(){
    string message;
    char answer[MAX_ANSWER_SIZE_SCORE] = {'\0'};
    connectTCP(_ip, _port);
    message = string("SSB") + "\n";

    if(sendTCP(socketTCP, message, message.size()) == FAIL){
        printf("Message can not send to server");
        return;
    }

    if(receiveTCP(socketTCP, answer, MAX_ANSWER_SIZE_SCORE) == FAIL){
        printf("Message can not receive from server");
        return;
    }

    istringstream iss(answer);

    string command, status, fileName, Fdata;
    int Fsize;

    iss >> command >> status;


    if (command == "RSS"){
        if (command == EMPTY){
            printf("No game was yet won by any player\n");
            return;
        }
        else{
            iss >> fileName >> Fsize;
            getline(iss >> ws, Fdata, '\0');
            printf("response saved as \"%s\" (%d bytes):\n", fileName.c_str(), Fsize);

            char saveFilePath[MAX_PATHNAME]; /*Path to save file*/
            sprintf(saveFilePath, "client/%s", fileName.c_str());

            FILE * saveFile = fopen(saveFilePath, "w");
            if (saveFile == NULL) {
                perror("Failed to open the received file for reading");
                return;
            }
            fwrite(Fdata.c_str(), sizeof(char), strlen(Fdata.c_str()), saveFile);
            fclose(saveFile);
            saveFile = fopen(saveFilePath, "r");
            char buffer_line[MAX_BUFF_SIZE];
            while(fgets(buffer_line, sizeof(buffer_line), saveFile) != NULL){
                printf("%s",buffer_line);
            }
            fclose(saveFile);
        }
    }
    disconnect(socketTCP);
}

void User::Quit(){
    string message;
    char answer[MAX_ANSWER_SIZE] = {'\0'};
    message = "QUT" + string(" ") + _uid + "\n";
    
    if(sendUDP(socketUDP, message) == FAIL){
        printf("Message can not send to server");
        return;
    }

    if(receiveUDP(socketUDP, answer) == FAIL){
        printf("Message can not receive from server");
        return;
    }

    istringstream iss(answer);
    std::string r_command, r_status, secret1, secret2, secret3, secret4;

    iss >> r_command >> r_status ;


    if (r_command == SERVER_COMMAND_QUIT){
        if (r_status == OK){
            iss >> secret1 >> secret2 >> secret3 >> secret4;
            printf("The game is terminated. This is the secret key: %s %s %s %s\n", secret1.c_str(), secret2.c_str(), secret3.c_str(), secret4.c_str());
        }
        else if (r_status == NOK){
            printf("The game is already terminated.\n");
        }
        else{
            printf("Quit does not work, pleases exit!\n");
        }
    }
}

void User::Exit(){
    Quit();
    // CleanupTemporaryFiles("./client", "TOPSCORES_");
    // CleanupTemporaryFiles("./client", "STATE_");
    disconnect(socketUDP);
    exit(0);
}


void User::Debug(string input){
    int id, max_playtime;
    string command, guess1, guess2, guess3, guess4, message;
    char answer[MAX_ANSWER_SIZE] = {'\0'};

    std::istringstream iss(input);

    iss >> command >> id >> max_playtime >> guess1 >> guess2 >> guess3 >> guess4;

    message = "DBG " + to_string(id) + " " + to_string(max_playtime) + " " + guess1 + " " +  guess2 + " " + guess3 + " " + guess4 + "\n";

    // send and receive messages from UDP
    if(sendUDP(socketUDP, message) == FAIL){
        printf("Message can not send to server");
        return;
    }
    if(receiveUDP(socketUDP, answer) == FAIL){
        printf("Message can not receive from server");
        return;
    }

    iss.clear();
    iss.str(answer);
    string r_command, r_status;

    iss >> r_command >> r_status;
    //Conditions for each situation
    if (r_command == SERVER_COMMAND_DEBUG){
        if(r_status == OK){
            printf("Start a new game in debug mode.\n");
            _uid = to_string(id);
            _maxTime = to_string(max_playtime);
            try_time = 0;
        }
        else if(r_status == NOK){
            printf("Game still ongoing, please finish and try again.\n");
        }
        else if(r_status == ERR){
            printf("Error! Please check the format of insert\n");
        }
    }
}

/* UDP and TCP connect
*/
void User::connectUDP(string ip, string port){
    int fd, errcode;
    struct addrinfo hints, *res;

    fd = socket(AF_INET,SOCK_DGRAM,0);
    if(fd == FAIL){
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    } 

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if(errcode != SUCCESS){
        fprintf(stderr, "Unable to link to server.\n");
        exit(EXIT_FAILURE);
    } 

    socketUDP = (SOCKET *) malloc(sizeof(SOCKET));
    strcpy(socketUDP->owner, USER);
    socketUDP->fd = fd;
    socketUDP->res = res;
    
}

void User::connectTCP(string ip, string port){
    int fd, errcode;
    struct addrinfo hints, *res;

    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == FAIL){
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if(errcode != SUCCESS){
        fprintf(stderr, "Unable to link to server.\n");
        exit(EXIT_FAILURE);
    }

    if(connect(fd,res->ai_addr, res->ai_addrlen) == FAIL){
        fprintf(stderr, "Unable to connect to server");
        exit(EXIT_FAILURE);
    }

    socketTCP = (SOCKET *) malloc(sizeof(SOCKET));
    strcpy(socketTCP->owner, USER);
    socketTCP->fd = fd;
    socketTCP->res = res;
}

int main(int argc, char** argv) {
    User(argc, argv);
    return 0;
}