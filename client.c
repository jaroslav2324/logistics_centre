
#include "util.h"

#define MY_ADDR "127.0.0.1"

int sockfd;

int i_am_worker_flg = 0;

char username[USERNAME_LEN];
message msg; // buffer



void f_obr_user_thread(int sig) {
    printf("SIGPIPE in user thread!\n");
    close(sockfd);
    exit(EXIT_SUCCESS); 
}


void user_loop(){
    // enter message from console
    char exitflag = 0;
    while(!exitflag){
        printf("Enter operation(h for help)\n");
        // read first character
        char option = getchar();
        // skip other characters 
        while(getchar() != '\n');
        //printf("char: %c\n", option);
        switch(option){

            // display help
            case 'h':
            printf("n - Create new delivery\no - Check my delivery\ne - exit\n");
            break;

            case 'n':

            // TODO 
            // send
            // send(sockfd, &msg, sizeof(msg), 0);
            
            break;

            //request my delivery from server
            case 'o':

            // TODO 
            // send
            //send(sockfd, &msg, sizeof(msg), 0);
            
            break;

            // exit 
            case 'e':
            msg.msg_type = EXITING;
            strcpy(msg.username, username);
            send(sockfd, &msg, sizeof(msg), 0);
            exitflag = 1;
            break;

            default:
        }
    }
}


void worker_loop(){
// enter message from console
    char exitflag = 0;
    while(!exitflag){
        printf("Enter operation(h for help)\n");
        // read first character
        char option = getchar();
        // skip other characters 
        while(getchar() != '\n');
        //printf("char: %c\n", option);
        switch(option){

            // display help
            case 'h':
            printf("s - Show stock\nc - Change delivery status\ne - exit\n");
            break;

            // request my stock info from server
            // TODO 
            case 's':

            // send
            //send(sockfd, &msg, sizeof(msg), 0);
            
            break;

            // TODO 
            case 'c':

            // send
            //send(sockfd, &msg, sizeof(msg), 0);
            
            break;

            // exit 
            case 'e':
            msg.msg_type = EXITING;
            strcpy(msg.username, username);
            send(sockfd, &msg, sizeof(msg), 0);
            exitflag = 1;
            break;

            default:
        }
    }
}


int main(void){

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error in socket opening\n");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error when coonection!\n");
        return -1;
    } else {
        printf("Connection done!\n");
    }

    // authorize
    recv(sockfd, &msg, sizeof(msg), 0);
    if (msg.msg_type == MSG_AUTH) {
        // enter username and password
        printf("Enter username or login: ");
        fgets(msg.username, USERNAME_LEN, stdin);
        strcpy(username, msg.username);
        printf("You entered: %s\n", msg.username);
        printf("Enter password: ");
        fgets(msg.text, MSG_BUFF_SIZE, stdin);
        printf("You entered: %s\n", msg.text);
        
        msg.msg_type = MSG_AUTH;
        send(sockfd, &msg, sizeof(msg), 0);
    } else {
        printf("Error!\n");
        exit(EXIT_FAILURE);
    }
    recv(sockfd, &msg, sizeof(msg), 0);

    //printf("%s\n", msg.text);

    switch (msg.msg_type)
    {
        case REGISTRATION:
            printf("%s", msg.text);
            char ynflag = 0;
            char ynchar;
            while(!ynflag){
                // read first character
                ynchar = getchar();
                // skip other characters 
                while(getchar() != '\n');

                if (ynchar == 'n'){
                    strcpy(msg.text, "n\0");
                    ynflag = 1;
                }
                else if (ynchar == 'y'){
                    strcpy(msg.text, "y\0");
                    ynflag = 1;
                }
                else{
                    printf("Error. Enter once again[y/n]\n");
                }
            }
            msg.msg_type = REGISTRATION;
            strcpy(msg.username, username);
            // sending y or n
            send(sockfd, &msg, sizeof(msg), 0);
            if (ynchar == 'n'){
                close(sockfd);
                exit(-1);
            }
            recv(sockfd, &msg, sizeof(msg), 0);
            //TODO i_am_worker_flg = msg.text[0] 
            printf("%s", msg.text);
            send(sockfd, &msg, sizeof(msg), 0);
        break;

        case FORBIDDEN:
            printf("%s", msg.text);
            exit(-1);
        break;

        // ! REPLACE
        case MSG_TRANSFER_MSG:
            printf("%s", msg.text);
            send(sockfd, &msg, sizeof(msg), 0);
        break;


    default:
        break;
    }

    //TODO set i_am_worker_flag

    if (i_am_worker_flg){
        worker_loop();
    }
    else{
        user_loop();
    }
    

    close(sockfd);

    return 0;
}