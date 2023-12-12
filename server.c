#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *user_thread(void *data);
void requests_from_worker(int sock2, int index_of_warehouse);
void requests_from_consumer(int sock2);
user_record check_users_credentials(char* login, char* password);
int write_order_to_file(order order);
int write_user_to_file(user_record user, int id_of_warehouse);

int check_user_name(char* username);
int get_count_of_warehouses();
char** get_names_of_warehouses();
char* get_warehouse_by_id(int id_of_warehouse);
int get_count_of_orders(char* value, find_order_type parameter);
order* get_orders(char* value, find_order_type parameter);
int find_last_index_of_order();

// DONE: replace order status to new by index
// orders.txt look like
// 3 - index of order
// gggg - username_of_sender
// asd - username_of_receiver
// 0 - order_status
// Sklad123 - destination
// Sklad33 - position
// gleb - content
// Need to find order by index and then erase old status to new_status
int update_order(int order_index, order_status new_status);

// TODO: need to check warehouse name by reading warehouses.txt
// if warehouse name exist return 1
// else return 0
int check_warehouse_name(char* name);

// TODO: edit check_user_name
// need to check if username exist and user is not worker
// usersdb.txt
// zxc - username
// 123 - password
// 1 1 - first number: 0 - consumer or 1 - worker; second number: index of warehouse if user worker

pthread_mutex_t mutex;

