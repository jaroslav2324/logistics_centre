#include "util.h"

// some functions we may need(or not)

//write new user/worker to database
int write_user(char* username, char* password);
// find_user ??
// delete user ??


// write order to db using order structure
int write_order(order* order);
// replace order with idx to new order
int update_order(int order_idx, order* order);

// delete_order ??
// archive_order ?? (with DELIVERED status)

int main(int argc, char* argv[]){

    printf("I am server\n");
    return 0;
}