//
//  caoyuc.adventure.c
//*********************************************************************
// Program name: Program 2 - adventure
// Author: YUCEN CAO - CS344 Fall 2019
// Date: Nov 1, 2019
// Description: write two programs that will introduce you to programming in C on UNIX based systems, and will get you familiar with reading and writing files. 
// The first program (hereafter called the "rooms program") will be contained in a file named "<STUDENT ONID USERNAME>.buildrooms.c", which when compiled with the same name (minus the extension) and run creates a series of files that hold descriptions of the in-game rooms and how the rooms are connected.
// The second program (hereafter called the "game") will be called "<STUDENT ONID USERNAME>.adventure.c" and when compiled with the same name (minus the extension) and run provides an interface for playing the game using the most recently generated rooms.
// In the game, the player will begin in the "starting room" and will win the game automatically upon entering the "ending room", which causes the game to exit, displaying the path taken by the player.
// During the game, the player can also enter a command that returns the current time - this functionality utilizes mutexes and multithreading.
// *********************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>




typedef struct  rooms{
    int roomID;						//from 0 to 6, make it easy to run 
    int connect_roomID[6];			//the connect room ID
    char connect_roomName[6][10];	//the connect room names
    int connectNum;					//total num of connect room
    int roomTypeID;					// 1 for start room, 2 for mid room, 3 for end room
}               Room;



// Declare Mutex and thread 
pthread_mutex_t locks[2];
pthread_t threads[2];

//check input vaild
int check_input(char **roomName, char *userInput){
	int z;
    for(z = 0; z < 7; z++){
        if(strcmp(userInput, roomName[z]) == 0){
            return(z);
        }
    }
    return (-1);
}

void get_time(){
	time_t rawtime;
	struct tm *info;
	char buffer[80];
	FILE *timeFile;
	
	time( &rawtime );
	info = localtime( &rawtime );
	strftime(buffer, 80, "%I:%M%p, %A, %B %d, %Y", info);
	// change AM/PM to am and pm
	buffer[5] = tolower(buffer[5]);
	buffer[6] = tolower(buffer[6]);
//	printf(buffer);
//	printf("\n");
	timeFile = fopen("./currentTime.txt", "w+");
	fprintf(timeFile, "%s", buffer);
	fclose(timeFile);
}



void* ReadTimeThread(void *close) {
	int *i_close = (int*) close;  
	char buffer[50];              // buffer for  time
	FILE *timeFile;               
	
	// loop until close flag is set by main thread
	while (!*i_close) {
		pthread_mutex_lock(&locks[1]);
		if (*i_close)
			break;
		// open currentTime file stream for reading, alert if error occurs
		timeFile = fopen("./currentTime.txt", "r");
		if (timeFile == NULL) {
			perror("Error\n");
		}
		else {
			// read current time into buffer
			fgets(buffer, 50, timeFile);
			printf("\n%s\n", buffer);
			fclose(timeFile);
		}
		// unlock mutex 
		pthread_mutex_unlock(&locks[1]);
		usleep(50);
	}
	return NULL;
}


//check the connection
void game(Room *roomarray, char **roomName){
    int startRoom = -1;
    int endRoom = -1;
    int currentRoom = -1;
    int step = 0;
    int path[1000];
	FILE *file;      //time file
	char buffer[200];
	

    memset(path, -1, sizeof(path));
    //find the start and end room
    int i;
    for(i = 0; i < 7; i++){
        //1 stand for start room
        if((roomarray + i)->roomTypeID == 1){
            startRoom = i;
        }
        if((roomarray + i)->roomTypeID == 3){
            endRoom = i;
        }
    }
    //initialize the currentRoom to startRoom
    currentRoom = startRoom;
    //begin input loop
    while(1){
        char userInput[100]; //the string that user input
        int userInputNum;  //if the input is vaild, use name number

        printf("CURRENT LOCATION: %s\n", (roomName[currentRoom]));
        printf("POSSIBLE CONNECTIONS: ");
        int j;
        for(j = 0; j < 6; j++){
            if((roomarray + currentRoom)->connect_roomID[j] != -1){
                if(j > 0){
                    printf(", ");
                }
                printf("%s", (roomName[(roomarray + currentRoom)->connect_roomID[j]]));
            }
        }
        printf(".\nWHERE TO? >");
        scanf("%s", userInput);
        printf("\n");
		
		//check time
		if(strcmp("time", userInput ) == 0){
			get_time();
			
			file = fopen("./currentTime.txt", "r");
			if (file == NULL) {
				perror("Error\n");  //open file erroe
			}
			else{
				fgets(buffer, 200, file);
				printf("%s\n", buffer);
			}
			fclose(file);
			
			
			
			//continue to ask where to 
			printf("\nWHERE TO? >");
			scanf("%s", userInput);
			printf("\n");
		}
		
        //check input vaild
        userInputNum =check_input(roomName, userInput);
        if(userInputNum == -1){ // -1 means the input is not valid
            printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
            continue;
        }
        else{
            int z;
            for(z = 0; z < 6; z++){
                if((roomarray + currentRoom)->connect_roomID[z] != -1){
                    if(userInputNum == (roomarray + currentRoom)->connect_roomID[z]){
                        currentRoom = userInputNum;
                        path[step] = currentRoom;  //store the path
                        step++;						// add the step
                        if(currentRoom == endRoom){
                            printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
                            printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", step);
							int a;
                            for(a = 0; a < step; a++){
                                printf("%s\n", roomName[path[a]]);
                            }
                            return;
                        }
                        break;
                    }
                }
                if(z == 5){ //in the end, can't find the connection
                    printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
                }
            }
        }
    }
}






