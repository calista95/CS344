/*
 * Description: Game based on the location of California Disneyland (part 2)
 * Name: Calista Wong
 * Purpose: This program reads from the most recently created wongcal.rooms.xxxxx directory and uses files in
 * that directory to play the game. The user starts out in the start room and travels until the end room is reached.
 * A second thread can be activated to identify the current time stats if the user enters "time" instead of "<room name>" 
 * while in the game cycle. These stats will be written into a currentTime file. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/***************************DEFINITIONS**********************************/

struct Room //Struct describing the attributes of an individual room
{
        int name;
        int type; //type of room it is (start, end, mid)
        int n; //number of connections
        int array[6]; //array of at most 6 connections
};

//variables and arrays used
struct Room* rooms[7]; //array of 7 pointers to structs
struct Room* currentRoom; //pointer to the current room
bool won=false; //indicate whether the game has been won, initialized to false
int history[10000]; //pointer to array storing the history of the user's moves (not sure how large to make it since # moves is undefined) 
int iterator=0; //to iterate through history array
int steps=0; //keep track of number of steps

//array of 10 room names, with max 8 characters each (\n not included)
char room_names[10][9] =
         {
           "MainSt", //Main St, USA
           "ToonTown", //Mickey's Toon Town
           "Advntr", //Adventureland
           "Fantasy", //Fantasyland
           "NOsquare", //New Orleans Square
           "FrntrLnd", //Frontier Land
           "Tmmrwlnd", //Tomorrowland
           "Critter", //Critter County
           "StarWars", //Star Wars Land
           "CaliAdv" //California Adventure
           };
 
//array of 3 room types, with max 11 characters (\n not included)
char room_types[3][11] =
            {
              "START_ROOM",
               "MID_ROOM",
               "END_ROOM"
             };

/**********************function prototypes********************************/

//file parsing functions
static time_t getFileModifiedTime(const char *path);
void getFileData();
int getRoom(char room[100]);
int getType(char type[100]);
int getConnection(char str[100]);
//game functions
void initializeGame(); 
void resetCurrentRoom(int);
void recordRoom(char input[9]);
void displayMoves();
//time keeping functions
void *timeFunction(void *value); //this is part of the second thread

/*******************************MAIN****************************************/
int main()
{

	//create mutex for time keeping
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	//lock mutex
	pthread_mutex_lock(&mutex);
	
	currentRoom = malloc(sizeof(struct Room)); //set aside memory for currentRoom pointer

	getFileData(); //parse the file contents 
	
	initializeGame(); //set up for game
	
	while (won == false) //play the game
	{
		//user input
		bool inputGood = false; //flag to make sure that user puts in a valid input
		char input[9];
		while (!inputGood)
		{
			//display prompt
			printf("CURRENT LOCATION: %s\n", room_names[currentRoom->name]);	
			printf("POSSIBLE CONNECTIONS: ");
			for (int i=0; i<currentRoom->n-1; i++)
				printf("%s, ", room_names[ currentRoom->array[i] ]);
				printf("%s.\n", room_names[ currentRoom->array[currentRoom->n-1]]);
			printf("WHERE TO? >");
			scanf("%s", input);
		
			//second thread
			if (strcmp(input, "time") ==0)
			{
				pthread_t thread; //create time thread
				pthread_create(&thread, NULL, timeFunction, NULL);

				pthread_mutex_unlock(&mutex); //unlock the mutex
				pthread_join(thread, NULL); //join the time thread 
				pthread_mutex_lock(&mutex); //lock the mutex again
				
				inputGood = true;
				printf("WHERE TO? >");
				scanf("%s", input);
			}

			for (int i=0; i<currentRoom->n; i++)
				{
					if (strcmp( room_names[ currentRoom->array[i] ], input ) ==0  )
					inputGood = true;
				}
		
			if (inputGood)
				break;
				else
				{
					printf("\n");
					printf("HUH? I DONT'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");	
				}	
		
		}

		//see if the input is the end room
		for (int i=0; i<7; i++)
		{

			if ( strcmp(input,room_names[ rooms[i]->name ]) == 0 )
			{
				if ( rooms[i]->type == 2) //found the end room
					{
						won = true;
						printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
						break;
					}	
				else //assign input room as the new current room
					resetCurrentRoom(i); 
			}	
		}	

		recordRoom(input); //write selected room into history

		printf("\n"); //spacer
		steps++; //increment number of steps
	}
	
	displayMoves();
	
	//free memory
	for (int i=0; i<7; i++)
		free(rooms[i]);

	exit(0);
}


