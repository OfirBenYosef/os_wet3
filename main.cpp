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
#include <arpa/inet.h>
#include <netinet/in.h>

unsigned int fail_counter;

#define RESPONSE_MSG "I got your message”

using namespace std;
int main(int argc, char *argv[]) {

    if (argc != 4) {
        cout << "incorrect number of args" << endl;
        exit(1);
    }
    if(atoi(argv[1])< 10000 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 ){
        cout << "incorrect number of args" << endl;
        exit(1);
    }
    char buffer[512] = {0};
    unsigned short port = atoi(argv[1]);
    int timeout = atoi(argv[2]);
    int max_num_of_resends = atoi(argv[3]);

    struct timeval time_val;
    time_val.tv_sec = timeout;
    time_val.tv_usec = 0;

    //create socket
    int my_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (my_socket < 0) {
        perror("TTFTP_ERROR: socket() failed");
        exit(1);
    }
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;

    //initial out structures
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    //bind to the address
    //int a=bind(socket_, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if ((bind(my_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)) {
        perror("TTFTP_ERROR: bind() failed");
        exit(1);
    }
    listen(my_socket,1);
    unsigned int client_addr_len = sizeof(clnt_addr);
    int newsockfd = accept(my_socket,(struct sockaddr *)&clnt_addr, &client_addr_len);
    if (newsockfd < 0) perror("ERROR on accept");
    int n = read(newsockfd, buffer, 512);

    while(true) {

        int cliAddrLen = sizeof(clnt_addr);
        int recvMsgSize;
        if ((recvMsgSize = recvfrom(my_socket, buffer, 512, 0, (struct sockaddr *) &clnt_addr,
                                    reinterpret_cast<socklen_t *>(&cliAddrLen))) < 0)
            perror("recvfrom() failed");
        printf("Handling client %s\n",inet_ntoa(clnt_addr.sin_addr));

        if (sendto(my_socket,buffer, recvMsgSize, 0,(struct sockaddr *) &clnt_addr,sizeof(clnt_addr)) != recvMsgSize)
            perror("sendto() sent a different number of bytes than expected");
    }
}