//open the recently directory
int getDirName(const char *rootdir, char **dirName) {
    int i;
    int j;
    i = -1, // -1 means no dir
    j = 0;  // current rooms dir
  struct stat dirStat;
  DIR *dir;
  struct dirent *dirEnt;

  // open  directory
  dir = opendir(rootdir); // open directory
  if (dir == NULL) {
    perror("Directory Error:\n");
    return -1;
  }
  // read each directory entry
  while ((dirEnt = readdir(dir))) {
    if (strncmp(dirEnt->d_name, "caoyuc.rooms", 12) == 0) {
      // get stats of dir
      stat(dirEnt->d_name, &dirStat);
      j = (int)dirStat.st_mtime;
      if (j > i) {
        strcpy(dirName, dirEnt->d_name);
        i = j;                            // use j as the new one
      }
    }
  }
  closedir(dir);
  return i;
}





//read data from file
int read_data(char *dirName, Room *roomArray, char **roomName){
     DIR *dir;
     FILE *file;
    char fileBuffer[200];
    struct dirent *dirEnt;                //store directory entry
     char *p;                               //point in the line
     int lineIndex;                         //the index of the string in the line
    int roomNum;
    roomNum = 0;
    
    dir = opendir(dirName);               // open gamedir
    if (dir == NULL) {
      perror("Directory Error\n");
      return -1;
    }
    
    //read each files information
    while ((dirEnt = readdir(dir))){
    if (strlen(dirEnt->d_name) > 2){
      // store relative path from ./dirname/.filename
      sprintf(fileBuffer, "./%s/%s", dirName, dirEnt->d_name);
      // open file for reading
      file = fopen(fileBuffer, "r");
      // check that file opened successfully, if not return -1
      if (file == NULL ) {
          perror("File Error\n");
          return (-1);
      }
      
      // read in file line by line
        int connectLine = 0;
        while(fgets(fileBuffer, 200, file)!=NULL){
            p = strchr(fileBuffer, '\n');
            *p = '\0';
            p = strchr(fileBuffer, ':'); // get the point to ':'
            lineIndex = p - fileBuffer + 2; //": " two space
            // room name
            if (strncmp(fileBuffer, "ROOM NAME", 9) == 0){
                strcpy(roomName[roomNum], (p+2));
                roomArray[roomNum].roomID = roomNum;
            }
            //connections
            else if(strncmp(fileBuffer, "CONNECTIO", 9) == 0){
                strcpy(roomArray[roomNum].connect_roomName[connectLine], (p+2));
                connectLine++;
                
                roomArray[roomNum].connectNum = connectLine;
            }
            //room type
            else if(strncmp(fileBuffer, "ROOM TYPE", 9) == 0){
                if(fileBuffer[lineIndex] == 'S'){
                    roomArray[roomNum].roomTypeID = 1;  //start room
                }
                else if(fileBuffer[lineIndex] == 'M'){
                    roomArray[roomNum].roomTypeID = 2;  //mid room
                }
                else if(fileBuffer[lineIndex] == 'E'){
                    roomArray[roomNum].roomTypeID = 3;  //end room
                }
                roomNum++;
            }
        }
    }
    }
    
    return (0);
}




//get the coonect room id 
int getConnectRoomID(char **roomName, char *connectRoomName){
    int i;
    for(i = 0; i < 7; i++){
        if(strncmp(roomName[i], connectRoomName, 4) == 0){  //check the name
            return(i);
        }
    }
           return(-1);
}





int main(){
    char* typeName[] = {"START_ROOM", "MID_ROOM", "END_ROOM"}; //1, 2, 3
    Room roomArray[7];
    char dirName[50];
    int openDirSuccess = 0;
	int mark = 0;

    //initialize the roomarray to -1
    int i;
    for(i = 0; i < 7; i++)
    {
        roomArray[i].roomID = -1;
        memset(roomArray[i].connect_roomID, -1, sizeof(roomArray[i].connect_roomID)); //initialize the connect room to -1
        roomArray[i].roomTypeID = -1;
        roomArray[i].connectNum = 0;
    }
    
    //initialize the roomName
    char** roomName = (char**)malloc(sizeof(char*) * 7);
    int a;
    for (a = 0; a < 7; a++){
        roomName[a] = (char*)malloc(sizeof(char) * 10);
        int b;
        for(b = 0; b < 10; b++){
            roomName[a][b] = '\0';
        }
    }
    
	// Initialize both mutexes and aquire lock for main thread
	for (i = 0; i < 2; i++) {
		if(pthread_mutex_init(&locks[i], NULL) != 0){
			printf("Failed to initialize mutex %d\n", i+1);
			return 1;
		}
		pthread_mutex_lock(&locks[i]);
	}
	
	mark = pthread_create(&threads[1], NULL, &ReadTimeThread, &close);
	if (mark != 0) {
		printf("Failed to create thread: %s\n", strerror(mark));
		return 1;
	}
	
    //open the recently directory, fail to return 1
    openDirSuccess = getDirName("./", &dirName);
    if (openDirSuccess == -1) {
      printf("%s", dirName);
      return 1;
    }
    //read the data from the file
    read_data(dirName, roomArray, roomName);

    
    //put connection to connection index
    int n;
    for(n = 0; n < 7; n++){
        //getConnectRoomID(char **roomName, char *connectRoomName){
        int m;
        for(m = 0; m < 6; m++){
            roomArray[n].connect_roomID[m] = getConnectRoomID(roomName, roomArray[n].connect_roomName[m]);
        }
    }
    
    game(roomArray, roomName);
    
    return (0);
}

