/*   common.h
Program name: Program 4 - OTP
Author: YUCEN CAO - CS344 Fall 2019
ONID: caoyuc
Date: Dec 6, 2019
Description: In this assignment, you will be creating five small programs that encrypt and decrypt information using a one-time pad-like system. I believe that you will find the topic quite fascinating: one of your challenges will be to pull yourself away from the stories of real-world espionage and tradecraft that have used the techniques you will be implementing.
*/

#ifndef LOCALHOST
#define LOCALHOST "localhost"
#endif

#ifndef PROC_POOL_SIZE
#define PROC_POOL_SIZE 5
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE 131072
#endif

#ifndef SIZE_HEADER_BYTES
#define SIZE_HEADER_BYTES 3
#endif 

/** client type identifier, 'e' for encryption,'d' for decryption */
#ifndef CLIENT_TYPE_BYTES
#define CLIENT_TYPE_BYTES 1
#endif

#ifndef ENC_TYPE
#define ENC_TYPE 'e'
#endif

#ifndef DEC_TYPE
#define DEC_TYPE 'd'
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/* Description: function pointer for encrypt/decrypt */
typedef void (*pf)(char*, const char*, const char*, int);

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int charToInt(const char c) {
    if (c == ' ') {
        return 26;
    }
    return c - 'A';
}

char intToChar(const int i) {
    if (i == 26) {
        return ' ';
    }
    return 'A' + i;
}




/* Description: encode msg size [0 - MAX_MSG_SIZE) to a 3 byte char array */
void uInt32ToBytes(char* bytes, uint32_t size) {
   bytes[0] = size % 256; 
   bytes[1] = size / 256;
   bytes[2] = size / 65536;
} 




/* Description: check bad charactor */
int hasBadChar(const char* data, int size) {
   int i = 0;
   while (i < size && data[i] != '\0') {
        if ((data[i] < 'A' || data[i] > 'Z') && data[i] != ' ') {
	    return 1;
	}
	i++;
   }
   return 0;
}




/* Description: when writing data, we don't need to return size, client side will only parse up to length of plaintext */
int writeData(char* data, int connectionFD, int msgSize, int bufSize) {
    int cursor = 0; 
    int charsWrite = 0;
    int totalWrite = 0;
    while (cursor < msgSize) {
        int charsToWrite = (bufSize > msgSize - cursor) ? msgSize - cursor : bufSize;
        while ((charsWrite = send(connectionFD, data + cursor, charsToWrite, 0)) < 0) {
            //perror("ERROR: write data error, retrying\n");
        }
        cursor += charsWrite;
        totalWrite += charsWrite;
    }
    return totalWrite;
}




/* Description: read actual data recursively, each time read bufSize Bytes */
/* Return values: < 0: errors; >= 0: success, value is the total bytes read */
int readData(char* data, int connectionFD, int msgSize, int bufSize) {
   int cursor = 0; 
   int charsRead = 0;
   int totalRead = 0;
    while (cursor < msgSize) {
        charsRead = 0;
        /* recv min(BUFFER_SIZE, remaining chars) bytes */
        int charsToRead = (bufSize > msgSize - cursor)? msgSize - cursor : bufSize; 
        charsRead = recv(connectionFD, data + cursor, charsToRead, 0); 
        if (charsRead < 0) {
            //perror("ERROR: read data\n");
            memset(data, '\0', MAX_MSG_SIZE);
            return -1;
        }
        cursor += charsRead; 
        totalRead += charsRead;
    }
    // data[cursor] = '\0'; // append a '\0'
    return totalRead;
}




/* Description:  convert byte string of size 4 to unsigned int, supported max msg size is MAX_MSG_SIZE */
uint32_t bytesToUInt32(const char* header) {
    //printf("DEBUG: header size uint32 = %d, header[0] = %c, header[1] = %c\n", header[0] + header[1] * 256, header[0], header[1]);
    return header[0] + header[1] * 256 + header[2] * 65536;
}




/* */
char getClientTypeHeader(int connectionFD) {
    char header[CLIENT_TYPE_BYTES + 1] = {'\0'};
    int charsRead = recv(connectionFD, header, CLIENT_TYPE_BYTES, 0);
    if (charsRead < CLIENT_TYPE_BYTES) {
      fprintf(stderr, "ERROR: read client type header error, expected %d bytes", CLIENT_TYPE_BYTES);
      return '\0';
    }
    char ret = header[0];
    return ret; 
}




