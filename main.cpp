#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <utility>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
unsigned int fail_counter;
int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "incorrect number of args" << endl;
        exit(1);
    }
    if(atoi(argv[1])< 10000 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 ){
        cout << "incorrect number of args" << endl;
        exit(1);
    }

    string port = argv[1];
    int timeout = atoi(argv[2]);
    int max_num_of_resends = atoi(argv[3]);

    struct timeval time_val;
    time_val.tv_sec = timeout;
    time_val.tv_usec = 0;

    //create socket
    int socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        perror("TTFTP_ERROR: socket() failed");
        exit(1);
    }
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;

    //initial out structures
    memset(&ServAddr, 0, sizeof(echoServAddr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(echoServPort);

    //bind to the address
    if (bind(socket_, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("TTFTP_ERROR: bind() failed");
        exit(1);
    }
}