/*******************************************FUNCTIONS FOR FILE DATA PARSING************************************************************/
//function that get file modified time
static time_t getFileModifiedTime(const char *path)
{
        struct stat attr;
        if (stat(path, &attr) == 0)
		    {
		            //printf("%s: last modified time: %s", path, ctime(&attr.st_mtime));
		            return attr.st_mtime;
	            }
        return 0;
}


//function that opens each file of the wongcal.room.xxxxx directory and creates/fills a struct for each file
void getFileData()
{
	DIR * dir;
	struct dirent*sd;
	dir = opendir("."); //open root directory and search through it to find the file with all the Rooms

	char target[23]; //string to hold the path to the target file
	char dirName[23]; //string to hold the actual name of the target file
	int processFile=1;
	time_t t1; 
	time_t t2;

	while (	  (sd=readdir(dir)) != NULL ) //check which directory starts with wongcal.room, then create its path and save its name
	{

		if (strncmp(sd->d_name, "wongcal.room", 12) == 0) //find the most recently created wongcal.room.xxxxx file 
		{
			if (processFile == 1) //set first file so that we have a t1 value to compare to
			{
				t1 = getFileModifiedTime(sd->d_name);
				strcpy(target, "./");
				strcat(target, sd->d_name);
				strcpy(dirName, sd->d_name);
				processFile++;
			}
			else //after first file is processed
			{
				t2 = getFileModifiedTime(sd->d_name);

				if (t1<t2) //t1 is older than t2
				{
					memset(target, 0, 23);
					strcpy(target, "./");
					strcat(target, sd->d_name);
					strcpy(dirName, sd->d_name);

					//set t2 to be the new t1
					time_t temp = t1;
					t1 = t2;
					t2 = temp;
				}

			}
		}
	}

	//printf("The most recent directory is: %s\n", target);

	closedir(dir);

	dir = opendir(target); //open the path to the target directory, wongcal.room.xxxxx
	int n=0; //index for going through Room array

	while (	  (sd= readdir(dir)) != NULL )
	{
		if ( !(strcmp(sd->d_name, ".")) || !(strcmp(sd->d_name, "..")) ) {} //filter out home directory and root directory
		else //process each file
		{
			
			FILE* file;
			//set up path
			char path[100];
			strcpy(path, "./");
			strcat(path, dirName);
			strcat(path, "/");
			strcat(path, sd->d_name);
			
			file=fopen( path, "r");
	

			//allocate memory for a struct
			rooms[n] = malloc(sizeof(struct Room));
			
				char singleLine[100];
				int m=0; //to keep track of which room connection we are at
				bool typeFound = false; //to ensure we only read the "ROOM TYPE" line once for each file 
				while (!feof(file)) //fill in the struct data for each individual file
				{
					fgets(singleLine, 100, file);
				
					if (strncmp(singleLine, "ROOM NAME: ", 11) == 0) //assign the appropriate room name index 
					{
						 rooms[n]->name = getRoom(singleLine);
					}
					else if (strncmp(singleLine, "CONNECTION:",10) ==0) //assign the mth connection index
					{
						rooms[n]->array[m]=getConnection(singleLine); 
						m++;
					}
					else if (strncmp(singleLine, "ROOM TYPE:",10) ==0) //assign the appropriate room type index
					{
						if (!typeFound) //I had to add this conditional because for some reason, feof reads duplicates of the same line (not sure why)
						{
							rooms[n]->type = getType(singleLine);
							typeFound = true; //so that we only read this line ONE TIME ONLY
						}
					}
				}
			rooms[n]->n = m; //record the number of connections the room has

			n++; //increment iterator to process next file	
			
			fclose(file);
		}
	}

	closedir(dir);
}


//function that receives a string and determines which room name index it corresponds to in the room_name array
int getRoom(char room[100])
{
	if (strcmp(room, "ROOM NAME: MainSt\n")==0)
		return 0;
	else if (strcmp(room, "ROOM NAME: ToonTown\n")==0)
		return 1;
	else if (strcmp(room, "ROOM NAME: Advntr\n")==0)
		return 2;
	else if (strcmp(room, "ROOM NAME: Fantasy\n")==0)
		return 3;
	else if (strcmp(room, "ROOM NAME: NOsquare\n")==0)
		return 4;
	else if (strcmp(room, "ROOM NAME: FrntrLnd\n")==0)
		return 5;
	else if (strcmp(room, "ROOM NAME: Tmmrwlnd\n")==0)
		return 6;
	else if (strcmp(room, "ROOM NAME: Critter\n")==0)
		return 7;
	else if (strcmp(room, "ROOM NAME: StarWars\n")==0)
		return 8;
	else if (strcmp(room, "ROOM NAME: CaliAdv\n")==0)
		return 9;
	else
		return -1;
}

