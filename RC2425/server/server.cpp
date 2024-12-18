#include "server.hpp"
#include "../utils.hpp"
#include "../const.hpp"


Server::Server(int argc, char** argv){
    m_verbose = false;

    parse_arguments(argc, argv);

    initialize_connection();

    receiveRequest();
}

void Server::parse_arguments(int argc, char** argv){
    extern char* optarg;
    int max_argc = 1;

    char c;
    while((c = getopt(argc, argv, "p:v")) != -1) {
        switch(c) {
            case 'p':
                m_dsport = optarg;
                max_argc += 2;
                break;
            case 'v':
                m_verbose = true;
                max_argc += 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p DSport] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(m_dsport.empty())
        m_dsport = DSPORT_DEFAULT;

    if(max_argc < argc) {
        fprintf(stderr, "Usage: %s [-p DSport] [-v]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}

void Server::initialize_connection(){
    connectUDP(m_dsport);
    connectTCP(m_dsport);
}

void Server::connectUDP(std::string port) {
    int fd;
    struct addrinfo *res;
    struct addrinfo hints;
    int errcode;


    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Bind failed");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    socketUDP = (SOCKET *)malloc(sizeof(SOCKET));
    if (!socketUDP) {
        perror("Memory allocation failed");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }
    strcpy(socketUDP->owner, SERVER);
    socketUDP->fd = fd;
    socketUDP->res = res;
}

void Server::connectTCP(std::string port){
    int fd;
    int n;
    struct sockaddr_in servaddr;

    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == FAIL){
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(stoi(port));

    n = bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(n == FAIL){
        fprintf(stderr, "Unable to bind.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(fd, 10) == FAIL){
        fprintf(stderr, "Unable to listen.\n");
        exit(EXIT_FAILURE);
    }

    socketTCP = (SOCKET *) malloc(sizeof(SOCKET));
    strcpy(socketTCP->owner, SERVER);
    socketTCP->fd = fd;
    socketTCP->addr = servaddr;
}

int Server::create_dir(char * dirname){
    if (mkdir(dirname, 0700) == FAIL){
        fprintf(stderr, "Unable to create %s directory.\n", dirname);
        return FAIL;
    }
    return SUCCESS;
}

bool Server::exists(const char* path) {
    return access(path, F_OK) == 0; // F_OK checks for existence
}

int Server::delete_dir(char * dirname){    
    if (rmdir(dirname) == FAIL){
        fprintf(stderr, "Unable to delete %s directory.\n", dirname);
        return FAIL;
    }
    return SUCCESS;
}

int Server::delete_file(char * pathname){

    if(!fopen(pathname, "w")){
        return FAIL;
    }
    if (remove(pathname) != SUCCESS){
        return FAIL;
    }
    return SUCCESS;
}

void Server::terminate(){
    protocols::disconnect(socketUDP);
    protocols::disconnect(socketTCP);
    exit(EXIT_SUCCESS);
}

// TO receive Request between UDP and TCP
void Server::receiveRequest() {
    fd_set inputs, testfds;
    char buffer[MAX_BUFF_SIZE];
    int max_fd;
    if (!socketUDP || !socketTCP) {
        fprintf(stderr, "Sockets not initialized. Call connectUDP and connectTCP first.\n");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&inputs);
    FD_SET(socketUDP->fd, &inputs);
    FD_SET(socketTCP->fd, &inputs);

    max_fd = std::max(socketUDP->fd, socketTCP->fd);

    while (true) {
        testfds = inputs;

        int out_fds = select(max_fd + 1, &testfds, NULL, NULL, NULL);

        switch (out_fds) {
            case 0: // Timeout
                break;
            case -1: // Error
                perror("select");
                exit(EXIT_FAILURE);
            default:
                // Check UDP socket
                if (FD_ISSET(socketUDP->fd, &testfds)) {
                    socklen_t addr_len = sizeof(socketUDP->addr);
                    int ret = recvfrom(socketUDP->fd, buffer, MAX_BUFF_SIZE - 1, 0, (struct sockaddr*)&socketUDP->addr, &addr_len);

                    if (ret >= 0) {
                        buffer[ret] = '\0';
                        handle_request_UDP(buffer);
                    } else {
                        perror("recvfrom failed");
                    }
                }

                // Check TCP socket
                if (FD_ISSET(socketTCP->fd, &testfds)) {
                    socklen_t addr_len = sizeof(socketTCP->addr);
                    int client_fd = accept(socketTCP->fd, (struct sockaddr*)&socketTCP->addr, &addr_len);
                    if (client_fd == FAIL) {
                        perror("accept");
                        continue;
                    }

                    int ret = read(client_fd, buffer, MAX_BUFF_SIZE);
                    if (ret > 0) {
                        buffer[ret] = '\0'; // Null-terminate the std::string
                        std::cout << "[TCP] Received message: " << buffer << std::endl;
                        handle_request_TCP(buffer, client_fd);
                    } else if (ret == 0) {
                        std::cout << "[TCP] Client disconnected" << std::endl;
                    } else {
                        perror("recv");
                    }
                    close(client_fd);
                }
        }
    }
}

void Server::handle_request_UDP(char *message) {
    std::istringstream iss(message);
    std::string command;

    iss >> command;
    if (command == USER_COMMAND_START) {
        handle_start(message);
    } else if (command == USER_COMMAND_TRY) {
        handle_try(message);
    } else if (command == USER_COMMAND_QUIT) {
        handle_quit(message);
    } else if (command == USER_COMMAND_DEBUG) {
        handle_dbug(message);
    }
}

void Server::handle_request_TCP(char *message, int client_fd) {
    std::istringstream iss(message);
    std::string command;

    iss >> command;

    if (command == USER_COMMAND_SHOW_STIALS) {
        handle_showtrails(message, client_fd);
    } else if (command == USER_COMMAND_SCOREBOARD) {
        handle_scoreboard(client_fd);
    }
}

/*DONE*/
void Server::handle_start(char *message){
    std::cout << message << std::endl;

    std::string command;
    int PLID, max_playtime;
    std::istringstream iss(message);

    iss >> command >> PLID >> max_playtime;

    if(!checkers::check_PLID(PLID) || !checkers::check_maxtime(max_playtime)){
        protocols::sendstatusUDP_START(socketUDP, SERVER_COMMAND_START, ERR);
    }

    auto resultTime = parsers::getTime(0);

    /*check if the player information already saved */
    auto it = find_if(user_list.begin(), user_list.end(), [PLID](const USERLIST& user){
        return user.PLID == PLID;
    });
    
    /*if saved, checks if it already ongame*/
    if (it != user_list.end()){
        size_t time_left = it->max_playtime + it->start_sec - resultTime.first;
        if (it->gameStatus == ONGAME && time_left > 0 && it->restartGame == NO){
            protocols::sendstatusUDP_START(socketUDP, SERVER_COMMAND_START, NOK);
            // printf("good");
            return;
        }
    }
    
    /*Create the file GAMES/GAME_(PLID).txt */
    char playerfilepath[MAX_PATHNAME] = {'\0'};
    sprintf(playerfilepath, "GAMES/GAME_%d.txt", PLID); 
    FILE * playerfile = fopen(playerfilepath, "w");
    if (!playerfile){
        protocols::sendstatusUDP_START(socketUDP, SERVER_COMMAND_START, NOK);
        // printf("bad\n");
        return;
    }

    /* Create 4 colors secret key */
    std::string secret_key;
    secret_key = parsers::random_secretkey();

    char buffer[MAX_BUFF_SIZE];
    sprintf(buffer, "%d P %s %03d %s %ld\n",
            PLID, secret_key.c_str(), max_playtime, resultTime.second.c_str(), resultTime.first);
    
    fwrite(buffer, sizeof(char), strlen(buffer), playerfile);

    /*if not onGame, reinitial with new informations*/
    if(it != user_list.end()){
        it->secret_key = secret_key;
        it->n_trial = 1;
        it->start_sec = resultTime.first;
        it->last_secret_keys = std::vector<std::string>(8, "");
        it->max_playtime = max_playtime;
        it->gameStatus = ONGAME;
        it->mode = "P";
        strncpy(it->destinationfile, playerfilepath, MAX_PATHNAME);
        it->destinationfile[MAX_PATHNAME - 1] = '\0';
        it->finishStatus = "";
        it->restartGame = YES;
    }
    /*Create new player */
    else{
        USERLIST new_user = {PLID, secret_key, 1, resultTime.first, std::vector<std::string>(8, "") , max_playtime, ONGAME, "P", "", "", YES};
        strncpy(new_user.destinationfile, playerfilepath, MAX_PATHNAME);
        new_user.destinationfile[MAX_PATHNAME - 1] = '\0';
        user_list.push_back(new_user);
    }
    
    fclose(playerfile);
    protocols::sendstatusUDP_START(socketUDP, SERVER_COMMAND_START, OK);
    return;
}


void Server::handle_try(char * message){
    int PLID;
    std::string command, secret1 ,secret2, secret3, secret4;
    int n_trial = 0;
    printf("%s\n", message);

    std::istringstream iss(message);

    iss >> command >> PLID >> secret1 >> secret2 >> secret3 >> secret4 >> n_trial;

    if (checkers::check_secretkey(secret1, secret2, secret3, secret4) == 0){
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, ERR, 0, 0, 0, "");
        return;
    }

    auto it = find_if(user_list.begin(), user_list.end(), [PLID](const USERLIST& user){
        return user.PLID == PLID;
    });

    /*if the game is not ongame, return status NOK to client */
    if (it != user_list.end()){
        if (it->gameStatus == NONGAME){
            protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, NOK, 0, 0, 0, "");
            return;
        }
        
    }
    /*Open the file GAMES/GAME_(PLID).txt */
    char playerfilepath[MAX_PATHNAME] = {'\0'};
    sprintf(playerfilepath, "GAMES/GAME_%s.txt", std::to_string(PLID).c_str());
    FILE * playerfile = fopen(playerfilepath, "a");

    /*Get time*/
    auto resultTime = parsers::getTime(1);
    it->restartGame = NO;
    strcmp(it->destinationfile, playerfilepath);
    std::string guess_secret_key =secret1 + secret2 + secret3 + secret4;

    /*Find the nB and nW */
    auto result = parsers::parse_nB_nW(it->secret_key, guess_secret_key); 

    std::cout << it->n_trial-1 << n_trial << it->last_secret_keys[n_trial - 1] << guess_secret_key <<std::endl;
    if ((it->n_trial - 1 == n_trial) && it->last_secret_keys[n_trial - 1] == guess_secret_key){
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, OK, n_trial, result.first , result.second, "");
        return;
    }

    /*Check if the secret key is duplicated */ 
    int inx = 0;
    while(inx < n_trial){
        if (strcmp(guess_secret_key.c_str(), it->last_secret_keys[inx].c_str()) == 0){
            protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, DUP, 0, 0, 0, "");
            fclose(playerfile);
            return;
        }
        inx ++;
    }
    it->last_secret_keys[it->n_trial - 1] = guess_secret_key.c_str();
    

    /*Check if the n_trial is not correct number*/
    if (n_trial != it->n_trial){
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, INV, 0, 0, 0, "");
        fclose(playerfile);
        return;
    }

    it->n_trial ++;

    char message_file[MAX_BUFF_SIZE];
    
    char destinationFile[MAX_PATHNAME] = {'\0'};
    char destinationDir[MAX_DIRNAME];
    sprintf(destinationDir, "GAMES/%d",PLID);
    /*Calculate the time of user used */
    time_t sec_needed = resultTime.first - it->start_sec;
    if(sec_needed > it->max_playtime || (n_trial == 8 && result.first != 4) || result.first == 4){
        if (!exists(destinationDir)) {
            create_dir(destinationDir);
        }
    }

    /*Check if the time is already exceeded*/
    if (sec_needed > it->max_playtime){
        it->gameStatus = NONGAME; /*the game is finished, not on game*/
        it->finishStatus = TIMEOUT;
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, ETM, 0, 0, 0, guess_secret_key); /* 0 means NULL*/
        sprintf(destinationFile, "GAMES/%s/%s_T.txt", std::to_string(PLID).c_str(), resultTime.second.c_str());
        fclose(playerfile);
        rename(playerfilepath, destinationFile);
        return;
    }

    /*Write message into file */
    sprintf(message_file, "T: %s %d %d %ld\n", guess_secret_key.c_str(), result.first, result.second, sec_needed);
    int nwritten = fwrite(message_file, sizeof(char), strlen(message_file), playerfile);
    if (nwritten < (int) strlen(message_file)){
        fclose(playerfile);
        delete_file(playerfilepath);
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, NOK, 0, 0, 0, "");
        return;
    }
    
    /*no more attemps are available and not win yet*/
    if (n_trial == 8 && result.first != 4){
        it->gameStatus = NONGAME;
        it->finishStatus = LOSS;
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, ENT, 0, 0, 0, guess_secret_key);
        sprintf(destinationFile, "GAMES/%s/%s_F.txt", std::to_string(PLID).c_str(), resultTime.second.c_str());
        fclose(playerfile);
        strcmp(it->destinationfile, destinationFile);
        rename(playerfilepath, destinationFile);
        return;
    }

    /*While WIN the GAME */
    if (result.first == 4){
        it->gameStatus = NONGAME;
        it->finishStatus = WIN;
        protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, OK, n_trial, result.first , result.second, "");
        sprintf(destinationFile, "GAMES/%s/%s_W.txt", std::to_string(PLID).c_str(), resultTime.second.c_str());
        strcmp(it->destinationfile, destinationFile);
        fclose(playerfile);
        rename(playerfilepath, destinationFile);

        playerfile = fopen(destinationFile, "a");
        char buffer[MAX_BUFF_SIZE] = {'\0'};
        resultTime = parsers::getTime(1);
        sprintf(buffer, "%s %ld\n", resultTime.second.c_str(), sec_needed);
        nwritten = fwrite(buffer, sizeof(char), strlen(buffer), playerfile);
        score_dir(PLID, n_trial, resultTime.second, it->secret_key, it->mode);
        it->gameStatus = NONGAME;
        return;
    }
    protocols::sendstatusUDP_TRY(socketUDP, SERVER_COMMAND_TRY, OK, n_trial, result.first , result.second, "");
    fclose(playerfile);
}

