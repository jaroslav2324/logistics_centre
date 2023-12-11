
#include "util.h"
#include <unistd.h>

// set console text colour
#define TEXT_RED printf("\033[91m");
#define TEXT_YELLOW printf("\033[93m");
#define TEXT_DEFAULT printf("\033[39m");


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
            {   
                strcpy(msg.username, username);
                // enter username of receiver
                printf("Enter username of receiver:\n");
                fgets(msg.order.username_of_receiver, USERNAME_LEN, stdin);
                // enter other order data
                printf("Enter delivery destination:\n");
                fgets(msg.order.destination, 64, stdin);
                printf("Enter current delivery position:\n");
                fgets(msg.order.position, 64, stdin);
                printf("Enter delivery content:\n");
                fgets(msg.order.content, 64, stdin);
                // send
                msg.msg_type = CREATE_ORDER;
                send(sockfd, &msg, sizeof msg, 0);

                system("clear");
                // wait server to handle data
                recv(sockfd, &msg, sizeof msg, 0);
                if (msg.msg_type == OK){
                    TEXT_YELLOW
                    printf("Delivery created\n");
                    TEXT_DEFAULT
                }
                else {
                    TEXT_RED
                    printf("Error in delivery creating\n");
                    TEXT_DEFAULT
                }
            }
            break;

            //request my delivery from server
            case 'o':
            
            msg.msg_type = GET_ORDERS_STATUS;
            send(sockfd, &msg, sizeof msg, 0);

            // wait for response
            recv(sockfd, &msg, sizeof msg, 0);
            int amount_of_orders;

            amount_of_orders = atoi(msg.text);

            for (int cnt = 0; cnt < amount_of_orders; cnt++){
                recv(sockfd, &msg, sizeof msg, 0);
                printf("Order %2i:", cnt + 1);
                
                printf("Status - %i\n destination - %s current position - %s content - %s", msg.order.status, msg.order.destination,
                 msg.order.position, msg.order.content);
            }

            break;

            // exit 
            case 'e':
            msg.msg_type = EXITING;
            strcpy(msg.username, username);
            send(sockfd, &msg, sizeof(msg), 0);
            exitflag = 1;
            close(sockfd);
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
            printf("s - Show stock(incoming and already in warehouse)\nc - Change delivery status\ne - exit\n");
            break;

            // request my stock info from server 
            case 's':

            // TODO send request to get all incoming delivery and delivery in warehouse
            // TODO g
            msg.msg_type = GET_ORDERS_WAREHOUSE;
            send(sockfd, &msg, sizeof msg, 0);

            recv(sockfd, &msg, sizeof msg, 0);
            int amount_of_orders = atoi(msg.text);

            for (int cnt = 0; cnt < amount_of_orders; cnt++){
                recv(sockfd, &msg, sizeof msg, 0);
                printf("Order %2i: ", cnt + 1);
                // TODO show order
                //printf(order);
            }
            
            break;

            // TODO 
            case 'c':

            // requese

            // TODO
            // ! TODO  

            // enter index of order 
            int idx; 
            char entered_good_idx = 0;

            while(!entered_good_idx){
                // TODO enter order index
                // TODO check number
                // skip other characters 
                while(getchar() != '\n');
            }
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
        fgets(msg.text, PASSWORD_LEN, stdin);
        printf("You entered: %s\n", msg.text);
        msg.msg_type = MSG_AUTH;
        send(sockfd, &msg, sizeof(msg), 0);
    } else {
        printf("Error!\n");
        exit(EXIT_FAILURE);
    }


    recv(sockfd, &msg, sizeof(msg), 0);

    switch (msg.msg_type)
    {
        case REGISTRATION:
            printf("%s", msg.text);
            char ynflag = 0;
            char ynchar;   

            // enter y/n
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
            // read type of user
            // ! do not read when n entered
            printf("Enter your type: 1 - worker; 0 - comsumer\n");
            char strbuf[4];
            fgets(strbuf, sizeof(strbuf), stdin);
            msg.user_type = strbuf[0] - '0';
            user_type worker_code_temp = msg.user_type;
            printf("You entered: %i\n", msg.user_type);

            // sending y or n
            send(sockfd, &msg, sizeof(msg), 0);

            if (ynchar == 'n'){
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            // if i am worker i receive all warehouses to choose on whic one i am working
            if (worker_code_temp == WORKER){
                i_am_worker_flg = 1;

                // read amount of warehouses
                recv(sockfd, &msg, sizeof msg, 0);
                int amount_of_warehouses;

                amount_of_warehouses = atoi(msg.text);

                // read every warehouse and print
                for (int cnt = 0; cnt < amount_of_warehouses; cnt++){
                    recv(sockfd, &msg, sizeof msg, 0);
                    printf("Warehouse %2i: ", cnt + 1);
                    printf("%s", msg.text);
                }
                
                // TODO choose warehouse
                int idx;

                // send idex of warehouse (from 1)
                sprintf(msg.text, "%i\n", idx);
                send(sockfd, &msg, sizeof(msg), 0);

            }
            else{
                i_am_worker_flg = 0;
            }

            // receive success registration message
            recv(sockfd, &msg, sizeof(msg), 0);
            printf("%s", msg.text);
                
        break;

        case FORBIDDEN:
            printf("%s", msg.text);
            close(sockfd);
            exit(EXIT_FAILURE);
        break;

        case AUTH_SUCCESS:
            i_am_worker_flg = msg.user_type;
            printf("%s", msg.text);
        break;

        default:
        break;
    }


    if (i_am_worker_flg){
        worker_loop();
    }
    else{
        user_loop();
    }
    

    close(sockfd);

    return 0;
}