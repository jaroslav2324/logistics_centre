#/bin/bash

clear
echo "compiling..."
gcc server.c -o server
gcc client.c -o client
echo "done"