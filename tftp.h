//
// Created by Ofir Ben Yosef on 10/06/2022.
//

#ifndef WET3_TFTP_H
#define WET3_TFTP_H
#include <sys/types.h>
#include <sys/wait.h>
#include<sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>

using namespace std;
#define maxPacket 1500
#define maxFilename 256
#define maxData 512

struct WRQ {
    short Opcode;
    char file_name[maxFilename];
    char Tran_mode[maxPacket];
} __attribute__((packed));


struct ACK {
    short Opcode;
    short Block_num;
} __attribute__((packed));

struct Data {
    short Opcode;
    short Block_num;
    char data[maxData];
} __attribute__((packed));

struct Error {
    short Opcode;
    short Error_code;
    char Error_msg[maxData];
    char Tran_mode[maxPacket] = "0";
} __attribute__((packed));

bool checkIfFileExists(const char* filename){
    struct stat buffer;
    int exist = stat(filename,&buffer);
    if(exist == 0)
        return true;
    else
        return false;
}
#endif //WET3_TFTP_H