/* */
int32_t getMessageSizeHeader(int connectionFD) {
    char header[SIZE_HEADER_BYTES + 1] = {'\0'};
    int charsRead = recv(connectionFD, header, SIZE_HEADER_BYTES, 0);
    if (charsRead < SIZE_HEADER_BYTES) {
        fprintf(stderr, "ERROR: read %d bytes total, expected %d bytes\n", charsRead, SIZE_HEADER_BYTES);
        return -1;
    }
    // return int32 size
    return (int32_t)(bytesToUInt32(header));
}




/* Description: decrypt ciphertext  using key, store decrypted text in plaintext */
void decryptText(char* plaintext, const char* ciphertext, const char* key, int size) {
    int i = 0;
    while (i < size && ciphertext[i] != 0) {
        if(charToInt(ciphertext[i]) - charToInt(key[i]) < 0){
            plaintext[i] = intToChar((charToInt(ciphertext[i]) - charToInt(key[i]) + 27 )  % 27);
        }
        else{
            plaintext[i] = intToChar((charToInt(ciphertext[i]) - charToInt(key[i])) % 27);
        }
        i++;
    }
}




/* Description: encrypt plaintext using key, store encrypted text in ciphertext */
void encryptText(char* ciphertext, const char* plaintext, const char* key, int size) {
    int i = 0;
    while (i < size && plaintext[i] != 0) {
        ciphertext[i] = intToChar((charToInt(plaintext[i]) + charToInt(key[i])) % 27);
        i++;
    }
    //printf("DEBUG: ciphertext = %s\n", ciphertext);
}

/* Description:  handle TCP request, transmitted data is comprised of a header with 2 uint32, plus data */
int handleRequest(char* text, char* key, char* output, int connectionFD, pf func) {
    memset(text, '\0', MAX_MSG_SIZE);
    memset(key, '\0', MAX_MSG_SIZE);
    /* read header */
    int32_t textLen = getMessageSizeHeader(connectionFD); 
    int32_t keyLen = getMessageSizeHeader(connectionFD);
    if (textLen < 0 || keyLen < 0) {
        perror("ERROR: message size error\n");
        return -1;
    }
    /* read plain/cipher text to text/ciphertext, each time read BUFFER_SIZE bytes*/
    int textRead = readData(text, connectionFD, textLen, BUFFER_SIZE);
    if (textRead < 0) {
        fprintf(stderr, "ERROR: error reading text, expected %d bytes\n", textLen);
        return -1;
    }
    int keyRead = readData(key, connectionFD, keyLen, BUFFER_SIZE);
    if (keyRead < 0) {
        fprintf(stderr, "ERROR: error reading key, expected %d bytes\n", keyLen);
        return -1;
    }
    /* encrypt/decrypt data */
        func(output, text, key, textLen);
    
    /* write data back to client */
    writeData(output, connectionFD, strlen(text), BUFFER_SIZE);
    char endline[2] = {'\0'};
    endline[0] = '\n';
    writeData(endline, connectionFD, strlen(endline), BUFFER_SIZE);
     
    /* return the total number of bytes read */
    return CLIENT_TYPE_BYTES + SIZE_HEADER_BYTES * 2 + textRead + keyRead;
}

/* Description:  generic server impl, set up connection and process pool,
 * serverType can be ENC_TYPE or DEC_TYPE */
