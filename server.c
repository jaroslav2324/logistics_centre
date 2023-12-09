#include "util.h"

void *user_thread(void *data);
user_record check_users_credentials(char* login, char* password);
int write_order_to_file(order order);
int write_user_to_file(char* login, char* password, user_type isWorker);

// TODO: need to find username and return 1, else return 0
// example
// usersdb.txt
// name_of_user
// user_password
// type_of_user
int check_user_name(char* username);

// TODO: get all orders for user by name from file orders.txt
// Need to allocate memmory and return array of all orders for user
// One user can have more than 1 order, ochevidno
// orders.txt
// aboba - username
// 0 - type of order
// sklad2 - dest
// sklad - srx
// krytaia posilka - description
order* get_orders_for_user(char* username);

// TODO: find count of orders for user by username
int get_count_of_orders(char* username);

// replace order with idx to new order
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
            printf("Connection done!\n");
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

    printf("Username: %s", message.username);
    printf("Password: %s\n", message.text);
    printf("User socket fd: %d\n", sock2);
    
    user_record user = check_users_credentials(message.username, message.text);

    if (STREQU(user.user_name, message.username)) {
        if (STREQU(user.password, message.text)) {
            strcpy(message.text, "Successfully authorization!\n");
            printf("Successfully authorization of %s", message.username);
            message.msg_type = AUTH_SUCCESS;
            message.user_type = user.type; // 
            send(sock2, &message, sizeof(message), 0);

        } else {
            strcpy(message.text, "Wrong password entered!\n");
            message.username[strlen(message.username) - 1] = '\0';
            printf("User %s entered a wrong password!\n", message.username);
            message.msg_type = FORBIDDEN;
            send(sock2, &message, sizeof(message), 0);
            shutdown(sock2, SHUT_RDWR);
            close(sock2);
            pthread_exit(NULL);
        }
    } else {
        char login_from_user[USERNAME_LEN];
        strcpy(login_from_user, message.username);
        char password_from_user[PASSWORD_LEN];
        strcpy(password_from_user, message.text);

        strcpy(message.text, "You're not registered! Do you want to register?(Y/N)\n");
        message.msg_type = REGISTRATION;
        send(sock2, &message, sizeof(message), 0); 

        // wating answer from client
        recv(sock2, &message, sizeof(message), 0);
        user_type isWorker = message.user_type;

        if (strcmp(message.text, "y") == 0) {
            write_user_to_file(login_from_user, password_from_user, isWorker);
            printf("Success registration!\n");
            strcpy(message.text, "Success registration!\n");
            send(sock2, &message, sizeof(message), 0);
            
        } else {
            printf("User doesn't want register on server!\n");
            shutdown(sock2, SHUT_RDWR);
            close(sock2);
            pthread_exit(NULL);
        }
    }

    if (user.type == WORKER) {
        
    } else if (user.type == CONSUMER) {
        while (1) {
            // printf("Before recv int comsumer!\n");
            recv(sock2, &message, sizeof(message), 0);

            if (message.msg_type == CREATE_ORDER) {
                printf("Username of receiver: %s", message.order.username_of_receiver);
                printf("Destination: %s", message.order.destination);
                printf("Position: %s", message.order.position);
                printf("Content about item: %s", message.order.content);
                message.order.status = CREATED;
                
                int code = check_user_name(message.order.username_of_receiver);
                if (code) {
                    code = write_order_to_file(message.order);
                    message.msg_type = OK;
                    send(sock2, &message, sizeof(message), 0);
                } else {
                    message.msg_type = BAD;
                    send(sock2, &message, sizeof(message), 0);
                }

            } else if (message.msg_type == GET_ORDERS_STATUS) {
                //TODO: get count of orders for user from logisticdb.txt and then send it to user
                int count_of_orders = get_count_of_orders(message.username);
                order* orders = get_orders_for_user(message.username);

                //message.text = count_of_orders
                send(sock2, &message, sizeof(message), 0);

                for (int i = 0; i < count_of_orders; ++i) {
                    send(sock2, &message, sizeof(message), 0);
                }
                //TODO: free memmory for orders

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

int write_user_to_file(char* login, char* password, user_type isWorker) {
    FILE* file;
    file = fopen("usersdb.txt", "a");
    fprintf(file, "%s%s%d\n", login, password, isWorker);
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
    return NULL;
}

int get_id_of_user_from_username(char* username) {
    return 1;
}

int check_user_name(char* username) {
    return 1;
}

int get_count_of_orders(char* username) {
    return 1;
}