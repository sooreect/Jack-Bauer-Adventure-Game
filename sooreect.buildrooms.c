/*************************************************************************************************
 * Author: Tida Sooreechine
 * Date: 2/12/2017
 * Program: CS344 Program 2 - Adventure, Part 1
 * Description: Program creates a directory of files that hold descriptions of the rooms at the
 * 	Counter Terrorist Unit (CTU) Los Angeles office, located in West Los Angeles, California. 
 * 	Each file lists how the individual rooms are connected. 
*************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

//define constants
#define MAX_ROOMS 10
#define TOTAL_ROOMS 7
#define MIN_CONNECTIONS 3
#define MAX_CONNECTIONS 6 

//declare room struct
struct Room {
	char* name;								//10 possible options
	int type;								//3 possible options
	int connections;						//3-6 possible connections
	int connectionList[MAX_CONNECTIONS];	//array listing all connections
}; 

//FUNCTION PROTOTYPES
char* createDir();
void createRoomFiles(char* directory);


//MAIN PROGRAM
int main() {
	srand(time(NULL));

	char* dirPath = createDir();
	createRoomFiles(dirPath);

	free(dirPath);

	return 0;
}


//FUNCTION DEFINITIONS

//create directory for files
char* createDir() {
	//define directory path prefix
	char* prefix = "sooreect.rooms.";
	
	//get directory path suffix, which is the process id of the calling process
	int pid = getpid();
	
	//concatenate the suffix and postfix into one string 
	//source: http://stackoverflow.com/questions/7315936/which-of-sprintf-snprintf-is-more-secure
	char* directory = malloc(50);
	snprintf(directory, 50, "%s%d", prefix, pid);

	//create directory with full permissions to owner and read and executable for everyone else
	//source: http://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
	mkdir(directory, 0755); 
	
	return directory;
}

//generate rooms
void createRoomFiles(char* directory) {
	//CTU LA building list of room names
	char* ctuRooms[MAX_ROOMS];
	ctuRooms[0] = "Bullpen";	
	ctuRooms[1] = "Situation Room";	
	ctuRooms[2] = "CTU Director Office";	
	ctuRooms[3] = "Field Ops Office";	
	ctuRooms[4] = "Armory";	
	ctuRooms[5] = "Holding Room 3";	
	ctuRooms[6] = "Medical";	
	ctuRooms[7] = "Forensics";	
	ctuRooms[8] = "Panic Room";	
	ctuRooms[9] = "Server Room";	
	
	//assign each room a unique random number, which will be mapped to the ctuRooms array
	int random, unique, i, j;
	int nameList[TOTAL_ROOMS];

	for (i = 0; i < TOTAL_ROOMS; i++) {
		nameList[i] = -1;							//clear all room assignments
	}

	for (i = 0; i < TOTAL_ROOMS; i++) {
		unique = 0;									//set unique flag to false to enter while loop
		while (!unique)	{							//repeat while output random is not unique	 
			unique = 1;								//toggle unique flag to true
			random = rand() % MAX_ROOMS;			//generate random number
			
			for (j = 0; j < TOTAL_ROOMS; j++) {		//check if output has already been assigned	
				if (nameList[j] == random)			//set unique flag to false if any element matches
					unique = 0;						
			}	

			if (unique)								//if number is unique, add to room assignments
				nameList[i] = random;
		}
	}

	//assign 2 rooms to be a start room and an end room
	//the unassigned rooms will be considered mid rooms
	int num, start, end;
	int typeList[2];

	for (i = 0; i < 2; i++) {						//clear all room assignments
		typeList[i] = -1;
	}  
	
	for (i = 0; i < 2; i++) {
		unique = 0; 
		while (!unique)	{
			unique = 1; 
			random = rand() % TOTAL_ROOMS;
			
			for (j = 0; j < 2; j++) {
				if (typeList[j] == random)
					unique = 0;
			}

			if (unique)								//only need to assign 2 elements
				typeList[i] = random;				//first element is start, 2nd element is end
		}
	}

	//create and initialize rooms
	struct Room rooms[TOTAL_ROOMS];

	for (i = 0; i < TOTAL_ROOMS; i++) {
		num = nameList[i];
		start = typeList[0];
		end = typeList[1];

		//define room name
		rooms[i].name = ctuRooms[num];			 
	
		//define room type	
		if (i == start)						
			rooms[i].type = 0;
		else if (i == end)
			rooms[i].type = 2;
		else
			rooms[i].type = 1;  

		//all rooms are initialized with 0 connections
		rooms[i].connections = 0;
		for (j = 0; j < MAX_CONNECTIONS; j++) {
			rooms[i].connectionList[j] = -10;
		}
	}	

	//connect the rooms together, while maintaining the minimum and maximum required number of connections
	//if room A is connected to room B, then room B must be connected to room A
	//room A cannot be connected to itself
	int numConnections, neighbor, nextIndex, n;

	//for each room, determine a preliminary number of connections 
	//while the current quantity of connections is less than the preliminary number
	//find a unique room number that is not itself and add to its connections
	//also add itself to the now-neighbor's list of connections
	for (i = 0; i < TOTAL_ROOMS; i++) {
		numConnections = rand() % (MAX_CONNECTIONS + 1 - MIN_CONNECTIONS) + MIN_CONNECTIONS;

		while (rooms[i].connections < numConnections){
			//get a number indicating neighbor's room
			//ensure that the connection to be added is a unique number and is not itself
			unique = 0;
			while (!unique) {
				unique = 1;
				neighbor = rand() % TOTAL_ROOMS;
				while (neighbor == i) {
					neighbor = rand() % TOTAL_ROOMS;
				}
				for (j = 0; j < rooms[i].connections; j++) {
					if (rooms[i].connectionList[j] == neighbor)
						unique = 0;
				}

				//proceed to list the new connections on both lists
				//update the total quantity of connections on both rooms
				if (unique) {
					if (rooms[neighbor].connections < MAX_CONNECTIONS) {
						if (rooms[i].connections == 0)
							rooms[i].connectionList[0] = neighbor;
						else {
							nextIndex = rooms[i].connections;
							rooms[i].connectionList[nextIndex] = neighbor;
						}
						if (rooms[neighbor].connections == 0)
							rooms[neighbor].connectionList[0] = i;
						else {
							nextIndex = rooms[neighbor].connections;
							rooms[neighbor].connectionList[nextIndex] = i;
						}
						rooms[i].connections++;
						rooms[neighbor].connections++;
					}
				}
			}
		}
	}		

	//now that all the room structs have been created and filled in with correct information
	//print room details to individual files in the directory created
	int roomNum;
	FILE *outputFile;
	char *filePath;

	for (i = 0; i < TOTAL_ROOMS; i++)
	{
		filePath = malloc(50);
		snprintf(filePath, 50, "%s/room%d.txt", directory, i+1);

		//open file with append
		//source: http://stackoverflow.com/questions/19429138/append-to-the-end-of-a-file-in-c	
		outputFile = fopen(filePath, "a");
	
		fprintf(outputFile, "ROOM NAME: %s\n", rooms[i].name);
		//printf("\nROOM NAME: %s\n", rooms[i].name);

		for (j = 0; j < rooms[i].connections; j++) {
			roomNum = rooms[i].connectionList[j];
			fprintf(outputFile, "CONNECTION %d: %s\n", j+1, rooms[roomNum].name);
			//printf("CONNECTION %d: %s\n", j+1, rooms[roomNum].name);
		}
	
		if (rooms[i].type == 0) {
			fprintf(outputFile, "ROOM TYPE: START_ROOM");
			//printf("ROOM TYPE: START_ROOM");
		} 
		else if (rooms[i].type == 1) {
			fprintf(outputFile, "ROOM TYPE: MID_ROOM");
			//printf("ROOM TYPE: MID_ROOM");
		} 
		else { 
			fprintf(outputFile, "ROOM TYPE: END_ROOM");	
			//printf("ROOM TYPE: END_ROOM");	
		}
		fprintf(outputFile, "\n\n");
		//printf("\n\n");
	
		fclose(outputFile);
	}
	free(filePath);
}

	