int main(int argc, char* argv[]) {
    int codeErr = 0;
    struct sockaddr_in server_addr, client_addr;
     pthread_mutex_init(&mutex, NULL);

    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 == -1) {
        printf("Error when socket was in process of creating!\n");
        exit(EXIT_FAILURE);
    }

    int true = 1;
    codeErr = setsockopt(sock1,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
    if (codeErr == -1) {
        perror("setsockopt\n");
        exit(1);
    }
    
    memset((char *)&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server_addr.sin_port = htons(SERVER_PORT);
    codeErr = bind(sock1, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (codeErr == -1) {
        printf("Error when bind function was called!\n");
        exit(1);
    }

    printf("Server is running on addr: %s\n", SERVER_ADDR);

    pthread_t thread;
    while (1) {
        codeErr = listen(sock1, 3);
        if (codeErr == -1) {
            printf("Error when listening function was activated!\n");
            exit(EXIT_FAILURE);
        }
        // if (cnt_of_threads == USER_NUMBER + 1) {
        //     printf("Limit of users!\n");
        //     continue;
        // }

        socklen_t ans_len = sizeof(client_addr);  
        int sock2 = accept(sock1, (struct sockaddr*)&client_addr, &ans_len);
        if (sock2 == -1) {
            printf("Error when accept connection with sock2!\n");
            exit(EXIT_FAILURE);
        } else {
            printf("\nConnection done!\n");
        }

        pthread_create(&thread, NULL, user_thread, (void*)&sock2);
        // pthread_mutex_lock(&mutex);
        // cnt_of_threads++;
        // pthread_mutex_unlock(&mutex);
    }

    return 0;
}

void *user_thread(void *param) {
    int* sock2_addr = (int*) param;
    int sock2 = *sock2_addr;

    message auth_message;
    auth_message.msg_type = MSG_AUTH;
    send(sock2, &auth_message, sizeof(auth_message), 0);

    message message;
    recv(sock2, &message, sizeof(message), 0);

    printf("\nUsername: %s", message.username);
    printf("Password: %s", message.text);
    printf("User socket fd: %d\n", sock2);
    
    user_record user = check_users_credentials(message.username, message.text);

    if (STREQU(user.user_name, message.username)) {
        // AUTHORIZATION
        if (STREQU(user.password, message.text)) {
            strcpy(message.text, "Successfully authorization!\n");
            message.username[strlen(message.username) - 1] = '\0';
            printf("Successfully authorization of %s!\n", message.username);
            message.msg_type = AUTH_SUCCESS;
            message.user_type = user.type; // 
            send(sock2, &message, sizeof(message), 0);

        } else {
            strcpy(message.text, "Wrong password entered!\n");
            message.username[strlen(message.username) - 1] = '\0';
            printf("User %s entered a wrong password! Bye-bye..\n", message.username);
            message.msg_type = FORBIDDEN;
            send(sock2, &message, sizeof(message), 0);
            shutdown(sock2, SHUT_RDWR);
            close(sock2);
            pthread_exit(NULL);
        }

    } else {
        // REGISTRATION
        strcpy(user.user_name, message.username);
        strcpy(user.password, message.text);

        strcpy(message.text, "You're not registered! Do you want to register?(y/n)\n");
        message.msg_type = REGISTRATION;
        send(sock2, &message, sizeof(message), 0); 

        // wating answer from client
        recv(sock2, &message, sizeof(message), 0);
        user.type = message.user_type;
        
        if (STREQU(message.text, "y")) {
            switch (user.type) {
            case WORKER: {
                int count_of_warehouses = get_count_of_warehouses();
                char** warehouses = get_names_of_warehouses();
                sprintf(message.text, "%d\n", count_of_warehouses);
                send(sock2, &message, sizeof(message), 0);

                for (int i = 0; i < count_of_warehouses; ++i) {
                    strcpy(message.text, warehouses[i]);
                    send(sock2, &message, sizeof(message), 0);
                }

                for (int i = 0; i < count_of_warehouses; ++i) {
                    free(warehouses[i]);
                }
                free(warehouses);

                recv(sock2, &message, sizeof(message), 0);
                int id_of_warehouse = atoi(message.text);
                char* warehouse = get_warehouse_by_id(id_of_warehouse);
                printf("Worker choose %s", warehouse);
                free(warehouse);
                write_user_to_file(user, id_of_warehouse);
                user.index_of_warehouse = id_of_warehouse;
                break;

            } case CONSUMER: {
                write_user_to_file(user, 0);
                break;
                
            } default: {
                strcpy(message.text, "Wrong type of user entered!\n");
                message.msg_type = FORBIDDEN;
                send(sock2, &message, sizeof(message), 0);
                shutdown(sock2, SHUT_RDWR);
                close(sock2);
                pthread_exit(NULL);
            }
            }

            printf("Success registration!\n");
            strcpy(message.text, "Success registration!\n");
            send(sock2, &message, sizeof(message), 0);
            
        } else {
            message.username[strlen(message.username) - 1] = '\0';
            printf("User %s doesn't want register on server!\n", message.username);
            shutdown(sock2, SHUT_RDWR);
            close(sock2);
            pthread_exit(NULL);
        }
    }

    switch (user.type) {
    case WORKER: {
        requests_from_worker(sock2, user.index_of_warehouse);
        break;

    } case CONSUMER: {
        requests_from_consumer(sock2);
        break;

    } default: {
        printf("Server doesn't support this type of user!\n");
    }
    }

    close(sock2);
    pthread_exit(NULL);
}

void requests_from_worker(int sock2, int index_of_warehouse) {
    message message;
    while (1) {
        recv(sock2, &message, sizeof(message), 0);

        if (message.msg_type == GET_ORDERS_WAREHOUSE) {
            char* user_warehouse = get_warehouse_by_id(index_of_warehouse);
            printf("User warehouse: %s", user_warehouse);
            int count_of_orders_in_warehouse = get_count_of_orders(user_warehouse, POSITION);
            printf("User count of orders: %d\n", count_of_orders_in_warehouse);
            sprintf(message.text, "%d\n", count_of_orders_in_warehouse);
            send(sock2, &message, sizeof(message), 0);

            order* orders = get_orders(user_warehouse, POSITION);
            for (int i = 0; i < count_of_orders_in_warehouse; ++i) {
                message.order = orders[i];
                send(sock2, &message, sizeof(message), 0);
            }

            free(user_warehouse);
            free(orders);
            printf("Server send orders from warehouse!\n");
    
        }  else if (message.msg_type == GET_ORDERS_AWAITING) {
            char* user_warehouse = get_warehouse_by_id(index_of_warehouse);
            printf("User warehouse: %s", user_warehouse);
            int count_of_orders= get_count_of_orders(user_warehouse, DESTINATION);
            printf("User count of orders: %d\n", count_of_orders);
            sprintf(message.text, "%d\n", count_of_orders);
            send(sock2, &message, sizeof(message), 0);

            order* orders = get_orders(user_warehouse, DESTINATION);
            for (int i = 0; i < count_of_orders; ++i) {
                message.order = orders[i];
                send(sock2, &message, sizeof(message), 0);
            }

            free(user_warehouse);
            free(orders);
            printf("Server send orders awaiting!\n");
    
        } else if (message.msg_type == CHANGE_ORDER_STATUS) {
            //TODO: impl update order func and mb need to use index of order 
            char* user_warehouse = get_warehouse_by_id(index_of_warehouse);
            printf("User warehouse: %s", user_warehouse);
            int count_of_orders = get_count_of_orders(user_warehouse, DEST_AND_POS);
            printf("User count of orders: %d\n", count_of_orders);
            sprintf(message.text, "%d\n", count_of_orders);
            send(sock2, &message, sizeof(message), 0);

            order* orders = get_orders(user_warehouse, DEST_AND_POS);
            for (int i = 0; i < count_of_orders; ++i) {
                message.order = orders[i];
                send(sock2, &message, sizeof(message), 0);
            }

            free(user_warehouse);
            free(orders);
            printf("Server sends dest and pos orders!\n");

            // wait for index and status of order
            recv(sock2, &message, sizeof(message), 0);
            order changed_order = message.order;
            int last_index_of_order = find_last_index_of_order();
            if (changed_order.index > last_index_of_order || changed_order.index < 1) {
                printf("User entered a wrong index of order to change!\n");
                message.msg_type = BAD;
                send(sock2, &message, sizeof(message), 0);
                continue;
            }
            update_order(changed_order.index, changed_order.status);
            printf("Order's status was changed!\n");
            message.msg_type = OK;
            send(sock2, &message, sizeof(message), 0);

        } else if (message.msg_type == EXITING) {
            message.username[strlen(message.username) - 1] = '\0';
            printf("Client %s exiting!\n", message.username);
            break;
        }
    }
}

void requests_from_consumer(int sock2) {
    message message;
    while (1) {
        // printf("Before recv int comsumer!\n");
        recv(sock2, &message, sizeof(message), 0);

        if (message.msg_type == CREATE_ORDER) {
            //TODO: from yarik: do not let user to create order for himself
            printf("Username of recevier: %s", message.order.username_of_receiver);
            int code = check_user_name(message.order.username_of_receiver);
            if (code) {
                message.order.status = CREATED;
                code = write_order_to_file(message.order);

                message.username[strlen(message.username) - 1] = '\0';
                printf("User %s create order!\n", message.username);
                printf("Username of sender: %s", message.order.username_of_sender);
                printf("Username of receiver: %s", message.order.username_of_receiver);
                printf("Destination: %s", message.order.destination);
                printf("Position: %s", message.order.position);
                printf("Content about item: %s\n", message.order.content);

                message.msg_type = OK;
                send(sock2, &message, sizeof(message), 0);
            } else {
                message.msg_type = BAD;
                send(sock2, &message, sizeof(message), 0);
                printf("User entered wr0ng name of receiver!\n");
            }

        } else if (message.msg_type == GET_ORDERS_STATUS_SENDER) {
            int count_of_orders = get_count_of_orders(message.username, SENDER);
            order* orders = get_orders(message.username, SENDER);

            sprintf(message.text, "%d\n", count_of_orders);
            send(sock2, &message, sizeof(message), 0);

            for (int i = 0; i < count_of_orders; ++i) {
                message.order = orders[i];
                send(sock2, &message, sizeof(message), 0);
            }
            free(orders);

        } else if (message.msg_type == GET_ORDERS_STATUS_RECEIVER) {
            int count_of_orders = get_count_of_orders(message.username, RECEIVER);
            order* orders = get_orders(message.username, RECEIVER);

            sprintf(message.text, "%d\n", count_of_orders);
            send(sock2, &message, sizeof(message), 0);

            for (int i = 0; i < count_of_orders; ++i) {
                message.order = orders[i];
                send(sock2, &message, sizeof(message), 0);
            }
            free(orders);

        }  else if (message.msg_type == EXITING) {
            message.username[strlen(message.username) - 1] = '\0';
            printf("Client %s exiting!\n", message.username);
            break;
        }
    }
}

// return user_record if user was found in db
// return user_record with password contains '\0' chars if password is incorrect
// return user_record with username and password contain '\0' chars if user doesnt exist
user_record check_users_credentials(char* login, char* password) {
    FILE* file;
    file = fopen(USERSDB_FILE, "r");
    user_record user;
    
    while ((fgets(user.user_name, USERNAME_LEN, file)) != NULL) {
        fgets(user.password, PASSWORD_LEN, file);

        fscanf(file, "%d", (int*)&user.type);
        if (user.type == WORKER) {
            fscanf(file, "%d", &user.index_of_warehouse);
        } else if (user.type == CONSUMER) {
            user.index_of_warehouse = 0;
        }
        fgetc(file); // reading \n in the end of line

        if (STREQU(login, user.user_name)) {
            if (STREQU(password, user.password)) {
                fclose(file);
                return user;
            } else {
                memset(user.password, '\0', sizeof(user.password));
                fclose(file);
                return user; 
            }
        } 
    }
    fclose(file);
    memset(user.user_name, '\0', sizeof(user.user_name));
    memset(user.password, '\0', sizeof(user.password));

    return user;
}

int write_user_to_file(user_record user, int id_of_warehouse) {
    FILE* file;
    file = fopen(USERSDB_FILE, "a");
    if (user.type == CONSUMER) {
        fprintf(file, "%s%s%d\n", user.user_name, user.password, user.type);
    } else {
        fprintf(file, "%s%s%d %d\n", user.user_name, user.password, user.type, id_of_warehouse);
    }
    fclose(file);
    return 1;
}

int write_order_to_file(order order) {
    FILE* file;
    int last_index_of_file = find_last_index_of_order() + 1;
    file = fopen(ORDERS_FILE, "a");
    fprintf(file, "%d\n%s%s%d\n%s%s%s", last_index_of_file, order.username_of_sender,
        order.username_of_receiver, order.status, order.destination, order.position, order.content);
    fclose(file);
    return 1;
}

// TODO: check username and this user doesnt worker
int check_user_name(char* username) {
    FILE* file;
    file = fopen(USERSDB_FILE, "r");
    char str[USERNAME_LEN];
    int i = 0;
    char username_flag = 0;
    while (fgets(str, USERNAME_LEN, file)) {
        if (i % 3 == 0)
        {
            if (STREQU(str, username))
            {
                username_flag = 1;
            }
            else username_flag = 0;
        }
        if (str[0] == '0' && (i % 3 == 2) && username_flag == 1)
            return 1;
        i++;
    }
    return 0;
}

int get_count_of_warehouses() {
    // Because warehouse.txt has one line for id and one line for name, 
    // to get count of warehouses I just divide number of lines by 2
    FILE* file;
    file = fopen(WAREHOUSES_FILE, "r");
    char str[STR_FILE_LEN];
    int i = 0;
    while (fgets(str, STR_FILE_LEN, file)) i++;
    return i / 2;
}

char** get_names_of_warehouses() {
    FILE* fp;
    if ((fp = fopen(WAREHOUSES_FILE, "r")) == NULL) {
        printf("The file could not be opened\n");
        return NULL;
    }
    int rows = get_count_of_warehouses();
    char** warehouse = (char**)malloc(rows * sizeof(char*));
    for (int i = 0; i < rows; i++) {
        warehouse[i] = (char*)malloc(STR_FILE_LEN);
    }

    char line[STR_FILE_LEN];
    int i = 0;
    while (fgets(line, STR_FILE_LEN, fp)) {
        fgets(line, STR_FILE_LEN, fp);
        strcpy(warehouse[i], line);
        i++;
    }
    return warehouse;
}

// parametr for searching in orders file can take: USERNAME, DESTINATION or POSTION 
// value - value of parametr for searching
int get_count_of_orders(char* value, find_order_type parameter) {
    // COMMENT THIS IF YOU WANT TO USE check_user_name OUT OF THIS FUNCTION
    // if (!check_user_name(username)) {
    //     printf("There\'s no user with username %s\n", username);
    //     return -1;
    // }

    FILE* file;
    file = fopen(ORDERS_FILE, "r");
    char str[STR_FILE_LEN];
    int i = 0, cnt = 0;
    while (fgets(str, STR_FILE_LEN, file)) {
        ++i;
        switch (parameter) {
        case SENDER: {
            if ((i == 2) && STREQU(str, value)) cnt++;
            break;
            
        } case RECEIVER: {
            if ((i == 3) && STREQU(str, value)) cnt++;
            break;

        } case DESTINATION: {
            if ((i == 5) && STREQU(str, value)) cnt++;
            break;

        } case POSITION: {
            if ((i == 6) && STREQU(str, value)) cnt++;
            break;

        } case DEST_AND_POS: {
            if (((i == 5) && STREQU(str, value)) || 
                ((i == 6) && STREQU(str, value))) cnt++;
            break;

        } default: {
            perror("Incorrect type in get_count_of_orders() for searching!\n");
            return -1;
        }
        }
        if (i == 7) i = 0;
    }
    return cnt;
}

// return NULL, it needs to be handled
char* get_warehouse_by_id(int id_of_warehouse) {
    // RESULT STRING HAS TO BE FREED AFTER USE!!!
    FILE* file;
    file = fopen(WAREHOUSES_FILE, "r");
    // Convert int id to char* id to use STREQU
    char id[4];
    sprintf(id, "%d\n", id_of_warehouse);

    char str[STR_FILE_LEN];
    int i = 0;
    while (fgets(str, STR_FILE_LEN, file)) {
        if (STREQU(str, id) && (i % 2 == 0)) {
            fgets(str, STR_FILE_LEN, file);
            char *str_to_return = malloc(strlen(str));
            strcpy(str_to_return, str);
            return str_to_return;
        } 
    }
    return NULL;
}

// return NULL, it needs to be handled
order* get_orders(char* value, find_order_type parameter) {
    FILE* fp;
    char line[6][STR_FILE_LEN];
    order_status status;
    if ((fp = fopen(ORDERS_FILE, "r")) == NULL) {
        printf("The file could not be opened\n");
        return NULL;
    }

    int i = 0, flag = 0;
    int index_of_order = 0;
    int count_of_orders = get_count_of_orders(value, parameter);
    if (count_of_orders == - 1) {
        perror("Error int get_orders!\n");
        return  NULL;
    }
    order* orders = (order*)malloc(sizeof(order) * count_of_orders);

           //index of order
    while (fgets(line[0], STR_FILE_LEN, fp) != NULL) {
        index_of_order = atoi(line[0]);

        // username of sender
        fgets(line[1], STR_FILE_LEN, fp);

        // username of receiver
        fgets(line[2], STR_FILE_LEN, fp);

        //status
        fscanf(fp, "%d", (int*)&status);
        fgetc(fp);

        //dest
        fgets(line[3], STR_FILE_LEN, fp);

        //postion
        fgets(line[4], STR_FILE_LEN, fp);

        //content
        fgets(line[5], STR_FILE_LEN, fp);

        switch (parameter) {
        case SENDER: {
            if (STREQU(line[1], value)) flag = 1;
            break;

        } case RECEIVER: {
            if (STREQU(line[2], value)) flag = 1;
            break;

            break;

        } case DESTINATION: {
            if (STREQU(line[3], value)) flag = 1;
            break;

        } case POSITION: {
            if (STREQU(line[4], value)) flag = 1;
            break;
        } case DEST_AND_POS: {
            if (STREQU(line[3], value) || STREQU(line[4], value)) flag = 1;
        }
        }

        if (flag) { 
            orders[i].index = index_of_order;
            strcpy(orders[i].username_of_sender, line[1]);
            strcpy(orders[i].username_of_receiver, line[2]);
            orders[i].status =  status;
            strcpy(orders[i].destination, line[3]);
            strcpy(orders[i].position, line[4]);
            strcpy(orders[i].content, line[5]);
            i++;
            flag = 0;
        }         
    }

    fclose(fp);
    return orders;
}

int find_last_index_of_order() {
    FILE* file;
    file = fopen(ORDERS_FILE, "r");
    char str[STR_FILE_LEN];

    int cnt = 0, res = 0, flag = 0;
    while (fgets(str, STR_FILE_LEN, file) != NULL) {
        cnt++;
        if (cnt == 1) {
            res = atoi(str);
            flag = 1;
        }
        if (cnt == 7) cnt = 0;
    }
    if (flag == 1) return res;
    else return 0;
}

int check_warehouse_name(char* name)
{
    FILE* file;
    file = fopen(WAREHOUSES_FILE, "r");
    char str[STR_FILE_LEN];
    int i = 0;
    while (fgets(str, STR_FILE_LEN, file))
    {
        if ((i % 2 == 0) && STREQU(str, name))
            return 1;
        i++;
    }
    return 0;
}

int update_order(int order_index, order_status new_status) {
    //return 0 if file dont open, return 1 if ok
    FILE* fp;
    if ((fp = fopen(ORDERS_FILE, "r+")) == NULL)
    {
        printf("The file could not be opened\n");
        return 0;
    }
    int line[STR_FILE_LEN];
    int lineNumber = 0;
    while (fgets(line, sizeof(line), fp)) {
        lineNumber++;
        if (atoi(line) == order_index) {
            fgets(line, STR_FILE_LEN, fp);
            lineNumber += 3;
            fseek(fp, strlen(line), SEEK_CUR);
            fprintf(fp, "%d\n", new_status);
            return 1;
        }
    }
}