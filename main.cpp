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
#include "tftp.h"

#define ECHOMAX 512
unsigned int fail_counter;

//#define RESPONSE_MSG "I got your message”

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
    //vars
    int recvMsgSize;
    char buffer[512] = {0};
    unsigned short port = atoi(argv[1]);
    int timeout = atoi(argv[2]);
    int max_num_of_resends = atoi(argv[3]);
    short Opcode;
    short ack_number = 0;

    //setting time struct
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


    while(true) {
        unsigned int client_addr_len = sizeof(clnt_addr);
        /* Set the size of the in-out parameter */
        /* Block until receive message from a client */
        if((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,(struct sockaddr *) &clnt_addr, &client_addr_len)) < 0){
            perror("recvfrom() failed");
            exit(1);
        }
      /*  printf("Handling client %s\n", inet_ntoa(clnt_addr.sin_addr));
        //Send received datagram back to the client
        if (sendto(my_socket, buffer, recvMsgSize, 0,(struct sockaddr *) &clnt_addr,sizeof(clnt_addr)) != recvMsgSize){
            perror("sendto() sent a different number of bytes than expected");
        }*/

        //parse the data resived from recvfrom
        WRQ tmp_WRQ;
        memcpy(&(tmp_WRQ.Opcode), buffer, 2);
        strcpy(tmp_WRQ.file_name, &(buffer[2]));
        strcpy(tmp_WRQ.Tran_mode, &(buffer[3 + strlen(tmp_WRQ.file_name)]));

        Opcode = ntohs(tmp_WRQ.Opcode);
        char* file_name = tmp_WRQ.file_name;
        char* Tran_mode = tmp_WRQ.Tran_mode;

        //check packet parameters:
       if (Opcode != 2 || strcmp(Tran_mode, "octet") != 0 || strlen(file_name) > ECHOMAX) {
            //cout << "FLOWERROR: packet parameters are bad" << endl;
            continue;
       }
        //open a file for claint
        int Packet_file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if (Packet_file < 0) {
           // perror("TTFTP_ERROR: open() failed");
           // cout << "RECVFAIL" << endl;
           //todo error exist file already
            fail_counter++;
        }
        ACK serv_ack;
        serv_ack.Opcode = htons(4);
        serv_ack.Block_num = htons(ack_number);
        if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) &clnt_addr, sizeof(clnt_addr)) < 0) {
            //todo
            close(Packet_file);
            unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
        }
        do{
            do{
                do{

                } while (...);//todo condition
            } while (...);//todo condition
            ack_number = 0;
        } while (recvMsgSize == 516);



    }
}




