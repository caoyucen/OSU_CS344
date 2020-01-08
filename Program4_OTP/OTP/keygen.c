/*Program name: Program 4 - OTP
Author: YUCEN CAO - CS344 Fall 2019
ONID: caoyuc
Date: Dec 6, 2019
Description: In this assignment, you will be creating five small programs that encrypt and decrypt information using a one-time pad-like system. I believe that you will find the topic quite fascinating: one of your challenges will be to pull yourself away from the stories of real-world espionage and tradecraft that have used the techniques you will be implementing.
*/

# include <stdio.h>
# include <stdlib.h>
# include <time.h>

int main(int argc, char** argv) {
    if (argc <= 1 || argc > 2) {
        fprintf(stderr, "Expecting exactly 1 argument, but %d given, exiting program\n", argc - 1);
        return 1;
    }
    int keySize = atoi(argv[1]);
    if (keySize > 0) {
        /* initialize rand seed with current timestamp */
        time_t ts; 
        srand((unsigned) time(&ts));

        char* key = malloc(keySize * sizeof(char) + 2);
        int i = 0;
        for (i = 0; i < keySize; i++) {
            key[i] = 'A' + rand() % 27;
            if (key[i] == 'A' + 26) {
                /* substitute '[' with ' ' */
                key[i] = ' ';
            }
        }
        key[i++] = '\n';
        key[i] = '\0';
        printf("%s", key);
    } else {
        fprintf(stderr, "argument is not valid integer or <= 0, exitig program\n");
    }
    return 0;
}