void Server::score_dir(int PLID, int n_trial, std::string time_str, std::string secret_key, std::string mode){
    char destinationfile[MAX_PATHNAME] = {"\0"};
    int score = 100 - (8 * (n_trial - 1));
    sprintf(destinationfile, "SCORES/%03d_%d_%s.txt",score, PLID, time_str.c_str());
    char buffer[MAX_BUFF_SIZE] = {"\0"};
    sprintf(buffer, "%03d %d %s %d %s", score, PLID, secret_key.c_str(), n_trial, mode.c_str());
    FILE * playerfile = fopen(destinationfile, "w");
    fwrite(buffer, sizeof(char), strlen(buffer), playerfile);
    fclose(playerfile);
}

void Server::handle_showtrails(char *message, int client_fd){
    printf("%s\n", message);
    int PLID;
    sscanf(message,"STR %d", &PLID);
    auto it = find_if(user_list.begin(), user_list.end(), [PLID](const USERLIST& user){
        return user.PLID == PLID;
    });

    if (it == user_list.end()){
        protocols::sendstatusTCP_ST(client_fd, SERVER_COMMAND_SHOW_TRIALS, NOK, nullptr, nullptr);
        return;
    }
    char buffer_line[MAX_BUFF_SIZE];
    char buffer[MAX_BUFF_SIZE] = {'\0'};
    FILE * playerfile = fopen(it->destinationfile, "r");
    if (playerfile == NULL) {
        perror("Failed to open file");
        return;
    }
    char game_mode;
    char secret_key[MAX_SECRET_SIZE] = {'\0'};
    int  year, month, day, hour, min, sec, nB, nW, used_sec, fulltime;
    time_t max_playtime;

    /* CREAT SUMMARY FILE TO PLAYER*/
    char summaryFilePath[MAX_PATHNAME] = {'\0'};
    char summaryFileName[MAX_FILENAME] = {'\0'};
    sprintf(summaryFileName, "STATE_%d.txt", PLID);
    sprintf(summaryFilePath, "GAMES/%s", summaryFileName);
    FILE * summaryFile = fopen(summaryFilePath, "w");

    auto resultTime = parsers::getTime(0);
    time_t time_left = 0;

    while(fgets(buffer_line, sizeof(buffer_line), playerfile) != NULL){
        if(sscanf(buffer_line, "%d %c %31s %ld %4d-%2d-%2d %2d:%2d:%2d %d", &PLID, &game_mode, secret_key, &max_playtime, &year, &month, &day, &hour, &min, &sec, &fulltime) == 11){
            time_left = it->start_sec + max_playtime - resultTime.first;
            sprintf(buffer, "   Active game found for player %d\nGame initiated: %04d-%02d-%02d %02d:%02d:%02d with %ld seconds to be completed\n\n", 
            PLID, year, month, day, hour, min, sec, max_playtime);
            fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
            if (time_left <= 0){
                it->gameStatus = NONGAME;

                char destinationFile[MAX_PATHNAME] = {'\0'};
                char destinationDir[MAX_DIRNAME];
                sprintf(destinationDir, "GAMES/%d",PLID);
                if (!exists(destinationDir)) {
                    create_dir(destinationDir);
                }
                it->finishStatus = TIMEOUT;
                sprintf(destinationFile, "GAMES/%s/%s_T.txt", std::to_string(PLID).c_str(), resultTime.second.c_str());
                rename(it->destinationfile, destinationFile);
                strcmp(it->destinationfile, destinationFile);
                
                if(it->mode == "P"){
                    sprintf(buffer, "Mode: PLAY     Secret code:%s\n\n", it->secret_key.c_str());
                    fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
                }
                else{
                    sprintf(buffer, "Mode: DEBUG     Secret code:%s\n\n", it->secret_key.c_str());
                    fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
                }
            }
            if (it->n_trial == 1){
                sprintf(buffer, "     Game started - no transactions found\n\n");
                fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
                break;
            }else{
                sprintf(buffer, "     --- Transactions found: %d ---\n\n",it->n_trial-1);
                fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
            }
        }
        else if(sscanf(buffer_line, "T: %s %d %d %d", secret_key, &nB, &nW, &used_sec) == 4){
            sprintf(buffer, "Trial: %s, nB:%d nW:%d at     %ds\n", secret_key, nB, nW, used_sec);
            fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
        }
    }
    fclose(playerfile);
    
    if(it->gameStatus == ONGAME){
        sprintf(buffer, "   -- %ld seconds remaining to be completed --\n", time_left);
        fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
        fclose(summaryFile);
        protocols::sendstatusTCP_ST(client_fd, SERVER_COMMAND_SHOW_TRIALS, ACT, summaryFilePath, summaryFileName);
        return;
    }

    //if the game is finished, define 4 different finish way
    else if(it->gameStatus == NONGAME){
        printf("haa\n");
        if(it->finishStatus == TIMEOUT){
            sprintf(buffer, "   Termination: TIMEOUT at %s, Duration: %d\n", resultTime.second.c_str(), it->max_playtime);
        }
        else if(it->finishStatus == WIN){
            sprintf(buffer, "   Termination: WIN at %s, Duration: %d\n", resultTime.second.c_str(), it->max_playtime);
        }
        else if(it->finishStatus == LOSS){
            sprintf(buffer, "   Termination: FAIL at %s, Duration: %d\n", resultTime.second.c_str(), it->max_playtime);
        }
        else if(it->finishStatus == QUIT){
            sprintf(buffer, "   Termination: QUIT at %s, Duration: %d\n", resultTime.second.c_str(), it->max_playtime);
        }
        fwrite(buffer, sizeof(char), strlen(buffer), summaryFile);
        fclose(summaryFile);
        protocols::sendstatusTCP_ST(client_fd, SERVER_COMMAND_SHOW_TRIALS, FIN, summaryFilePath, summaryFileName);
    }
}

