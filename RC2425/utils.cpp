#include "utils.hpp"
#include "const.hpp"

using namespace std;

namespace parsers{
    pair<int, int> parse_nB_nW(string user_secret_key, string guess_secret_key){
        int nB = 0; 
        int nW = 0; 

        int length = user_secret_key.size();
        std::vector<bool> black_matched(length, false); // Tracks black matches
        std::vector<bool> white_checked(length, false); // Tracks positions already matched as white

        // Step 1: Calculate black pegs
        for (int i = 0; i < length; ++i) {
            if (user_secret_key[i] == guess_secret_key[i]) {
                nB++;
                black_matched[i] = true; // Mark this position as matched for black
            }
        }

        // Step 2: Calculate white pegs
        for (int i = 0; i < length; ++i) {
            if (!black_matched[i]) { // Skip already matched black positions
                for (int j = 0; j < length; ++j) {
                    if (!black_matched[j] && !white_checked[j] && user_secret_key[j] == guess_secret_key[i]) {
                        nW++;
                        white_checked[j] = true; // Mark this position as matched for white
                        break;
                    }
                }
            }
        }

        return {nB, nW};
    }

    pair<time_t, string> getTime(int options){ /*int stands for fulltime, char stands for time_str*/
        time_t fulltime;
        struct tm *current_time;
        char time_str[20];
        time(&fulltime); // Get current time in seconds starting
        current_time = gmtime(&fulltime); // Convert time to YYYYMMDD HH:MM:SS.
        
        if (options == 0){
            sprintf(time_str,"%4d-%02d-%02d %02d:%02d:%02d",
                current_time->tm_year+1900, current_time->tm_mon+1, current_time->tm_mday,
                current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
        }
        else if (options == 1){
            sprintf(time_str,"%04d%02d%02d_%02d%02d%02d",
                current_time->tm_year+1900, current_time->tm_mon+1, current_time->tm_mday,
                current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
        }

        return {fulltime, string(time_str)};
    }
    
    string random_secretkey(){
        const vector<char> colors = {'R', 'G', 'B', 'Y', 'O', 'P'};
        const int keylength = 4;
        string secret_key;

        srand(time(0));

        for (int i = 0; i < keylength; ++i) {
            int randomIndex = std::rand() % colors.size();
            secret_key += colors[randomIndex];
        }

        return secret_key;
    }

    void CleanupTemporaryFiles(const char *directory, const char * d_name) {
        struct dirent *entry;
        DIR *dp = opendir(directory);
        char filepath[512];

        if (dp == NULL) {
            perror("opendir");
            return;
        }
        int d_name_len = strlen(d_name);
        while ((entry = readdir(dp))) {
            // Check if the file matches "TOPSCORES_*.txt"
            
            if (strncmp(entry->d_name, d_name, d_name_len) == 0 && strstr(entry->d_name, ".txt") != NULL) {
                snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
                if (remove(filepath) == 0) {
                    printf("Deleted: %s\n", filepath);
                } else {
                    perror("Failed to delete file");
                }
            }
        }

        closedir(dp);
    }
} 

namespace protocols{
    int sendUDP(SOCKET *s, string message) {
        if (!strcmp(s->owner, USER)) {
            if (sendto(s->fd, message.c_str(), message.length(), 0, s->res->ai_addr, s->res->ai_addrlen) == -1) {
                perror("sendto failed");
                return FAIL;
            }
        } else if (strcmp(s->owner, SERVER) == 0) {
            socklen_t addrlen = sizeof(struct sockaddr_in);
            if (sendto(s->fd, message.c_str(), message.length(), 0, (struct sockaddr *)&s->addr, addrlen) == -1) {
                perror("sendto failed");
                return FAIL;
            }
        }
        return SUCCESS;
    }

    int sendTCP(SOCKET * s, string message, int nbytes){
        ssize_t nleft, n;
        int nwritten = 0;
        char * ptr, buffer[MAX_BUFF_SIZE];
        ptr = strcpy(buffer, message.c_str());
        
        nleft=nbytes;
        while(nleft > 0){
            n = write(s->fd, ptr, nleft);
            if(n <= 0){
                fprintf(stderr, "Unable to send message, please try again!\n");
                return FAIL;
            }
            nleft -= n;
            ptr += n;
            nwritten += n;
        }
        return nwritten;
    }

    int sendTCP_SERVER(int clientfd, string message, int nbytes){
        ssize_t nleft, n;
        int nwritten = 0;
        char * ptr, buffer[2048];
        ptr = strcpy(buffer, message.c_str());
        
        nleft=nbytes;
        while(nleft > 0){
            n = write(clientfd, ptr, nleft);
            if(n <= 0){
                fprintf(stderr, "Unable to send message, please try again!\n");
                return FAIL;
            }
            nleft -= n;
            ptr += n;
            nwritten += n;
        }
        return nwritten;
    }
    
    int receiveUDP(SOCKET *s, char *message) {
        struct timeval tv;
        socklen_t addrlen = sizeof(s->addr);
        int tries = 0;

        while (tries < 3) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(s->fd, &rfds);

            tv.tv_sec = 3;  // 重置超时时间
            tv.tv_usec = 0;

            int recVal = select(s->fd + 1, &rfds, NULL, NULL, &tv);
            if (recVal == 0) {
                fprintf(stdout, "Timeout. %d out of 3 tries completed.\n", tries + 1);
            } else if (recVal == -1) {
                perror("select failed");
                exit(EXIT_FAILURE);
            } else {
                int bytes_received = recvfrom(s->fd, message, MAX_STRING_UDP, 0, (struct sockaddr *)&s->addr, &addrlen);

                if (bytes_received == -1) {
                    perror("recvfrom failed");
                    return FAIL;
                }
                message[bytes_received] = '\0';  // 确保消息以 NULL 结尾
                return SUCCESS;
            }
            tries++;
        }
        return FAIL;
    }

    int receiveTCP(SOCKET * s, char * message, int nbytes){
        ssize_t nleft, nread;
        char *ptr;
        nleft = nbytes;
        ptr = message;
        while(nleft > 0){
            nread = read(s->fd, ptr, nleft);
            if (nread == -1) {
                perror("Error reading from socket");
                return FAIL;
            } else if (nread == 0) {
                break;
            }
            nleft -= nread;
            ptr += nread;

        }
        nread = nbytes - nleft;
        return nread;
    }

    int sendstatusUDP_START(SOCKET * s, string command, string status){
        string buffer;
        if(status == ERR){
            buffer = status + "\n";
        }else{
            buffer = command + " " + status + "\n";
        }

        if(sendUDP(s, buffer) == FAIL){
           return FAIL;
        }
        
        return SUCCESS;
    }

    int sendstatusUDP_DBUG(SOCKET * s, string command, string status){
        string buffer;
            buffer = command + " " + status + "\n";

        if(sendUDP(s, buffer) == FAIL){
           return FAIL;
        }

        return SUCCESS;
    }

    int sendstatusUDP_TRY(SOCKET * s, string command, string status,int n_trial, int nB, int nW, string secret_key){
        string buffer;
        if(status == OK){
            buffer = command + " " + OK + " " + to_string(n_trial) + " " + to_string(nB) + " " + to_string(nW) + "\n";
            printf("%s\n", buffer.c_str());
        }

        else if(status == ERR || status == NOK || status == DUP || status == INV){
            buffer = command + " " + status + "\n";
        }
        else if(status == ENT || status == ETM){
            char s1 = secret_key[0];
            char s2 = secret_key[1];
            char s3 = secret_key[2];
            char s4 = secret_key[3];
            buffer = command + " " + status + " " + s1  + " " + s2 + " " + s3 + " " + s4 + "\n";
        }

        if(sendUDP(s, buffer) == FAIL){
           return FAIL;
        }

        return SUCCESS;
    }

    int sendstatusUDP_QUIT(SOCKET * s, string command, string status, string secret_key){
        string buffer;
        char s1 = secret_key[0];
        char s2 = secret_key[1];
        char s3 = secret_key[2];
        char s4 = secret_key[3];

        if(status == ERR){
            buffer = status + "\n";
        }
        else if (status == OK){
            buffer = command + " " + status + " " + s1 + " " + s2 + " " + s3 + " " + s4 + "\n";
        }
        else{
            buffer = command + " " + status + "\n";
        }

        if(sendUDP(s, buffer) == FAIL){
           return FAIL;
        }

        return SUCCESS;
    }

    int sendstatusTCP_ST(int clientfd, string command, string status, char *summaryFilePath, char * summaryFileName) {
        string buffer;
        if (status == ERR || status == NOK) {
            buffer =command + " " + status + "\n";
            if (sendTCP_SERVER(clientfd, buffer, buffer.length()) == FAIL){
                return FAIL;
            }
            return SUCCESS;
        }

        struct stat fileStat;
        stat(summaryFilePath, &fileStat);

        FILE* file = fopen(summaryFilePath, "rb");
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        string buffer_file(fileSize, '\0');
        fread(&buffer_file[0], 1, fileSize, file);

        fclose(file);

        buffer = command + " " + status + " "+ summaryFileName + " " + to_string(fileStat.st_size)  + " " + buffer_file +  "\n";

        if (sendTCP_SERVER(clientfd, buffer, buffer.length()) == FAIL){
            return FAIL;
        }

        return SUCCESS;
    }

    int sendstatusTCP_SB(int clientfd, string command, string status, char *scoreFilePath, char * scoreFileName){
        string buffer;
        struct stat fileStat;
        stat(scoreFilePath, &fileStat);
        if (status == EMPTY) {
            buffer =command + " " + status + "\n";
            return SUCCESS;
        } else {
            FILE* file = fopen(scoreFilePath, "rb");
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            string buffer_file(fileSize, '\0');
            fread(&buffer_file[0], 1, fileSize, file);

            fclose(file);
            buffer = command + " " + status + " "+ scoreFileName + " " + to_string(fileStat.st_size) +  " " + buffer_file + "\n";
        }
        if (sendTCP_SERVER(clientfd, buffer, buffer.length()) == FAIL){
            return FAIL;
        }

        return SUCCESS;
    }




    void disconnect(SOCKET * s){
        close(s->fd);
        free(s);
    }
}

namespace checkers{
    int check_maxtime(int maxtime){
        if(maxtime > 600){
            return false;
        }
        return true;
    }

    int check_PLID(int PLID){
        if (PLID >= 100000 && PLID <= 999999){
            return true;
        }
        return false;
    }
    
    int check_secretkey(std::string secret1, std::string secret2, std::string secret3, std::string secret4) {
        const std::vector<std::string> colors = {"R", "G", "B", "Y", "O", "P"};

        std::vector<std::string> secrets = {secret1, secret2, secret3, secret4};

        for (const auto& secret : secrets) {
            if (std::find(colors.begin(), colors.end(), secret) == colors.end()) {
                return 0;
            }
        }

        return 1;
    }
}