#include "server.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc == 3) {
        char* ip = argv[1];
        int port = atoi(argv[2]);
        serve(2, ip, port);
    }
    else if (argc < 3) {
        printf("Not enough arguments. Usage: ./{name} [ip] [port]\n");
        return 1;
    }

    return 0;
}
