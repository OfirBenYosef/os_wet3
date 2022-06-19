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
int fail_counter;

//#define RESPONSE_MSG "I got your message”

using namespace std;
int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "TTFTP_ERROR: incorrect number of args" << endl;//cerr - illigal ars
        exit(1);
    }
    cout<<"here 1"<<endl;
    if(atoi(argv[1])< 10000 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 ){
        cout << "incorrect  args" << endl;
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
    cout<<"here 2"<<endl;
    //setting time struct
    fd_set readfds;
    struct timeval time_val;
    time_val.tv_sec = timeout;
    time_val.tv_usec = 0;
    cout<<"here 3"<<endl;
    //create socket
    int my_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (my_socket < 0) {
        perror("TTFTP_ERROR: socket() failed");
        exit(1);
    }
    cout<<"here 4"<<endl;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    cout<<"here 5"<<endl;
    //initial out structures
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    //bind to the address
    //int a=bind(socket_, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    cout<<"here 6"<<endl;
    if ((bind(my_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)) {
        perror("TTFTP_ERROR: bind() failed");
        exit(1);
    }
    //start transaction
    cout<<"here 7"<<endl;
    while(true) {
        unsigned int client_addr_len = sizeof(clnt_addr);
        /* Set the size of the in-out parameter */
        /* Block until receive message from a client */
        if((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,(struct sockaddr *) &clnt_addr, &client_addr_len)) < 0){
            perror("TTFTP_ERROR: recvfrom() failed");
            exit(1);
        }
        cout<<"here 8"<<endl;
        //parse the data resived from recvfrom
        WRQ tmp_WRQ;
        memcpy(&(tmp_WRQ.Opcode), buffer, 2);
        strcpy(tmp_WRQ.file_name, &(buffer[2]));
        strcpy(tmp_WRQ.Tran_mode, &(buffer[3 + strlen(tmp_WRQ.file_name)]));
        cout<<"here 9"<<endl;
        Opcode = ntohs(tmp_WRQ.Opcode);
        char* file_name = tmp_WRQ.file_name;
        char* Tran_mode = tmp_WRQ.Tran_mode;
        cout<<"here 10"<<endl;
        //check packet parameters:
       if (Opcode != 2 || strcmp(Tran_mode, "octet") != 0 || strlen(file_name) > ECHOMAX) {
            //cout << "FLOWERROR: packet parameters are bad" << endl;
            continue;
       }
        cout<<"here 11"<<endl;
        //open a file for claint
        int Packet_file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, S_IRWXU);
        if (Packet_file < 0) {
            perror("TTFTP_ERROR: open() failed");
           // cout << "RECVFAIL" << endl;
           //todo error exist file already
            Error Error_packet;
            Error_packet.Opcode = htons(5);
            Error_packet.Error_code = htons(6);
            cout<<"here 12"<<endl;
            strcpy(Error_packet.Error_msg, "File already exists");
            if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                perror("TTFTP_ERROR: sendto() failed");
                close(Packet_file);
                unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
                exit(1);
            }
            cout<<"here 13"<<endl;
        }
        ACK serv_ack;
        serv_ack.Opcode = htons(4);
        serv_ack.Block_num = htons(ack_number);
        cout<<"here 14"<<endl;

        if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) &clnt_addr, sizeof(clnt_addr)) < 0) {
            perror("TTFTP_ERROR: sendto() failed");
            close(Packet_file);
            unlink(&file_name[0]); //deletes a name from the file system. If that name was the last link to a file and no processes have the file open the file is deleted and the space it was using is made available for reuse.
            exit(0);
        }
        cout<<"here 15"<<endl;
        //receive data
        int sel_result;
        do{
            do{
                do{
                    // for us at the socket (we are waiting for DATA)
                    cout<<"here 16"<<endl;
                    FD_ZERO(&readfds);
                    FD_SET(my_socket, &readfds);
                    sel_result = select(my_socket + 1, &readfds, nullptr, nullptr, &time_val);

                    if (sel_result>0) //if there was something at the socket and we are here not because of a timeout
                    {
                        cout<<"here 17"<<endl;
                        if ((recvMsgSize = recvfrom(my_socket, buffer, ECHOMAX, 0,
                                                    (struct sockaddr*) & clnt_addr, &client_addr_len)) < 0) {
                            //todo send error packet
                            perror("TTFTP_ERROR: recvfrom() failed");
                            close(Packet_file);
                            unlink(file_name);
                            exit(1);
                        }

                        else { //copy data to server
                            cout<<"here 18"<<endl;
                            Data data_res;
                            memcpy(&(data_res.Opcode), buffer, 2);
                            memcpy(&(data_res.Block_num), &(buffer[2]), 2);
                            memcpy(data_res.data, &(buffer[4]), recvMsgSize - 4);
                            memcpy(data, data_res.data, recvMsgSize - 4);
                            Opcode = ntohs(data_res.Opcode);
                            block_num = ntohs(data_res.Block_num);
                        }
                    }
                    cout<<"here 19"<<endl;
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
                            unlink(&file_name[0]);
                            exit(1);
                        }
                        cout<<"here 20"<<endl;
                    }
                    if (fail_counter >= max_num_of_resends)
                    {
                        cout<<"here 21"<<endl;
                        close(Packet_file);
                        unlink(&file_name[0]);
                        Error Error_packet;
                        Error_packet.Opcode = htons(5);
                        Error_packet.Error_code = htons(0);
                        strcpy(Error_packet.Error_msg, "Abandoning file transmission");
                        if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                            perror("TTFTP_ERROR: sendto() failed");
                            close(Packet_file);
                            unlink(&file_name[0]);
                            exit(1);
                        }
                        cout<<"Error: "<<Error_packet.Error_msg<<","<<ntohs(Error_packet.Error_code)<<endl;
                        exit(1);

                    }
                    cout<<"here 22"<<endl;

                } while (sel_result == 0 || recvMsgSize == 0);
                if (Opcode != 3) //We got something else but DATA
                {
                    Error Error_packet;
                    Error_packet.Opcode = htons(5);
                    Error_packet.Error_code = htons(4);
                    strcpy(Error_packet.Error_msg, "Unexpected packet");
                    // error packet FATAL ERROR BAIL OUT
                    cout<<"here 23"<<endl;
                    close(Packet_file);
                    unlink(&file_name[0]);
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        close(Packet_file);
                        unlink(&file_name[0]);
                        exit(1);
                    }
                    cout<<"Error: "<<Error_packet.Error_msg<<","<<ntohs(Error_packet.Error_code)<<endl;
                    exit(1);
                }

                if (block_num != ack_number + 1)
                {
                    cout<<"here 24"<<endl;
                    close(Packet_file);
                    unlink(&file_name[0]);
                    Error Error_packet;
                    Error_packet.Opcode = htons(5);
                    Error_packet.Error_code = htons(0);
                    strcpy(Error_packet.Error_msg, "Bad block number");
                    if (sendto(my_socket, &Error_packet, sizeof(Error_packet), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                        perror("TTFTP_ERROR: sendto() failed");
                        close(Packet_file);
                        unlink(&file_name[0]);
                        exit(1);
                    }
                    cout<<"Error: "<<Error_packet.Error_msg<<","<<ntohs(Error_packet.Error_code)<<endl;
                    //exit(1);
                }
                cout<<"here 25"<<endl;
            } while (false);
            fail_counter = 0;
            int lastWriteSize = write(Packet_file, data, recvMsgSize - 4); // write next bulk of data
            if (lastWriteSize < 0) {
                perror("TTFTP_ERROR: write() failed");
                close(Packet_file);
                unlink(&file_name[0]);
                exit(1);
            }
            cout<<"here 26"<<endl;
            // TODO: send ACK packet to the client
            ack_number++;
            serv_ack.Opcode = htons(4);
            serv_ack.Block_num = htons(ack_number);
            if (sendto(my_socket, &serv_ack, sizeof(serv_ack), 0, (struct sockaddr*) & clnt_addr, sizeof(clnt_addr)) < 0) {
                perror("TTFTP_ERROR: sendto() failed");
                //cout << "RECVFAIL" << endl;
                close(Packet_file);
                unlink(&file_name[0]);
                exit(1);
            }
            cout<<"here 27"<<endl;
        } while (recvMsgSize == 516);
        ack_number = 0;
        cout<<"here 28"<<endl;
        close(Packet_file);
    }

}




