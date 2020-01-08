/*Program name: Program 4 - OTP
Author: YUCEN CAO - CS344 Fall 2019
ONID: caoyuc
Date: Dec 6, 2019
Description: In this assignment, you will be creating five small programs that encrypt and decrypt information using a one-time pad-like system. I believe that you will find the topic quite fascinating: one of your challenges will be to pull yourself away from the stories of real-world espionage and tradecraft that have used the techniques you will be implementing.
 
 * Server that listens client requests and send decrypted data back to client
 *
 * TCP Packet Protocols (NAME (bytes)):
 *
 * 1. client to server: CLIENT_TYPE (1) TEXT_SIZE (3) KEY_SIZE (3) TEXT (TEXT_SIZE) KEY (KEY_SIZE)
 * 2. server to client: OUTPUT (TEXT_SIZE)
 */

#include "common.h"

int main(int argc, char** argv) {
    serverMain(argc, argv, DEC_TYPE);
    return 0;
}
