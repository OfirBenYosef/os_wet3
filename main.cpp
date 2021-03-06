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
#define _WRQ 2
#define _DATA 3

int fail_counter;


using namespace std;
int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "TTFTP_ERROR: illegal arguments" << endl;
        exit(1);
    }
    if(atoi(argv[1])< 10000 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 ){
        cout << "TTFTP_ERROR: illegal arguments" << endl;
        exit(1);
    }
    //vars
    int recvMsgSize;
    char buffer[ECHOMAX] = {0};
    char data[ECHOMAX] = {0};
    unsigned short port = atoi(argv[1]);
    int timeout = atoi(argv[2]);
    int max_num_of_resends = atoi(argv[3]);
    short Opcode;
    short ack_number = 0;
    short block_num = 0;
    bool the_time_is_out = false;
    //setting time struct
    fd_set readfds;
    struct timeval time_val;
    time_val.tv_sec = timeout;
    time_val.tv_usec = 0;
    //create socket
    int my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

    if ((::bind(my_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)) {
        perror("TTFTP_ERROR: bind() failed");
        exit(1);
    }
    //start transaction
   int state = _WRQ;
    char* file_name;
    int Packet_file;
    char *curr_ip;
    while (true){
        unsigned int client_addr_len = sizeof(clnt_addr);
        int sel_result = 0;
        //-------------------------- (( WRQ )) --------------------------------------//
        if(state == _WRQ){
            //Set the size of the in-out parameter
            // Block until receive message from a client
            if((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,
                                       (struct sockaddr *) &clnt_addr, &client_addr_len)) < 0){
                perror("TTFTP_ERROR: recvfrom() failed");
                exit(1);
            }
            Opcode = ntohs(*((unsigned short*) buffer));
            //if the opcode is not WRQ (2)
            if(Opcode != state){
                Error Error_packet;
                Error_packet.Opcode = htons(5);
                Error_packet.Error_code = htons(7);
                strcpy(Error_packet.Error_msg, "Unknown user");
                if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                    perror("TTFTP_ERROR: sendto() failed");
                    exit(1);
                }
                continue;
            }
            //parse the data resived from recvfrom
            curr_ip = inet_ntoa(clnt_addr.sin_addr);
            WRQ tmp_WRQ;
            memcpy(&(tmp_WRQ.Opcode), buffer, 2);
            strcpy(tmp_WRQ.file_name, &(buffer[2]));
            strcpy(tmp_WRQ.Tran_mode, &(buffer[3 + strlen(tmp_WRQ.file_name)]));

            file_name = tmp_WRQ.file_name;
            char* Tran_mode = tmp_WRQ.Tran_mode;
            //check packet parameters:
            if ( strcmp(Tran_mode, "octet") != 0 || strlen(file_name) > ECHOMAX) {
                continue;
            }
            //open a file for claint
            Packet_file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, S_IRWXU);
            if (Packet_file < 0 && !(errno == EEXIST)) {
                perror("TTFTP_ERROR: open() failed");
                exit(1);
            }

            else if(Packet_file < 0 && (errno == EEXIST)){
                // error exist file already (5)
                Error Error_packet;
                Error_packet.Opcode = htons(5);
                Error_packet.Error_code = htons(6);
                strcpy(Error_packet.Error_msg, "File already exists");
                if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                    perror("TTFTP_ERROR: sendto() failed");
                    close(Packet_file);
                    //unlink(&file_name[0]);
                    exit(1);
                }
                continue;
            }
            state = _DATA;
            ack_number = 0;
        }

        //-------------------- (( DATA )) --------------------------------------//
        else if(state == _DATA){
            // for us at the socket (we are waiting for DATA)
            time_val.tv_sec = timeout; //init time
            time_val.tv_usec = 0;
            FD_ZERO(&readfds);
            FD_SET(my_socket, &readfds);
            sel_result = select(my_socket + 1, &readfds, nullptr, nullptr, &time_val);

            if(sel_result < 0){
                perror("TTFTP_ERROR: select() failed");
                exit(1);
            }

            else if (sel_result==0) //Time out expired while waiting for data to appear at the socket
            {
                fail_counter++;
                the_time_is_out = true;
                if (fail_counter >= max_num_of_resends)
                {
                    close(Packet_file);
                    //unlink(&file_name[0]);
                    Error Error_packet;
                    Error_packet.Opcode = htons(5);
                    Error_packet.Error_code = htons(0);
                    strcpy(Error_packet.Error_msg, "Abandoning file transmission");
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        exit(1);
                    }
                    state = _WRQ;
                    continue;
                }
            }
            else {
                the_time_is_out = false;
                client_addr_len = sizeof(clnt_addr);
                if ((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,
                                            (struct sockaddr *) &clnt_addr, &client_addr_len)) < 0) {
                    perror("TTFTP_ERROR: recvfrom() failed");
                    close(Packet_file);
                    //unlink(file_name);
                    exit(1);
                }
                Opcode = ntohs(*((unsigned short *) buffer));
            }
            if(!the_time_is_out){
                if (Opcode != state || strcmp(inet_ntoa(clnt_addr.sin_addr),curr_ip) ) //We got something else but DATA
                {
                    Error Error_packet;
                    Error_packet.Opcode = htons(5);
                    Error_packet.Error_code = htons(4);
                    strcpy(Error_packet.Error_msg, "Unexpected packet");
                    // error packet FATAL ERROR BAIL OUT
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        close(Packet_file);
                        //unlink(&file_name[0]);
                        exit(1);
                    }
                    continue;
                }
                // write data to file
                Data data_res;
                memcpy(&(data_res.Opcode), buffer, 2);
                memcpy(&(data_res.Block_num), &(buffer[2]), 2);
                memcpy(data_res.data, &(buffer[4]), recvMsgSize - 4);
                memcpy(data, data_res.data, recvMsgSize - 4);
                //Opcode = ntohs(data_res.Opcode);
                block_num = ntohs(data_res.Block_num);
                if(block_num != ack_number + 1){
                    Error Error_packet;
                    Error_packet.Opcode = htons(5);
                    Error_packet.Error_code = htons(0);
                    strcpy(Error_packet.Error_msg, "Bad block number");
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        close(Packet_file);
                        //unlink(&file_name[0]);
                        exit(1);
                    }
                    close(Packet_file);
                    //unlink(&file_name[0]);
                    continue;
                }
                ack_number ++;
                fail_counter = 0;
                int lastWriteSize = write(Packet_file, data, recvMsgSize - 4); // write next bulk of data
                if (lastWriteSize < 0) {
                    perror("TTFTP_ERROR: write() failed");
                    close(Packet_file);
                    //unlink(&file_name[0]);
                    exit(1);
                }
                if (recvMsgSize < ECHOMAX){
                    state = _WRQ;
                    close(Packet_file);
                }
                else{
                    state =_DATA;
                }
            }//end !time is out;
        }//end Data

        //sending another ack
        ACK serv_ack;
        serv_ack.Opcode = htons(4);
        serv_ack.Block_num = htons(ack_number);
        if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
            perror("TTFTP_ERROR: sendto() failed");
            close(Packet_file);
            exit(1);
        }
    }

}