void Server::handle_scoreboard(int client_fd){
    SCORELIST scores;
    memset(&scores, 0, sizeof(SCORELIST));
    int n_result = FindTopScores(&scores);

    if (n_result == 0){
        protocols::sendstatusTCP_SB(client_fd, SERVER_COMMAND_SCOREBOARD , EMPTY, nullptr, nullptr);
        return;
    }
    int i;
    n_scoreboard ++;
    char scoreFileName[MAX_FILENAME] = {'\0'};
    char scoreFilePath[MAX_PATHNAME] = {'\0'};
    sprintf(scoreFileName, "TOPSCORES_%06d.txt", n_scoreboard);
    sprintf(scoreFilePath, "SCORES/%s", scoreFileName);
    
    FILE *scoreFile = fopen(scoreFilePath, "w");
    char buffer[2048] = {'\0'};

    sprintf(buffer,"-------------------------------- TOP 10 SCORES --------------------------------\n\n");
    fwrite(buffer, sizeof(char), strlen(buffer), scoreFile);
    sprintf(buffer,"                SCORE PLAYER     CODE    NO TRIALS   MODE\n\n");
    fwrite(buffer, sizeof(char), strlen(buffer), scoreFile);

    for(i = 0; i < n_result; i++){
        if(scores.mode[i] == 1){
            sprintf(buffer,"             %d - %03d  %s     %s        %d       PLAY\n",i, scores.score[i], scores.PLID[i], scores.col_code[i], scores.no_tries[i]);
            fwrite(buffer, sizeof(char), strlen(buffer), scoreFile);
        }else{
            sprintf(buffer,"             %d - %03d  %s     %s        %d       DEBUG\n",i, scores.score[i], scores.PLID[i], scores.col_code[i], scores.no_tries[i]);
            fwrite(buffer, sizeof(char), strlen(buffer), scoreFile);
        }
    }
    fclose(scoreFile);
    protocols::sendstatusTCP_SB(client_fd, SERVER_COMMAND_SCOREBOARD ,OK, scoreFilePath, scoreFileName);
    // delete_file(scoreFilePath);
    return;
}

