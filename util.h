#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define USERNAME_LEN 32
#define PASSWORD_LEN 32
#define ADDRESS_LEN 128
#define CONTENT_SIZE 128

#define MSG_BUFF_SIZE 256
//#define USER_NUMBER 3

#define SERVER_PORT 48124
#define SERVER_ADDR "192.168.29.167" //"127.0.0.1"
#define STREQU(a,b)	(strcmp(a, b) == 0)

char fileNameOrders[] = "orders.txt";

typedef enum msg_types{
    MSG_AUTH, 
    REGISTRATION,
    FORBIDDEN,
    EXITING,
    AUTH_SUCCESS,
    OK, // server marks message if all ok
    BAD, // server marks message if not ok
    CREATE_ORDER, // worker creates order
    CHANGE_ORDER_STATUS, // worker changes status of order
    DELETE_ORDER, // worker deletes order???
    GET_ORDERS_STATUS, // user requests info about all orders???
    GET_ORDERS_WAREHOUSE
} msg_types;

typedef enum user_type {
    CONSUMER,
    WORKER
} user_type;

typedef enum order_status{
    CREATED,
    MOVING, // more statuses???
    READY_TO_TAKE, // waiting to take
    DELIVERED // user has taken order
} order_status;

// TODO: user write 
typedef struct order{
    //int idx; 
    char username_of_receiver[USERNAME_LEN];
    order_status status;
    char destination[64];
    char position[64];
    char content[64];
} order;

typedef struct message{
    msg_types msg_type;
    char username[USERNAME_LEN];
    char text[MSG_BUFF_SIZE];
    order order;
    user_type user_type;
} message;

typedef struct user_record {
    int id;
    char user_name[USERNAME_LEN];
    char password[PASSWORD_LEN];
    user_type type;
} user_record;

typedef struct warehouse_record {
    int id;
    char position[USERNAME_LEN];

} warehouse_record;

