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
#define STR_FILE_LEN 128
#define MSG_BUFF_SIZE 256

#define SERVER_PORT 48125
#define SERVER_ADDR "192.168.29.167"//"127.0.0.1" //
#define STREQU(a,b)	(strcmp(a, b) == 0)

#define ORDERS_FILE "orders.txt"
#define WAREHOUSES_FILE "warehouses.txt"
#define USERSDB_FILE "usersdb.txt"

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
    GET_ORDERS_STATUS_SENDER,
    GET_ORDERS_STATUS_RECEIVER,
    GET_ORDERS_WAREHOUSE,
    GET_ORDERS_AWAITING
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

typedef enum find_order_type {
    SENDER,
    RECEIVER,
    DESTINATION,
    POSITION
} find_order_type;

// TODO: user write 
typedef struct order{
    int index;
    char username_of_sender[USERNAME_LEN];
    char username_of_receiver[USERNAME_LEN];
    order_status status;
    char destination[CONTENT_SIZE];
    char position[CONTENT_SIZE];
    char content[CONTENT_SIZE];
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
    int index_of_warehouse;
} user_record;

typedef struct warehouse_record {
    int id;
    char position[USERNAME_LEN];

} warehouse_record;