void Server::handle_quit(char *message){
    printf("%s\n", message);
    int PLID;

    sscanf(message, "QUT %d", &PLID);

    auto it = find_if(user_list.begin(), user_list.end(), [PLID](const USERLIST& user){
        return user.PLID == PLID;
    });

    if (it == user_list.end()){
        return;
    }
    if (it->gameStatus == ONGAME){
        it->gameStatus == NONGAME;
        it->finishStatus = QUIT;
        char destinationFile[MAX_PATHNAME] = {'\0'};
        auto resultTime = parsers::getTime(1);
        char destinationDir[MAX_DIRNAME];
        sprintf(destinationDir, "GAMES/%d",PLID);
        if (!exists(destinationDir)) {
            create_dir(destinationDir);
        }
        sprintf(destinationFile, "GAMES/%s/%s_Q.txt", std::to_string(PLID).c_str(), resultTime.second.c_str());
        rename(it->destinationfile, destinationFile);
        protocols::sendstatusUDP_QUIT(socketUDP, SERVER_COMMAND_QUIT, OK, it->secret_key);
        return;
    }
    else{
        protocols::sendstatusUDP_QUIT(socketUDP, SERVER_COMMAND_QUIT, NOK, "");
        return;
    }

    protocols::sendstatusUDP_QUIT(socketUDP, SERVER_COMMAND_QUIT, ERR, "");
}

