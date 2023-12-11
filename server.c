#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *user_thread(void *data);
user_record check_users_credentials(char* login, char* password);
int write_order_to_file(order order);
int write_user_to_file(user_record user, int id_of_warehouse);

int check_user_name(char* username);
order* get_orders_for_user(char* username);
int get_count_of_orders_by_username(char* username);
int get_count_of_warehouses();
char** get_names_of_warehouses();
char* get_warehouse_by_id(int id_of_warehouse);

int get_count_of_orders_by_warehouse(char* warehouse);

//TODO: need to allocate memory for orders like in get_orders_for_user()
// and then find all orders with user_warehouse and return these
// orders.txt
// aboba - username_of_receiver
// 0 - status
// sklad2 - destination
// sklad - user_warehouse
// krytaia posilka - content
order* get_orders_for_warehouse(char* user_warehouse);

// replace order status to new by index
// POKA NE NADO
int update_order(int order_idx, order* order);

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
        strcpy(user.user_name, message.username);
        strcpy(user.password, message.text);

        strcpy(message.text, "You're not registered! Do you want to register?(Y/N)\n");
        message.msg_type = REGISTRATION;
        send(sock2, &message, sizeof(message), 0); 

        // wating answer from client
        recv(sock2, &message, sizeof(message), 0);
        user.type = message.user_type;
        
        if (STREQU(message.text, "y")) {
            if (user.type == WORKER) {
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
            } else {
                write_user_to_file(user, 0);
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

    if (user.type == WORKER) {
        while (1) {
            recv(sock2, &message, sizeof(message), 0);

            if (message.msg_type == GET_ORDERS_WAREHOUSE) {
                char* user_warehouse = get_warehouse_by_id(user.index_of_warehouse);
                printf("User warehouse: %s", user_warehouse);
                int count_of_orders_in_warehouse = get_count_of_orders_by_warehouse(user_warehouse);
                printf("User count of orders: %d\n", count_of_orders_in_warehouse);
                sprintf(message.text, "%d\n", count_of_orders_in_warehouse);
                send(sock2, &message, sizeof(message), 0);

                order* orders = get_orders_for_warehouse(user_warehouse);
                for (int i = 0; i < count_of_orders_in_warehouse; ++i) {
                    message.order = orders[i];
                    send(sock2, &message, sizeof(message), 0);
                }

                free(user_warehouse);
                free(orders);
                printf("Server send orders from warehouse!\n");
        
            } else if (message.msg_type == CHANGE_ORDER_STATUS) {
                

            } else if (message.msg_type == EXITING) {
                message.username[strlen(message.username) - 1] = '\0';
                printf("Client %s exiting!\n", message.username);
                break;
            }
        }
    } else if (user.type == CONSUMER) {
        while (1) {
            // printf("Before recv int comsumer!\n");
            recv(sock2, &message, sizeof(message), 0);

            if (message.msg_type == CREATE_ORDER) {
                //TODO: from yarik: do not let user to create order for himself
                int code = check_user_name(message.order.username_of_receiver);
                if (code) {
                    message.order.status = CREATED;
                    code = write_order_to_file(message.order);

                    message.username[strlen(message.username) - 1] = '\0';
                    printf("User %s create order!\n", message.username);
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

            } else if (message.msg_type == GET_ORDERS_STATUS) {
                int count_of_orders = get_count_of_orders_by_username(message.username);
                order* orders = get_orders_for_user(message.username);

                sprintf(message.text, "%d\n", count_of_orders);
                send(sock2, &message, sizeof(message), 0);

                for (int i = 0; i < count_of_orders; ++i) {
                    message.order = orders[i];
                    send(sock2, &message, sizeof(message), 0);
                }
                free(orders);

            } else if (message.msg_type == EXITING) {
                message.username[strlen(message.username) - 1] = '\0';
                printf("Client %s exiting!\n", message.username);
                break;
            }
        }

    } else {
        printf("Server doesnt support this type!\n");
    }
    close(sock2);
    pthread_exit(NULL);
}

// return user_record if user was found in db
// return user_record with password contains '\0' chars if password is incorrect
// return user_record with username and password contain '\0' chars if user doesnt exist
user_record check_users_credentials(char* login, char* password) {
    FILE* file;
    file = fopen("usersdb.txt", "r");
    user_record user;
    
    while ((fgets(user.user_name, USERNAME_LEN, file)) != NULL) {
        fgets(user.password, PASSWORD_LEN, file);
        // char temp[3];
        // fgets(temp, 3, file); // fgets read until n-1 chars, need read 2 chars: user.type and \n
        // user.type = temp[0] - '0';
        fscanf(file, "%d", (int*)&user.type);
        if (user.type == WORKER) {
            fscanf(file, "%d", &user.index_of_warehouse);
        } else if (user.type == CONSUMER) {
            user.index_of_warehouse = 0;
        }
        fgetc(file);
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
    file = fopen("usersdb.txt", "a");
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
    file = fopen("orders.txt", "a");
    fprintf(file, "%s%d\n%s%s%s", 
        order.username_of_receiver, order.status, order.destination, order.position, order.content);
    fclose(file);
    return 1;
}

order* get_orders_for_user(char* username) {
    FILE* fp;
    char line[USERNAME_LEN];
    order_status lineStat = 1;
    if ((fp = fopen(fileNameOrders, "r")) == NULL)
    {
        printf("The file could not be opened\n");
        return NULL;
        //return -1;
    }
    int count_of_orders = get_count_of_orders_by_username(username);
    order* o = (order*)malloc(sizeof(order) * count_of_orders);
    int i = 0;

    //yarik mark: сомнительно, ноо ооокей
    while (fgets(line, sizeof(line), fp) || i != count_of_orders) {
        if (STREQU(line, username)) {
            strcpy(o[i].username_of_receiver, line);

            //fscanf(fp, "%d", &lineStat);
            fscanf(fp, "%d", (int*)&lineStat);
            o[i].status = lineStat;
            fgets(line, sizeof(line), fp);

            fgets(line, sizeof(line), fp);
            strcpy(o[i].destination, line);

            fgets(line, sizeof(line), fp);
            strcpy(o[i].position, line);

            fgets(line, sizeof(line), fp);
            strcpy(o[i].content, line);

            /*printf("%s %d\n %s %s %s", o[i].username_of_receiver, o[i].status, o[i].destination,
                o[i].position, o[i].content); if need to proverka */
            i++;
        }
    }
    fclose(fp);
    return o;
}

int check_user_name(char* username) {
    FILE* file;
    file = fopen("usersdb.txt", "r");
    char str[USERNAME_LEN];
    int i = 0;
    while (fgets(str, USERNAME_LEN, file))
    {
        if (STREQU(str, username) && (i % 3 == 0))
            return 1;
        i++;
    }
    return 0;
}

int get_count_of_orders_by_username(char* username) {
    // COMMENT THIS IF YOU WANT TO USE check_user_name OUT OF THIS FUNCTION
    if(!check_user_name(username))
    {
        printf("There\'s no user with username %s\n", username);
        return -1;
    }

    FILE* file;
    file = fopen("orders.txt", "r");
    char str[USERNAME_LEN];
    int i = 0;
    int cnt = 0;
    while (fgets(str, USERNAME_LEN, file))
    {
        if (STREQU(str, username) && (i % 5 == 0))
            cnt++;
        i++;
    }
    return cnt;
}

int get_count_of_warehouses() {
    // Because warehouse.txt has one line for id and one line for name, to get count of warehouses I just divide number of lines by 2
    FILE* file;
    file = fopen("warehouse.txt", "r");
    char str[WAREHOUSE_LEN];
    int i = 0;
    while (fgets(str, WAREHOUSE_LEN, file))
    {
        i++;
    }
    return i / 2;
}

char** get_names_of_warehouses() {
    FILE* fp;
    if ((fp = fopen(fileNameWarehouses, "r")) == NULL)
    {
        printf("The file could not be opened\n");
        return NULL;
    }
    int rows = get_count_of_warehouses();
    char** warehouse = (char**)malloc(rows * sizeof(char*));
    for (int i = 0; i < rows; i++) {
        warehouse[i] = (char*)malloc(64 * sizeof(char));
    }
    char line[64];
    int i = 0;
    while (fgets(line, sizeof(line), fp)) {
        fgets(line, sizeof(line), fp);
        strcpy(warehouse[i], line);
        i++;
    }
    return warehouse;
}

char* get_warehouse_by_id(int id_of_warehouse) {
    // RESULT STRING HAS TO BE FREED AFTER USE!!!
    FILE* file;
    file = fopen("warehouse.txt", "r");
    // Convert int id to char* id to use STREQU
    char id[4];
    sprintf(id, "%d\n", id_of_warehouse);

    char str[WAREHOUSE_LEN];
    int i = 0;
    while (fgets(str, WAREHOUSE_LEN, file))
    {
        if (STREQU(str, id) && (i % 2 == 0))
        {
            fgets(str, WAREHOUSE_LEN, file);
            char *str_to_return = malloc(strlen(str)*sizeof(char));
            strcpy(str_to_return, str);
            return str_to_return;
        } 
    }
    return NULL;
}

int get_count_of_orders_by_warehouse(char* warehouse) {
    FILE* file;
    file = fopen("orders.txt", "r");
    char str[USERNAME_LEN];
    int i = 1;
    int cnt = 0;
    // printf("Input in func: %s with len %ld\n", warehouse, strlen(warehouse));
    while (fgets(str, USERNAME_LEN, file) != NULL)
    {
        if (STREQU(str, warehouse) && (i % 4 == 0)) {
            cnt++;
        }
        ++i;
        if (i == 5) {
            i = 0;
        }
    }
    return cnt;
}

order* get_orders_for_warehouse(char* user_warehouse) {
    FILE* fp;
    char line[4][64];
    order_status status;
    if ((fp = fopen(fileNameOrders, "r")) == NULL)
    {
        printf("The file could not be opened\n");
        return NULL;
    }
    int count_of_orders = get_count_of_orders_by_warehouse(user_warehouse);
    order* o = (order*)malloc(sizeof(order) * count_of_orders);
    int i = 0;
    printf("Warehouse like param: %s", user_warehouse);
            //name
    while (fgets(line[0], 64, fp) != NULL) {
        //status
        fscanf(fp, "%d", (int*)&status);
        fgetc(fp);
        //dest
        fgets(line[1], 64, fp);
        //postion
        fgets(line[2], 64, fp);
        //content
        fgets(line[3], 64, fp);

        if (STREQU(line[2], user_warehouse)) { 
            strcpy(o[i].username_of_receiver, line[0]);
            o[i].status =  status;
            strcpy(o[i].destination, line[1]);
            strcpy(o[i].position, line[2]);
            strcpy(o[i].content, line[3]);
            i++;
        }         
    }

    fclose(fp);
    return o;
}