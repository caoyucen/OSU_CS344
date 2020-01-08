/*
 Program name: Program 4 - OTP
 Author: YUCEN CAO - CS344 Fall 2019
 ONID: caoyuc
 Date: Dec 6, 2019
 Description: In this assignment, you will be creating five small programs that encrypt and decrypt information using a one-time pad-like system. I believe that you will find the topic quite fascinating: one of your challenges will be to pull yourself away from the stories of real-world espionage and tradecraft that have used the techniques you will be implementing.
 
 * Client program to encrypt ciphertext with key
 * Neet to support:
 *  - segmenting data into multiple packets
 *  - pick up from where it's left off, in case of network interruption (server received less data than sent)
 *
 * Usage:
 *  ./otp_enc ciphertext key port
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "common.h"

int main(int argc, char** argv) {
    clientMain(argc, argv, DEC_TYPE);
    return 0;
}
