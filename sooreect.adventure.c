/*************************************************************************************************
 * Author: Tida Sooreechine
 * Date: 2/13/2017
 * Program: CS344 Program 2 - Adventure, Part 2
 * Description: Program provides an interface for playing the game using the most recently 
 * 	generated room files. 
*************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>

//define constants
#define MAX_ROOMS 10
#define TOTAL_ROOMS 7
#define MIN_CONNECTIONS 3
#define MAX_CONNECTIONS 6 

//threads and mutex - took a day and a half to even begin to understand what these things are
//source: CS344 Block 2, Lecture 3
//source: https://piazza.com/class/ixhzh3rn2la6vk?cid=160 
//source: http://stackoverflow.com/questions/14888027/mutex-lock-threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;	//create and initialize mutex variable
pthread_t thread;	//declare a global thread variable

//declare room struct
struct Room {
	char name[50];							//complications with room names, had to revise to array
	int type;								//3 possible options
	int connections;						//3-6 possible connections
	int connectionList[MAX_CONNECTIONS];	//array listing all connections based on CTU 
	int updatedList[MAX_CONNECTIONS];		//array listing all connections based on Room struct index
}; 

//declare player struct
struct ctuAgent {
	int history[100];
	int steps;
	int destination;
	int done;
	int time;
};

//function prototypes
void getDirectory(char* path);
struct Room* extractFile (char* path, int num, char** ctu);
void getMap(struct Room** rooms, int arr[], char** ctu);
void updateConnections(struct Room** rooms, int arr[]);
int getStart(struct Room** rooms);
int getEnd(struct Room** rooms);
void printGameOptions(struct Room** rooms, int roomNum);
void getUserInput(struct Room** rooms, struct ctuAgent* agent);
void printCongrats(struct Room** rooms, struct ctuAgent* agent);
void* writeTime(void* arg);
void readTime();


int main() {
	//main program initiates lock on mutex before a second thread is created
	pthread_mutex_lock(&mutex);
	
	//create a second thread for writing time to file
	//second thread will gain control when the mutex is unlocked by main
	pthread_create(&thread, NULL, writeTime, NULL);

	//CTU LA building list of room names
	//index is CTU ID
	char* ctuBuilding[MAX_ROOMS];
	ctuBuilding[0] = "Bullpen";
	ctuBuilding[1] = "Situation Room";
	ctuBuilding[2] = "CTU Director Office";	
	ctuBuilding[3] = "Field Ops Office";
	ctuBuilding[4] = "Armory";
	ctuBuilding[5] = "Holding Room 3";
	ctuBuilding[6] = "Medical";
	ctuBuilding[7] = "Forensics";
	ctuBuilding[8] = "Panic Room";
	ctuBuilding[9] = "Server Room";

	//get the most recent directory to read files from
	char* folder = malloc(50);
	getDirectory(folder);		
	printf("folder name = %s\n", folder);

	//create rooms from files
	int i, j;
	struct Room *roomArray[TOTAL_ROOMS];
	for (i = 0; i < TOTAL_ROOMS; i++) { 
		roomArray[i] = extractFile(folder, i, ctuBuilding);
	}

	//update list of connections from CTU-index number to current room index	
	int map[TOTAL_ROOMS];
	getMap(roomArray, map, ctuBuilding);
	updateConnections(roomArray, map);

	//PLAY GAME
	
	//get starting and ending room index numbers 
	int start = getStart(roomArray);	
	int end = getEnd(roomArray);
	int current;

	//create and initialize Jack Bauer, CTU's most capable agent 
	struct ctuAgent *jackBauer = malloc(sizeof(struct ctuAgent));
	jackBauer->steps = 0;
	jackBauer->done = 0;
	jackBauer->time = 0;
	jackBauer->destination = end;
	jackBauer->history[jackBauer->steps] = start;
	
	//play the game
	while(!jackBauer->done) {
		if (!jackBauer->time) {
			current = jackBauer->history[jackBauer->steps];
			printGameOptions(roomArray, current);	
			getUserInput(roomArray, jackBauer);
		} else {
			getUserInput(roomArray, jackBauer);
		}
	}

	//print congratulatory message
	printCongrats(roomArray, jackBauer);
	
	free(roomArray[0]);		
	free(folder);
	
	return 0;
}


//FUNCTION DEFINITIONS

//determine the most recent directory
//source: https://linux.die.net/man/2/stat
//source: Spencer Moran's snippet of code - https://piazza.com/class/ixhzh3rn2la6vk?cid=234 
void getDirectory(char* path) {
	//get current directory's path
	char* prefix = "sooreect.rooms.";
	char curDirectory[100];
	memset(curDirectory, '\0', sizeof(curDirectory));	//clear char array string
	getcwd(curDirectory, sizeof(curDirectory));			//get current working directory

	DIR *dir;											//directory stream pointer
	struct dirent *dp;									//directory struct
	struct stat *buffer;								//stat containing file info
	time_t lastModified; 			 					//time of file last modification
	int maxTime = 0;/Users/teamtida
	int suffix = 0;
	char dirName[50];

	memset(dirName, '\0', sizeof(dirName));

	dp = malloc(sizeof(struct dirent));
	buffer = malloc(sizeof(struct stat));
	dir = opendir(curDirectory);						//open directory stream for reading
	if (dir != NULL) {									
	   	while (dp= readdir(dir)) {	
			if (strstr(dp->d_name,prefix) != NULL){		//continue if directory's name contains prefix	
				stat(dp->d_name, buffer);				
		 		lastModified = buffer->st_mtime;
				//printf("%s: %s\n", dp->d_name, ctime(&lastModified));
		  		//printf("%s: %d\n", dp->d_name, lastModified);

				//get directory's name for one most recent
		  		if (lastModified > maxTime)	{
					maxTime = lastModified;
					strcpy(dirName, dp->d_name);
				}
			}
    	}
	}

	//store current directory in variable to be used in main
	strcpy(path, dirName);	

	closedir(dir); 
	free(buffer);
	free(dp);
}

//READ FILES FROM DIRECTORY, EXTRACT DATA, AND STORE THEM IN EACH ROOM STRUCT
struct Room* extractFile(char* path, int num, char** ctu) {
	FILE *inputFile;
	char fileName[50];
	char line[100];
	char subString[100];
	int roomNum = num;
	int c, i, len, count;

	//get the most recent directory to read files from
	//get file name from main and open file
	memset(fileName, '\0', sizeof(fileName));
	snprintf(fileName, 50, "%s/room%d.txt", path, roomNum + 1);
	inputFile = fopen(fileName, "r");

	//create a new room
	struct Room *newRoom = malloc(sizeof(struct Room));
	//newRoom->id = roomNum;
	newRoom->connections = 0;
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		newRoom->connectionList[i] = -9;
	}
	
	count = 0;

	//get and store each line from file into line variable while it is not EOF
	//source: http://stackoverflow.com/questions/9206091/going-through-a-text-file-line-by-line-in-c
	while(fgets(line, sizeof(line), inputFile)) {

		//source: http://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
		strtok(line, "\n");		//remove trailing new line

		memset(subString, '\0', sizeof(subString));
		len = strlen(line);

		//get room name from file
		//if the 6th character of the line is N from name, then get the name substring
		//source: CS344 Block 2, Lecture 1
		if (line[5] == 'N') {
			c = 0;
			while (c < len) {
				subString[c] = line[11 + c]; 
				c++;
			}
			subString[c] = '\0';
			strtok(subString, "\n");	
			strcpy(newRoom->name, subString);
	}
		//get room type and store value in integer
		//if the 6th character of the line is T from type, 
		//get the 12th character of the line and perform assignment
		if (line[5] == 'T') {
			if (line[11] == 'S') 
				newRoom->type = 0;	
			else if (line[11] == 'E') 
				newRoom->type = 2;
			else 			
				newRoom->type = 1;
		}
		//otherwise, get quantity and list of neighboring connections
		//if the line starts with C for connection
		//then save connections and update count 
		else if (line[0] == 'C') {
			c = 0;
			while (c < len) {
				subString[c] = line[14 + c];
				c++;
			}
			subString[c] = '\0';
			
			for (i = 0; i < MAX_ROOMS; i++) {
				if (strcmp(subString, ctu[i]) == 0)
					newRoom->connectionList[count] = i;
					//note-this is the CTU index that's being stored,
					//not the struct index
			}
			count++;
		}
	}
	newRoom->connections = count;

	fclose(inputFile);
	return newRoom;
}

//GET THE CTU INDEX NUMBER FOR EACH ROOM STRUCT IN THE ARRAY OF ROOMS
//this will be used to update the connections from CTU index to the rooms array index
void getMap(struct Room** rooms, int arr[], char** ctu) {
	int i, j;
	for (i = 0; i < TOTAL_ROOMS; i++){
		for (j = 0; j < MAX_ROOMS; j++) {
			if(strcmp(rooms[i]->name, ctu[j]) == 0) {
				arr[i] = j;
			}
		}
	}
}

//TRANSLATE THE CONNECTIONS FROM CTU INDEX NUMBER TO CURRENT ROOMS STRUCT INDEX NUMBER
//store in a separate array called updatedList
void updateConnections(struct Room** rooms, int arr[]) {
	int i, j, k;
	int numConnections;
	for (i = 0; i < TOTAL_ROOMS; i++) {
		numConnections = rooms[i]->connections;
		for (j = 0; j < numConnections; j++) {
			for (k = 0; k < TOTAL_ROOMS; k++) {
				if (rooms[i]->connectionList[j] == arr[k])
					rooms[i]->updatedList[j] = k;
			}
		}
	}
}

//return index of start room
int getStart(struct Room** rooms) {
	int i, result;
	for (i = 0; i < TOTAL_ROOMS; i++) {
		if (rooms[i]->type == 0)
			result = i;	
	}
	return result;
}

//return index of end room
int getEnd(struct Room** rooms) {
	int i, result;
	for (i = 0; i < TOTAL_ROOMS; i++) {
		if (rooms[i]->type == 2)
			result = i;	
	}
	return result;
}

//print to screen player's current location and adjacent rooms
void printGameOptions(struct Room** rooms, int roomNum){
	int i;
	int outs = rooms[roomNum]->connections;
	int neighbor;

	printf("CURRENT LOCATION: %s\n", rooms[roomNum]->name);
	
	printf("POSSIBLE CONNECTIONS: ");
	for (i = 0; i < outs; i++) {
		neighbor = rooms[roomNum]->updatedList[i];
		if (i != outs - 1) 
			printf("%s, ", rooms[neighbor]->name);
		else 
			printf("%s.", rooms[neighbor]->name);	 
	}
	printf("\n");
}

void getUserInput(struct Room** rooms, struct ctuAgent* agent) {
	int count, curLocation, i;
	int nextRoom = -5;
	int maxInputSize = 256;
	char* input = malloc(maxInputSize);
	int acceptableInput = 0;

	agent->time = 0;

	//print prompt to user
	printf("WHERE TO? >");

	//get user's response from stdin and store in variable input
	//source: http://stackoverflow.com/questions/1247989/how-do-you-allow-spaces-to-be-entered-using-scanf
	fgets(input, maxInputSize, stdin);
	if((strlen(input)>0) && (input[strlen(input) - 1] == '\n'))
		input[strlen(input) - 1] = '\0';

	//if user requests for time
	if ((strcmp(input, "time") == 0)) {
		pthread_mutex_unlock(&mutex);	//main unlocks mutex, allowing writeTime thread to run its thing

		//join threads after time has been written to file
		//second thread is terminated
		pthread_join(thread, NULL); 	

		pthread_mutex_lock(&mutex);	//main secures mutex again

		//create a new second thread for writing time in case user requests for time again
		pthread_create(&thread, NULL, writeTime, NULL);	

		readTime();

		agent->time = 1;
		free(input);
	}
	else {	

		//compare input string to all the room names
		//if there's a match, save request room index
		for (i = 0; i < TOTAL_ROOMS; i++) {
			if (strcmp(rooms[i]->name, input) == 0)
				nextRoom = i;
		}

		//check all the current location's connections and see if requested room is accessible
		curLocation = agent->history[agent->steps];
		for (i = 0; i < rooms[curLocation]->connections; i++) {
			if (rooms[curLocation]->updatedList[i] == nextRoom) {
				acceptableInput = 1;
			}
		}

		//print error message if room is not an existing connection 	
		//otherwise, proceed to that room by adding to history list and incrementing step count
		//if room is also the end room, then switch agent's done flag to done
		if (!acceptableInput) 
			printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
		else {
			printf("\n");
			agent->steps++;
			agent->history[agent->steps] = nextRoom;
	
			if (nextRoom == agent->destination)
				agent->done = 1;
		}

		free(input);
	}
}

//print end message to user and list number of steps and path to victory
void printCongrats(struct Room** rooms, struct ctuAgent* agent) {
	int i, j;
	//Jack Bauer saves the day again
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS\n", agent->steps);
	for (i = 1; i <= agent->steps; i++) {
		for (j = 0; j < TOTAL_ROOMS; j++) {
			if (agent->history[i] == j) 
				printf("%s\n", rooms[j]->name);  
		}
	}
}

//get and print current time to a file
//source: http://www.cplusplus.com/reference/ctime/localtime/
//source: http://www.cplusplus.com/reference/ctime/strftime/
//source: http://stackoverflow.com/questions/4295754/how-to-remove-first-character-from-c-string
void* writeTime(void* arg) {
	//second thread secures mutex
	pthread_mutex_lock(&mutex);

	time_t rawTime;
	struct tm* timeInfo;
	char buffer[80];	
	FILE *outputFile;
	char *filePath = "currentTime.txt";

	//set permission to write, so that it writes over old time every time time is requested
	outputFile = fopen(filePath, "w");

	//get local time
	time (&rawTime);
	timeInfo = localtime(&rawTime);

	//reformat how time is displayed
	strftime(buffer, 80, "%I:%M%p, %A, %B %d, %Y", timeInfo);

	if (buffer[5] == 'A')
		buffer[5] = 'a';
	else
		buffer[5] = 'p';
	buffer[6] = 'm';	

	if ((timeInfo->tm_hour < 10) || (timeInfo->tm_hour > 12 && timeInfo->tm_hour < 22))
		memmove(buffer, buffer + 1, strlen(buffer));

	//printf("\n%s\n", buffer);
	
	//print formatted time to file
	fprintf(outputFile, "%s", buffer);

	fclose(outputFile);
	
	//second thread unlocks mutex, releasing it back to the main
	pthread_mutex_unlock(&mutex);
}

//read time from file in the current working directory and print to screen
void readTime() {
	FILE *inputFile;
	char *filePath = "currentTime.txt";
	char line[1000];

	inputFile = fopen(filePath, "r");

	if (inputFile != NULL) {
		if (fgets(line, sizeof(line), inputFile) != NULL)
			fprintf(stdout, "\n%s\n", line);	
	}
	
	fclose(inputFile);	 
}
