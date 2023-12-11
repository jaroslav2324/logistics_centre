#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

void *user_thread(void *data);
user_record check_users_credentials(char* login, char* password);
int write_order_to_file(order order);
int write_user_to_file(user_record user, int id_of_warehouse);

// TODO: need to find username in file usersdb.txt and return 1, else return 0
// example of record about one user in file
// usersdb.txt
// name_of_user
// user_password
// type_of_user
int check_user_name(char* username);

// WELL DONE: get all orders for user by name from file orders.txt
// Need to allocate memmory and return array of all orders for user
// One user can have more than 1 order, ochevidno
// example about one order in file
// orders.txt 
// aboba - username
// 0 - type of order
// sklad2 - dest
// sklad - srx
// krytaia posilka - description
order* get_orders_for_user(char* username);

// TODO: find count of orders for user by username
int get_count_of_orders(char* username);

// TODO: get count of orders from warehouse.txt
// warehouse.txt contains records like:
// 1 - id of warehouse
// Sklad123 - name of warehouse
// You need to count of warehouses and return this number
int get_count_of_warehouses();

//TODO: need to allocate memory for array of names of warehouses
// and return this array of names, read file warehouse.txt
// example how warehouse.txt looks like see above
char** get_names_of_warehouses();


// TODO: need to allocate memory for char array and find warehouse by id 
// and return its name, example how warehouse.txt looks like see above
char* get_warehouse_by_id(int id_of_warehouse);

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

                send(sock2, &message, sizeof(message), 0);
                int id_of_warehouse = atoi(message.text);
                char* warehouse = get_warehouse_by_id(id_of_warehouse);
                printf("Worker choose %s!", warehouse);
                free(warehouse);
                write_user_to_file(user, id_of_warehouse);
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
                

            } else if (message.msg_type == GET_ORDERS_STATUS) {
                

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
                //TODO: get count of orders for user from logisticdb.txt and then send it to user
                int count_of_orders = get_count_of_orders(message.username);
                order* orders = get_orders_for_user(message.username);
                // orders = (order*)malloc(sizeof(order) * count_of_orders);

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
        char temp[3];
        fgets(temp, 3, file); // fgets read until n-1 chars, need read 2 chars: user.type and \n
        user.type = temp[0] - '0';
        
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
        return -1;
    }
    int count_of_orders = get_count_of_orders(username);
    order* o = (order*)malloc(sizeof(order) * count_of_orders);
    int i = 0;

    while (fgets(line, sizeof(line), fp) || i != count_of_orders) {
        if (STREQU(line, username) == 1) {
            strcpy(o[i].username_of_receiver, line);

            fscanf(fp, "%d", &lineStat);
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

int get_count_of_orders(char* username) {
    // COMMENT THIS IF YOU WANT TO USE check_user_name OUT OF THIS FUNCTION
    if(!check_user_name(username))
    {
        printf("There\'s no user with username %s", username);
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
    return 1;
}

char** get_names_of_warehouses() {
    return NULL;
}

char* get_warehouse_by_id(int id_of_warehouse) {
    return NULL;
}