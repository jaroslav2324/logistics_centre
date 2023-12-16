
#include "util.h"
#include <stdio.h>

// clear screen
#define CLR_SCRN system("clear");

// set console text colour
#define SET_TEXT_RED     printf("\033[91m");
#define SET_TEXT_YELLOW  printf("\033[93m");
#define SET_TEXT_MAGENTA  printf("\033[95m");
#define SET_TEXT_DEFAULT printf("\033[39m");

// not for formatting output, only for text
void printf_yellow(const char * str){
    SET_TEXT_YELLOW
    printf("%s", str);
    SET_TEXT_DEFAULT
}
// not for formatting output, only for text
void printf_red(const char * str){
    SET_TEXT_RED
    printf("%s", str);
    SET_TEXT_DEFAULT
}
// not for formatting output, only for text
void printf_magenta(const char * str){
    SET_TEXT_MAGENTA
    printf("%s", str);
    SET_TEXT_DEFAULT
}

void print_status(order_status status){
    switch (status)
    {
    case CREATED:
        printf("CREATED");
        break;
    case MOVING:
        printf("MOVING");
        break;    
    case READY_TO_TAKE:
        printf("READY TO TAKE");
        break;    
    case DELIVERED:
        printf("DELIVERED");
        break;
    default:
        printf_red("NO STATUS");
        break;
    }
}

void print_order(order* order){
<<<<<<< HEAD
    // printf("Index of order: %d\n", order->index);
    printf("Status - %i\n sender - %s receiver - %s destination - %s current position - %s content - %s\n", 
    order->status, order->username_of_sender, order->username_of_receiver, order->destination, order->position, order->content);
=======
    printf("Status - ");
    print_status(order->status);
    printf("\n destination - %s current position - %s content - %s\n", order->destination,
                 order->position, order->content);
>>>>>>> eec886d67f6154794b9a8ed6203f675cde9a8e01
}


// return positive(>0) int: lower_bound <= num <= upper_bound
int read_positive_num(FILE* __restrict__ stream, int lower_bound, int upper_bound){

    int num = 0;
    char buff[32];
    char good_num_flg = 0;
    while (!good_num_flg) {
        fgets(buff, sizeof buff, stdin);
        num = atoi(buff);
        if(num <= 0 || num < lower_bound || num > upper_bound){
            printf_yellow("Index out of bounds. Enter once again.\n");
        }
        else{
            good_num_flg = 1;
        }
    }
    return num;
}



int sockfd;

int i_am_worker_flg = 0;

char username[USERNAME_LEN];
char my_warehouse_position[128];
message msg; // buffer


// copy username and type to msg buffer, send to sockfd
void msg_send(msg_types type){
    strcpy(msg.username, username);
    msg.msg_type = type;
    send(sockfd, &msg, sizeof msg, 0);
}

// read message from sockfd to msg buffer. more convenient than recv() with four parameters
void msg_recv(){
    if(recv(sockfd, &msg, sizeof msg, 0) < 0) 
        printf_red("ERROR");
}