void Server::handle_dbug(char *message){
    std::cout << message << std::endl;
    int PLID, max_playtime;
    std::string command, s1, s2, s3, s4;

    std::istringstream iss(message);

    iss >> command >> PLID >> max_playtime >> s1 >> s2 >> s3 >> s4;


    if(!checkers::check_maxtime(max_playtime) || !checkers::check_PLID(PLID) || !checkers::check_secretkey(s1, s2, s3, s4) ){
        protocols::sendstatusUDP_DBUG(socketUDP, SERVER_COMMAND_DEBUG, ERR);
        return;
    }

    auto it = find_if(user_list.begin(), user_list.end(), [PLID](const USERLIST& user){
        return user.PLID == PLID;
    });
    std::string secret_key =s1 + s2 + s3 + s4; 
    auto resultTime = parsers::getTime(0);

    char playerfilepath[MAX_PATHNAME] = {'\0'};
    sprintf(playerfilepath, "GAMES/GAME_%d.txt", PLID); 
    FILE * playerfile = fopen(playerfilepath, "w");
    if (it == user_list.end()){
        USERLIST new_user = {PLID, secret_key, 01, resultTime.first, std::vector<std::string>(8, "") , max_playtime, ONGAME, "P", ""};
        strncpy(new_user.destinationfile, playerfilepath, MAX_PATHNAME);
        new_user.destinationfile[MAX_PATHNAME - 1] = '\0';
        user_list.push_back(new_user);
        protocols::sendstatusUDP_DBUG(socketUDP, SERVER_COMMAND_DEBUG, OK);
        char buffer[MAX_BUFF_SIZE];
        sprintf(buffer, "%d D %s %03d %s %ld\n",
                PLID, secret_key.c_str(), max_playtime, resultTime.second.c_str(), resultTime.first);
        fwrite(buffer, sizeof(char), strlen(buffer), playerfile);
        fclose(playerfile);
        return;
    }
    if (it->gameStatus == NONGAME){
        it->secret_key = secret_key;
        it->n_trial = 1;
        it->start_sec = resultTime.first;
        it->last_secret_keys = std::vector<std::string>(8, "");
        it->max_playtime = max_playtime;
        it->gameStatus = ONGAME;
        it->mode = "D";
        strncpy(it->destinationfile, playerfilepath, MAX_PATHNAME);
        it->destinationfile[MAX_PATHNAME - 1] = '\0';
        protocols::sendstatusUDP_DBUG(socketUDP, SERVER_COMMAND_DEBUG, OK);
        char buffer[MAX_BUFF_SIZE];
        sprintf(buffer, "%d D %s %03d %s %ld\n",
                PLID, secret_key.c_str(), max_playtime, resultTime.second.c_str(), resultTime.first);
        fwrite(buffer, sizeof(char), strlen(buffer), playerfile);
        fclose(playerfile);
        return;
    }
    else{
        protocols::sendstatusUDP_DBUG(socketUDP, SERVER_COMMAND_DEBUG, NOK);
        fclose(playerfile);
        return;
    }
}

/*Find top 10 Scores*/
int Server::FindTopScores(SCORELIST *list) {
    struct dirent **filelist;
    int n_entries, i_file;
    char fname[300];
    char mode[8];
    FILE *fp;

    n_entries = scandir("SCORES/", &filelist, NULL, alphasort);
    if (n_entries <= 0) {
        return 0; // No files found
    }
    else{
        i_file = 0;
        while (n_entries--) {
            if (filelist[n_entries]->d_name[0] != '.' && i_file < 10) {
                sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
                fp = fopen(fname, "r");
                if (fp != NULL) {
                    if (fscanf(fp, "%d %s %s %d %s",
                            &list->score[i_file],list->PLID[i_file],list->col_code[i_file],&list->no_tries[i_file], mode) == 5) {
                        if (strcmp(mode, "P") == 0)
                            list->mode[i_file] = MODE_PLAY;
                        else if (strcmp(mode, "D") == 0)
                            list->mode[i_file] = MODE_DEBUG;

                        i_file++;
                    }
                    
                    fclose(fp);
                }
            }
            free(filelist[n_entries]);
        }
    free(filelist);
    }
    list->n_scores = i_file;
    return i_file;
}

int main(int argc, char** argv) {
    Server(argc, argv);
    return 0;
}