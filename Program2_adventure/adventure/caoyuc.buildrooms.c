//
//  caoyuc.buildrooms.c
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




typedef struct  rooms{
    int roomID;                //from 0 to 6, make it easy to run 
    int connect_roomID[6];     //the connect room ID
    int roomTypeID;            // 1 for start room, 2 for mid room, 3 for end room
}               Room;


//creat the dir 
int get_directory()
{
    int processID = getpid();  //pid_t use as int type
    char filename[50];
    sprintf(filename, "caoyuc.rooms.%d", processID);
    if(mkdir("filename", 0777) == -1)
    {
        perror("mkdir error");
        return (-1);
    }
    return (0);
}





// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
int IsGraphFull(Room* roomArray)
{
    int i;
    for(i = 0; i < 7; i++){
        int j;
        for(j = 0; j < 3; j++){
            if((roomArray + i)->connect_roomID[j] == -1){
                return (0);
            }
        }
    }
    return (1); //false, not full
}





// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
int CanAddConnectionFrom(Room* x)  //address
{
    int i;
    for (i = 0; i < 6; i++){
        if(x->connect_roomID[i] == -1)
        {
            
            return (1);
        }
    }
          
    return (0); //can't  add connection
}



// Returns true if a connection from Room x to Room y already exists, false otherwise
int ConnectionAlreadyExists(Room *x, Room *y)
{
    
    int i;
    int index_y;
    i = 0;
    index_y = y->roomID;
   
    for(i = 0; i < 6; i++){
        if(x->connect_roomID[i] == index_y){
            return (1);
        }
    }
   
    return (0);
}




// Connects Rooms x and y together, does not check if this connection is valid
void ConnectRoom(Room *x, Room *y)
{
  
    int i;
    int index_y;
    index_y = y->roomID;
  for(i = 0; i < 6; i++){
      if(x->connect_roomID[i] == -1){
          x->connect_roomID[i] = index_y;
          return;
      }
  }
}





// Adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection(Room *roomArray)
{
    int a;
    int b;
    
    srand(time(NULL));
    a = rand() % 7;  //get a room

    while(CanAddConnectionFrom(roomArray + a) == 0){
        a = rand() % 7;
    }
    do{
        b = rand() % 7;  //if room a and b can't connect, choose other b
    }while( a == b ||(CanAddConnectionFrom(roomArray + b) == 0 ||  ConnectionAlreadyExists(roomArray + a, roomArray + b) == 1));
 
  ConnectRoom(roomArray +a, roomArray + b);  // TODO: Add this connection to the real variables,
  ConnectRoom(roomArray + b, roomArray +a);  //  because this A and B will be destroyed when this function terminates
}



 //random choose 7 names from 10
void choose_name(char **roomName){
    char* name[] = {"Crowther", "Dungeon", "PLUGH", "PLOVER", "twisty", "XYZZY", "Zork", "Maek", "Zurk", "Lee"}; // 10 names of hard code
    int a1;
    int a2;
    int a3;
    
	//get the random num everytime different
	srand(time(NULL));
    a1 = rand() % 10;
    a2 = rand() % 10;
    while(a1 == a2){
        a2 = rand() % 10;
    }
    a3 = rand() % 10;
    while(a1 == a3 || a2 == a3){
         a3 = rand() % 10;
    }
    
	
	int i;
	int index;
	index = 0;
	for(i = 0; i < 10; i++){
		if(i == a1){
			continue;
		}
		if (i == a2){
			continue;
		}
		if (i == a3){
			continue;
		}
		strcpy(roomName[index], name[i]);
		index++;
	}
}


int main()
{
    char filename[50];

    char **roomName;
    char* typeName[] = {"START_ROOM", "MID_ROOM", "END_ROOM"};
    char* myRoomFile[50];  //the file name for the room
    
    //random choose 7 names from 10
	roomName = (char**)malloc(sizeof(char*) * 7);
	int r;
	int s;
	for(r = 0; r < 7; r++){
		roomName[r] = (char*)malloc(sizeof(char) * 10);
		for(s = 0; s < 10; s++){
			roomName[r][s] = '\0';
		}
	}
	choose_name(roomName);
    
    
    //get directory
       int processID = getpid();  //pid_t use as int type
       sprintf(filename, "caoyuc.rooms.%d", processID);
       if(mkdir(filename, 0777) == -1)  //why 0777?
       {
           perror("mkdir error");
           return (-1);
       }
    
    //initialize the roomarray
    Room roomArray[7];
    int a;
    for(a = 0; a < 7; a++)   //c99??? change later
    {
        //roomArray[i]=malloc( sizeof(Room));
        roomArray[a].roomID = a;
       memset(roomArray[a].connect_roomID, -1, sizeof(roomArray[a].connect_roomID)); //initialize the connect room to -1
         roomArray[a].roomTypeID = 0;
    }
    
    
    //choose the room type
    //start_room
	srand(time(NULL));
    int startRoom = rand() % 7;
    roomArray[startRoom].roomTypeID = 1;
    //end room
    int endRoom = rand() % 7;
    while (startRoom == endRoom){
        endRoom = rand() % 7;
    }
    roomArray[endRoom].roomTypeID = 3;
    //mid room
    int b;
    for (b = 0; b < 7; b++){
        if(roomArray[b].roomTypeID == 0){
            roomArray[b].roomTypeID = 2;
        }
    }
    
    // Create all connections in graph
    while (IsGraphFull(roomArray) == 0 )  //address
    {
      AddRandomConnection(roomArray);
    }
    
    
     chdir(filename);   //open the folder which we got before
    int i;
     for(i = 0; i < 7; i++){
         memset(myRoomFile, 0, 50);
         sprintf(myRoomFile, "%s_room", roomName[i]);
       
         //creat the file
		 FILE *fin;
		 fin = fopen(myRoomFile,"w+");
         //put data in the file
         fprintf(fin, "ROOM NAME: %s\n", roomName[roomArray[i].roomID]);
         int j;
            for(j = 0; j < 6; j++){
                    if(roomArray[i].connect_roomID[j] != -1){
                        fprintf(fin, "CONNECTION %d: %s\n", j+1, roomName[roomArray[i].connect_roomID[j]]);
                    }
                }
                fprintf(fin, "ROOM TYPE: %s\n", typeName[roomArray[i].roomTypeID -1]);
         fclose(fin);
     }
     

    
    
    return (0);
}