// handler ctrl+c -> send EXITING to server
void sigint_handler(int sig){
    printf("Ctrl+C\n");
    msg_send(EXITING);
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
            {
                CLR_SCRN
                printf("n - Create new delivery\ns - Check delivery i sent\nr - Check delivery for me\ne - exit\n\n");
            }
            break;

            case 'n':
            {   
                CLR_SCRN
                // fill message buffer
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
                // copy my name to username of sender
                strcpy(msg.order.username_of_sender, username);

                // send
                msg_send(CREATE_ORDER);

                CLR_SCRN

                // wait server to handle data
                recv(sockfd, &msg, sizeof msg, 0);
                if (msg.msg_type == OK){
                    printf_yellow("Delivery created\n");
                }
                else {
                    printf_yellow("Error in delivery creation(wrong input)\n");
                }
            }
            break;

            //request my delivery as sender from server
            case 's':
            {
                msg_send(GET_ORDERS_STATUS_SENDER);

                // wait for amount of orders
                msg_recv();

                int amount_of_orders = atoi(msg.text);
                if (amount_of_orders == 0){
                    printf_yellow("Amount of orders 0\n");
                    break;
                }

                CLR_SCRN

                printf_magenta("You sent orders:\n\n");

                for (int cnt = 0; cnt < amount_of_orders; cnt++){
                    // msg_recv();
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);

                    char strbuf[64];
                    sprintf(strbuf, "Order %2i to %s", cnt + 1, msg.order.username_of_receiver);

                    printf_yellow(strbuf);
                    print_order(&msg.order);
                }
            }
            break;

            //request my delivery as receiver from server
            case 'r':
            {
                msg_send(GET_ORDERS_STATUS_RECEIVER);

                // wait for amount of orders
                msg_recv();

                int amount_of_orders = atoi(msg.text);
                if (amount_of_orders == 0){
                    printf_yellow("Amount of orders 0\n");
                    break;
                }

                CLR_SCRN

                printf_magenta("Orders for you:\n\n");

                for (int cnt = 0; cnt < amount_of_orders; cnt++){
                    // msg_recv();
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);

                    char strbuf[64];
                    sprintf(strbuf, "Order %2i from %s", cnt + 1, msg.order.username_of_sender);

                    printf_yellow(strbuf);
                    print_order(&msg.order);
                }
            }
            break;

            // exit 
            case 'e':
            msg_send(EXITING);
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

        switch(option){

            // display help
            case 'h':
            printf("s - Show deliveryin warehouse\na - Show awaiting delivery\nc - Change delivery status\ne - exit\n");
            break;

            // request my stock info from server 
            case 's':
            {
                // send request to get all delivery in warehouse
                msg_send(GET_ORDERS_WAREHOUSE);

                // receive amount of orders
                msg_recv();
                int amount_of_orders = atoi(msg.text);
                if (amount_of_orders == 0){
                    printf_yellow("Amount of orders 0\n");
                    break;
                }

                // TODO printf_magenta

                // receive orders
                for (int cnt = 0; cnt < amount_of_orders; cnt++){
                    // msg_recv();
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);

                    // show order
                    char strbuf[30];
                    sprintf(strbuf, "Order %2i:\n", cnt + 1);
                    printf_yellow(strbuf);
                    print_order(&msg.order);
                }
            }
            break;

            // request awaiting delivery
            case 'a':
            {
                // send request to get all incoming delivery 
                msg_send(GET_ORDERS_AWAITING);

                // receive amount of orders
                msg_recv();
                int amount_of_orders = atoi(msg.text);
                if (amount_of_orders == 0){
                    printf_yellow("Amount of orders 0\n");
                    break;
                }

                // TODO printf_magenta

                // receive orders
                for (int cnt = 0; cnt < amount_of_orders; cnt++){
                    // msg_recv();
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);

                    // show order
                    char strbuf[30];
                    sprintf(strbuf, "Order %2i:\n", cnt + 1);
                    printf_yellow(strbuf);
                    print_order(&msg.order);
                }
            }
            break;

            case 'c':
            {
                // request all orders
                // send request to get all delivery in warehouse and incoming
                msg_send(CHANGE_ORDER_STATUS);

                // receive amount of orders
                msg_recv();
                int amount_of_orders = atoi(msg.text);
                if (amount_of_orders == 0){
                    printf_yellow("Amount of orders 0\n");
                    break;
                }

                // array of orers
                order* orders = (order*)malloc(sizeof(order) * amount_of_orders);
                // receive orders
                for (int cnt = 0; cnt < amount_of_orders; cnt++){
                    //msg_recv();
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);

                    // show order
                    char strbuf[30];
                    sprintf(strbuf, "Order %2i:\n", cnt + 1);
                    printf_yellow(strbuf);
                    print_order(&msg.order);
                    // save order
                    orders[cnt] = msg.order;
                }

                // enter index of order 
                printf_yellow("Enter number of order:\n");
                int idx = read_positive_num(stdin, 0, amount_of_orders);
                
                printf("%i - CREATED\n%i - MOVING\n%i - READY TO TAKE\n%i - DELIVERED\nEnter new status:\n", 
                (int)CREATED + 1, (int)MOVING + 1, (int)READY_TO_TAKE + 1, (int)DELIVERED + 1);
                // enter new status
                int status = read_positive_num(stdin, 0, 4);

                // send index of order and new status
                msg.order.index = orders[idx - 1].index;
                msg.order.status = status - 1;
                msg_send(CHANGE_ORDER_STATUS);

                msg_recv();
                if (msg.msg_type == OK){
                    printf_yellow("Message updated\n");
                }
                else{
                    printf_red("Error in message updating\n");
                }

                free(orders);
            }
            break;

            // transfer to another warehouse
            case 't':
            {
                //! DO NOT USE NOW
                break;
                // request all orders
                // send request to get all delivery in warehouse and incoming
                msg_send(TRANSFER_DELIVERY);

                // receive amount of orders
                msg_recv();
                int amount_of_orders = atoi(msg.text);
                if (amount_of_orders == 0){
                    printf_yellow("Amount of orders 0\n");
                    break;
                }

                // array of orers
                order* orders = (order*)malloc(sizeof(order) * amount_of_orders);
                // receive orders
                for (int cnt = 0; cnt < amount_of_orders; cnt++){
                    //msg_recv();
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);

                    // show order
                    char strbuf[30];
                    sprintf(strbuf, "Order %2i:\n", cnt + 1);
                    printf_yellow(strbuf);
                    print_order(&msg.order);
                    // save order
                    orders[cnt] = msg.order;
                }

                // enter index of order 
                printf_yellow("Enter number of order:\n");
                int idx = read_positive_num(stdin, 0, amount_of_orders);


                // receive warehouses to choose
                // read amount of warehouses
                msg_recv();
                int amount_of_warehouses;

                amount_of_warehouses = atoi(msg.text);
                char ** warehouses = (char**)malloc(sizeof (char*) * amount_of_warehouses);
                for (int i = 0; i < amount_of_warehouses; i++){
                    warehouses[i] = (char *)malloc(MSG_BUFF_SIZE);
                }

                if (amount_of_warehouses <= 0){
                    printf_red("NO WAREHOUSES RECEIVED\n");
                }

                printf("Enter index of next warehouse:\n");
                // read every warehouse and print
                for (int cnt = 0; cnt < amount_of_warehouses; cnt++){
                    // waittall to wait all tcp segments
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);
                    printf("Warehouse %2i: %s", cnt + 1, msg.text);
                    strcpy(warehouses[cnt], msg.text);
                }
                printf("\n");
                

                printf("Enter index: ");
                int idx_war = read_positive_num(stdin, 0, amount_of_warehouses);


                // replace position and status
                order ord = orders[idx];
                strcpy(ord.position, warehouses[idx_war]);

                //new status
                int status = MOVING;

                // send index of order and new status
                ord.index = orders[idx - 1].index;
                ord.status = status - 1;
                msg.order = ord;
                msg_send(TRANSFER_DELIVERY);

                msg_recv();
                if (msg.msg_type == OK){
                    printf_yellow("Message updated\n");
                }
                else{
                    printf_red("Error in message updating\n");
                }

                for (int i = 0; i < amount_of_warehouses; i++){
                    free(warehouses[i]);
                }
                free(warehouses);
                free(orders);
            }

            // exit 
            case 'e':
            msg_send(EXITING);
            exitflag = 1;
            break;

            default:
        }
    }
}