void serverMain(int argc, char** argv, const char serverType) {
	int listenSocketFD;
  int establishedConnectionFD;
  int portNumber;
  int charsRead;
  int pid;
	socklen_t sizeOfClientInfo;
	char buffer[BUFFER_SIZE];
  char plaintext[MAX_MSG_SIZE];
  char key[MAX_MSG_SIZE];
  char ciphertext[MAX_MSG_SIZE];
	struct sockaddr_in serverAddress, clientAddress;

    /* set up listening port */
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]); 
        exit(1); // Check usage & args
    }

	/* Set up the address struct for this process (the server) */
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
    
	/* Set up the socket */
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

	/* Enable the socket to begin listening */
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
		perror("ERROR on binding");
        exit(1);
    }
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections


    /* spawn a pool of child processes with size PROC_POOL_SIZE that handles the decryption */
    int i = 0;
    for (i = 0; i < PROC_POOL_SIZE; i++) {
        pid = fork();
        if (pid == 0) {
            /* long running child process that will accept incoming requests */
            while (1) {
                establishedConnectionFD = accept(listenSocketFD, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
                if (establishedConnectionFD < 0) {
                    perror("ERROR on accept");
                    exit(1);
                } 

                /* read client type identifier */
                char clientType = getClientTypeHeader(establishedConnectionFD);
                
                /* close connection if client type doesn't match */
                if (clientType != serverType) {
                    fprintf(stderr, "Error: could not contact otp_enc_d on port %d: Success\n", portNumber);
                    close(establishedConnectionFD);
                } else {
                  if (serverType == ENC_TYPE) {
                    handleRequest(plaintext, key, ciphertext, establishedConnectionFD, &encryptText);
                  } else if (serverType == DEC_TYPE) {
                    handleRequest(ciphertext, key, plaintext, establishedConnectionFD, &decryptText);

                  } else {
                    fprintf(stderr, "ERROR undefined serverType: %c\n", serverType);
                    exit(1);
                  }
                    
                    /* getting data from client and encrtypt it */
                  close(establishedConnectionFD);
              
                }
            }
        }
    }
    close(listenSocketFD);
    /* Need to wait for processes */
    wait(NULL);
    
}

/* Description: generic client impl, entrance for client program */
int clientMain(int argc, char** argv, const char cType) {
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
    char plaintext[MAX_MSG_SIZE] = {'\0'};
    char key[MAX_MSG_SIZE] = {'\0'};
    char ciphertext[MAX_MSG_SIZE] = {'\0'};

	if (argc != 4) { 
        fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
        exit(0);
    } // Check usage & args
    
	/* Set up the server address struct */
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(LOCALHOST); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }

    /* get input data from file */ 
    FILE* fplaintext = fopen(argv[1], "r");
    FILE* fkey = fopen(argv[2], "r");

    /* input data ends with '\0' */
    if (fgets(plaintext, MAX_MSG_SIZE - 1, fplaintext) == NULL) {
      exit(1);
    } 
    if (fgets(key, MAX_MSG_SIZE - 1, fkey) == NULL) {
      exit(1);
    }
    /* Remove the trailing \n that fgets adds */
	plaintext[strcspn(plaintext, "\n")] = '\0'; 	
    key[strcspn(key, "\n")] = '\0';


    if (strlen(plaintext) > strlen(key)) {
        fprintf(stderr, "ERROR key '%s' is too short\n", argv[2]);
	    exit(1);
    }

    uint32_t ptLen = strlen(plaintext);
    uint32_t keyLen = strlen(key);

    if (hasBadChar(plaintext, ptLen) || hasBadChar(key, keyLen)) {
         error("ERROR input contains bad characters\n");
    }
    
    char head[7] = {'\0'};
    uInt32ToBytes(head, ptLen);
    uInt32ToBytes(head + SIZE_HEADER_BYTES, keyLen);

    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {// Connect socket to address
	    fprintf(stderr, "ERROR: could not contact server on port %d\n", portNumber);
	    exit(2);
    }

    /* write client type header */
    char clientType[2] = {'\0'};
    clientType[0] = cType; 
    charsWritten = writeData(clientType, socketFD, CLIENT_TYPE_BYTES, BUFFER_SIZE);



    /* write header of size 6 to server */
    charsWritten = writeData(head, socketFD, SIZE_HEADER_BYTES * 2, BUFFER_SIZE);

    /* write plaintext */
    charsWritten = writeData(plaintext, socketFD, strlen(plaintext), BUFFER_SIZE);

    /* write key */
    charsWritten = writeData(key, socketFD, strlen(key), BUFFER_SIZE);


    /* receive decrypted data from server */
    if ((charsRead = readData(ciphertext, socketFD, strlen(plaintext), BUFFER_SIZE)) > 0) {
        /* print ciphertext read from server */
        
        printf("%s\n", ciphertext);
        return 0;
    } else {
        //error("Debug: ERROR read data from server\n");
    }
    return 0;
}