//function that receives a string and determines which room type index it corresponds to in the room_type array
int getType(char type[100])
{
	if (strncmp(type, "ROOM TYPE: START_ROOM", 21) ==0)
		return 0;
	else if (strncmp(type, "ROOM TYPE: MID_ROOM", 19) ==0)
		return 1;
	else if (strncmp(type, "ROOM TYPE: END_ROOM", 19) ==0)
		return 2; 
	else
		return -1;
}

//function that receives a string and determines which room type index the connection corresponds to in the room_type array
int getConnection(char str[100])
{
	int len = strlen(str);
	const char *last = &str[len-9]; //last 9 chars
	//printf("%s\n", last_four);

	if (strcmp(last, ": MainSt\n")==0)
		return 0;
	else if (strcmp(last, "ToonTown\n")==0)
		return 1;
	else if (strcmp(last, ": Advntr\n")==0)
		return 2;
	else if (strcmp(last, " Fantasy\n")==0)
		return 3;
	else if (strcmp(last, "NOsquare\n")==0)
		return 4;
	else if (strcmp(last, "FrntrLnd\n")==0)
		return 5;
	else if (strcmp(last, "Tmmrwlnd\n")==0)
		return 6;	
	else if (strcmp(last, " Critter\n")==0)
		return 7;
	else if (strcmp(last, "StarWars\n")==0)
		return 8;
	else if (strcmp(last, " CaliAdv\n")==0)
		return 9;
	else
		return -1;
}

/*****************************************************GAME FUNCTIONS******************************************************/

//function that sets up current room pointer to start room before entering the game
void initializeGame()
{

	for (int i=0; i<7; i++) //fill up pointer info
	{
		if (rooms[i]->type == 0) //look for the start room
		{
			//printf("name: %i\n", rooms[i]->name);
			//printf("type: %i\n", rooms[i]->type);
			//printf("n: %i\n", rooms[i]->n);
			currentRoom->name = rooms[i]->name;
			currentRoom->type = rooms[i]->type;
			currentRoom->n = rooms[i]->n;
			for (int j=0; j<currentRoom->n; j++)
				currentRoom->array[j] = rooms[i]->array[j];
			break;
			
		}
	}
}

//function that resets the current room to the one indicated by the integer index (ex: if n=1, reset currentRoom to rooms[1])
void resetCurrentRoom(int i)
{
	currentRoom->name = rooms[i]->name;
	currentRoom->type = rooms[i]->type;
	currentRoom->n = rooms[i]->n;
	for (int j=0; j<currentRoom->n; j++) //copy the new connections over
		currentRoom->array[j] = rooms[i]->array[j];
}

//function that records the user move
void recordRoom(char input[9])
{
	for (int i=0; i<10; i++) //search for the index of the input room in room_names[], then write that index into history[]
	{
		if ( strcmp(room_names[i], input) ==0)
		{
			history[iterator] = i;	
			iterator++;
			break;	
		}
	}
	
}

//display all the moves when the game is over
void displayMoves()
{
	printf("YOU TOOK %i STEPS. YOUR PATH TO VICTORY WAS:\n",steps);
	for (int i=0; i<iterator; i++)
		printf("%s\n", room_names[history[i]]);
}

/*************************************************Time Keeping Functions****************************************/
//function that gets the current time in the specified format, then writes this information to a file called 
//"currentTime.txt", which is located in the same directory as the game
void *timeFunction(void *value)
{
	//lock mutex
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&mutex);
	
	//do time keeping activities
	struct tm *ptr;
	time_t t;
	char str[100];
	t = time(NULL);
	ptr = localtime(&t);
	strftime(str, 100, "%I:%M %p, %A, %B %d %Y", ptr); //time info will be written in str string
	printf("\n%s\n\n", str);

	//create current time file and write string into it
	FILE*file;
	file = fopen("currentTime", "w");
	fprintf(file, str);
	fclose(file);
	
	//unlock mutex
	pthread_mutex_unlock(&mutex);
	
	return NULL;
}
