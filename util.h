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
#define SERVER_ADDR "192.168.202.48"//"127.0.0.1"
#define STREQU(a,b)	(strcmp(a, b) == 0)

typedef enum msg_types{
    MSG_AUTH, 
    REGISTRATION,
    FORBIDDEN,
    EXITING,
    CREATE_ORDER, // worker creates order
    UPDATE_ORDER, // worker updates order
    DELETE_ORDER, // worker deletes order???
    GET_ORDERS_STATUS // user requests info about all orders???
} msg_types;

typedef struct message{
    msg_types msg_type;
    char username[USERNAME_LEN];
    char text[MSG_BUFF_SIZE];
    
} message;


typedef enum order_status{
    CREATED,
    MOVING, // more statuses???
    READY_TO_TAKE, // waiting to take
    DELIVERED // user has taken order
} order_status;

typedef struct order{
    int idx; // order index ?? as a primary key :)
    order_status status;
    char receiver[USERNAME_LEN];
    char receiver_address[ADDRESS_LEN]; // needed???
    char current_address[ADDRESS_LEN]; // needed?? what if moving somewhere in the current moment?
    char content[CONTENT_SIZE]; // delivering this
} order;

typedef enum user_type {
    CONSUMER,
    WORKER
} user_type;

typedef struct user_record {
    char user_name[USERNAME_LEN];
    char password[PASSWORD_LEN];
    user_type isWorker;
} user_record;