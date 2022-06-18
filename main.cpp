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
    char buffer[ECHOMAX] = {0};
    char data[ECHOMAX] = {0};
    unsigned short port = atoi(argv[1]);
    int timeout = atoi(argv[2]);
    int max_num_of_resends = atoi(argv[3]);
    short Opcode;
    short ack_number = 0;
    short block_num = 0;

    //setting time struct
    fd_set readfds;
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
    //start transaction
    while(true) {
        unsigned int client_addr_len = sizeof(clnt_addr);
        /* Set the size of the in-out parameter */
        /* Block until receive message from a client */
        if((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,(struct sockaddr *) &clnt_addr, &client_addr_len)) < 0){
            perror("TTFTP_ERROR: recvfrom() failed");
            exit(1);
        }
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
            perror("TTFTP_ERROR: open() failed");
           // cout << "RECVFAIL" << endl;
           //todo error exist file already
            Error Error_packet;
            Error_packet.Opcode = htons(6);
            strcpy(Error_packet.Error_msg, "File already exists");
            if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                perror("TTFTP_ERROR: sendto() failed");
                close(Packet_file);
                unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                exit(1);
            }

        }
        ACK serv_ack;
        serv_ack.Opcode = htons(4);
        serv_ack.Block_num = htons(ack_number);


        if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) &clnt_addr, sizeof(clnt_addr)) < 0) {
            perror("TTFTP_ERROR: sendto() failed");
            close(Packet_file);
            unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
            exit(0);
        }
        //receive data
        int sel_result;
        do{
            do{
                do{
                    // for us at the socket (we are waiting for DATA)
                    FD_ZERO(&readfds);
                    FD_SET(my_socket, &readfds);
                    sel_result = select(my_socket + 1, &readfds, nullptr, nullptr, &time_val);

                    if (sel_result>0) //if there was something at the socket and we are here not because of a timeout
                    {
                        if ((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,
                                                    (struct sockaddr*) & clnt_addr, &client_addr_len)) < 0) {
                            //todo send error packet
                            perror("TTFTP_ERROR: recvfrom() failed");
                            close(Packet_file);
                            unlink(file_name);
                            exit(1);
                        }
                        else { //copy data to server
                            Data data_res;
                            memcpy(&(data_res.Opcode), buffer, 2);
                            memcpy(&(data_res.Block_num), &(buffer[2]), 2);
                            memcpy(data_res.data, &(buffer[4]), recvMsgSize - 4);
                            memcpy(data, data_res.data, recvMsgSize - 4);
                            Opcode = ntohs(data_res.Opcode);
                            block_num = ntohs(data_res.Block_num);
                        }
                    }
                    if (sel_result==0) //Time out expired while waiting for data to appear at the socket
                    {
                        fail_counter++;
                        time_val.tv_sec = timeout; //init time
                        time_val.tv_usec = 0;

                        //sending another ack
                        serv_ack.Opcode = htons(4);
                        serv_ack.Block_num = htons(ack_number);
                        if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                            perror("TTFTP_ERROR: sendto() failed");
                            close(Packet_file);
                            unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                            exit(1);
                        }
                        //cout << "OUT:ACK,"<< ackCounter << endl;
                    }
                    if (fail_counter >= max_num_of_resends)
                    {
                        close(Packet_file);
                        unlink(&file_name[0]);
                        Error Error_packet;
                        Error_packet.Opcode = htons(0);
                        strcpy(Error_packet.Error_msg, "Abandoning file transmission");
                        if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                            perror("TTFTP_ERROR: sendto() failed");
                            close(Packet_file);
                            unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                            exit(1);
                        }
                        exit(1);
                    }
                } while (sel_result == 0 || recvMsgSize == 0);
                if (Opcode != 3) //We got something else but DATA
                {
                    Error Error_packet;
                    Error_packet.Opcode = htons(4);
                    strcpy(Error_packet.Error_msg, "Unexpected packet");
                    // error packet FATAL ERROR BAIL OUT
                    close(Packet_file);
                    unlink(&file_name[0]);
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        close(Packet_file);
                        unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                        exit(1);
                    }
                    exit(1);
                }

                if (block_num != ack_number + 1) //The incoming block number is not what we have expected, i.e. this is a DATA pkt but the block number in DATA was wrong (not last ACKs block number + 1)
                {
                    close(Packet_file);
                    unlink(&file_name[0]);
                    Error Error_packet;
                    Error_packet.Opcode = htons(0);
                    strcpy(Error_packet.Error_msg, "Bad block number");
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        close(Packet_file);
                        unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                        exit(1);
                    }
                    //exit(1);
                }
            } while (false);
            fail_counter = 0;
            int lastWriteSize = write(Packet_file, data, recvMsgSize - 4); // write next bulk of data
            if (lastWriteSize < 0) {
                perror("TTFTP_ERROR: write() failed");
                close(Packet_file);
                unlink(&file_name[0]);
                exit(1);
            }
           // cout << "WRITING: " << lastWriteSize << endl;


            // TODO: send ACK packet to the client
            ack_number++;
            serv_ack.Opcode = htons(4);
            serv_ack.Block_num = htons(ack_number);
            if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                perror("TTFTP_ERROR: sendto() failed");
                //cout << "RECVFAIL" << endl;
                close(Packet_file);
                unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                exit(1);
            }
            //cout << "OUT:ACK,"<< ackCounter << endl;

        } while (recvMsgSize == 516);
        ack_number = 0;
        close(Packet_file);
    }

}