int main(void){

    signal(SIGINT, sigint_handler);

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
        printf("Error with connection!\n");
        return -1;
    } else {
        printf("Connection done!\n");
    }

    // authorize
    msg_recv();
    if (msg.msg_type == MSG_AUTH) {
        // enter username and password
        printf("Enter username or login: ");
        fgets(username, USERNAME_LEN, stdin);
        printf("You entered: %s\n", username);
        printf("Enter password: ");
        fgets(msg.text, PASSWORD_LEN, stdin);
        printf("You entered: %s\n", msg.text);

        msg_send(MSG_AUTH);
    } else {
        printf("Error!\n");
        exit(EXIT_FAILURE);
    }

    // wait for server responce
    msg_recv();

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

            if (ynchar == 'n'){
                // sending n and exitind
                msg_send(REGISTRATION);
                close(sockfd);
                CLR_SCRN
                exit(EXIT_SUCCESS);
            }

            // read type of user
            printf("Enter your type: 1 - worker; 0 - comsumer\n");
            char strbuf[4];
            fgets(strbuf, sizeof(strbuf), stdin);
            msg.user_type = strbuf[0] - '0';
            user_type worker_code_temp = msg.user_type;
            printf("You entered: %i\n", msg.user_type);

            // sending y
            msg_send(REGISTRATION);

            // if i am worker i receive all warehouses to choose on which one i am working
            if (worker_code_temp == WORKER){
                i_am_worker_flg = 1;

                // read amount of warehouses
                msg_recv();
                int amount_of_warehouses;

                amount_of_warehouses = atoi(msg.text);

                if (amount_of_warehouses <= 0){
                    printf_red("NO WAREHOUSES RECEIVED\n");
                }

                printf("Enter index of warehouse:\n");
                // read every warehouse and print
                for (int cnt = 0; cnt < amount_of_warehouses; cnt++){
                    // waittall to wait all tcp segments
                    recv(sockfd, &msg, sizeof msg, MSG_WAITALL);
                    printf("Warehouse %2i: %s", cnt + 1, msg.text);
                }
                printf("\n");
                

                printf("Enter index: ");
                int idx = read_positive_num(stdin, 0, amount_of_warehouses);
                
                // send index of warehouse (indexation from 1)
                sprintf(msg.text, "%i\n", idx);
                msg_send(REGISTRATION);
            }
            else{
                i_am_worker_flg = 0;
            }
            CLR_SCRN
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
            CLR_SCRN
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
    
    CLR_SCRN
    close(sockfd);

    return 0;
}