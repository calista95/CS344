/* Description: Game based on the locations of California Disneyland (part 1)
 * Name: Calista Wong
 * Purpose: This program generates 7 files using 7 out of the 10 locations named after individual theme
 * parks in Disneyland. Each file will include the name of the location, 3-6 connections to other parks,
 * and what type of park it is (start, middle, or end) relative to the game.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

struct Room //Struct describing the attributes of an individual room
{
	int name; 
	int type; //type of room it is (start, end, mid)
	int n; //number of connections
	int array[6]; //array of at most 6 connections
};

struct Room* rooms[7]; //array of 7 pointers to structs


/*************************HELPER FUNCTION FOR GETTING FILE NAMES********************/

//function to determine if room name has already been used
bool alreadyUsed(int used_rooms[7], int n)
{
	for (int i=0; i<7; i++)
	{
		if (used_rooms[i] == n)
		{
			return true;
		}
	}
	return false; //not yet used
}


/************************HELPER FUNCTIONS FOR FILLING CONNECTIONS****************/
//Return true if if all rooms have 3 to 6 outbound connections, false otherwise
bool IsGraphFull()
{
	for (int i=0; i<7; i++)
	{
		if (rooms[i]->n < 3 ) //each room should have three connections minimum
			return false;
	}
	return true;
}

//returns an index to a random Room, does not validate 	
int GetRandomRoom()
{

	return rand()%7+0; //random number between 0-6
}	

// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
bool CanAddConnectionFrom(int x) 
{
   if (rooms[x]->n < 6)
   {
	   return true;
   }
   return false;
}

// Returns true if a connection from Room x to Room y already exists, false otherwise
bool ConnectionNotExist(int x, int y)
{
	for (int i=0; i<rooms[x]->n; i++)
	{
		if (rooms[x]->array[i] == rooms[y]->name)
		{
			return false;
		}
	}
	return true;

}

// Connects Rooms x and y together, does not check if this connection is valid
void ConnectRoom(int x, int y) 
{
	//make the connection
	rooms[x]->array[rooms[x]->n] = rooms[y]->name;
	rooms[y]->array[rooms[y]->n] = rooms[x]->name;
	//increment both connection counts
	rooms[x]->n = rooms[x]->n+1;
	rooms[y]->n = rooms[y]->n+1;
}



// Returns true if Rooms x and y are the same Room, false otherwise
bool differentRoom(int x, int y) 
{
	if (rooms[x]->name == rooms[y]->name)
		return false;
	else
		return true;
}

//adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection()
{
	int roomA;
	int roomB;
	
	//assign a room integer for room A
	bool getA = false; 
	while (getA == false)
	{
		roomA = GetRandomRoom();
		if (CanAddConnectionFrom(roomA) )
		{
			getA = true;
		}
	}

	bool getB = false;

	
	while(getB == false)
	{
		roomB = GetRandomRoom();

		int var1 = CanAddConnectionFrom(roomB); //expect true, 1
		int var2 = differentRoom(roomA, roomB); //expect true, 1
		int var3 = (ConnectionNotExist(roomA,roomB)); //expect true, 1

		if (var1==1 && var2==1 && var3==1)
		{
			ConnectRoom(roomA, roomB);	
			getB = true;
			break;
		}

	}

}

/*************************************************************************/

int main()
{
	/******** Intializes random number generator ********/
	time_t t; 
        srand((unsigned) time(&t));

	/*******room file setup************/
	
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

	//array of 6 numbers
	char numbers[6][2] = {"1", "2", "3", "4", "5", "6"};

	int used_rooms[7]={-1,-1,-1,-1,-1,-1,-1}; //keep track of the rooms that we have already used
	int m=0; //keep track of which room we are naming


	/*****generate seven different files*****/
	for (int i=0; i<7; i++)
		{
			bool fileMade = false;
			while (!fileMade)
			{
				int n = rand()%9+0; //random number between 0-9
				if (alreadyUsed(used_rooms, n) == false)
					{
						
						used_rooms[m] = n; //indicate that this room is used already so we don't use it again
						fileMade = true;
						
						rooms[m] = malloc(sizeof(struct Room)); //create the struct

						//room name
						rooms[m]->name= n; 

						//initialize number of connections as zero
						rooms[m]->n = 0;

						//room type
						if (m == 0 ){ //start room
							rooms[m]->type = 0; //write to struct 
							}
						else if (m == 6){ //end room
							rooms[m]->type = 2; //write to struct
							}
						else{ //mid room
							rooms[m]->type = 1; //write to struct
							}
						m++;	

			}
					
		}
	}


	//add connections
	while (IsGraphFull() == false)
		AddRandomConnection();

	//make the directory
	char rooms_dir[] = "wongcal.rooms.";
			
	char pid_buffer[20];
	pid_t current_processID = getpid();
	sprintf(pid_buffer, "%d", current_processID); 
	strcat(rooms_dir, pid_buffer);
	int result = mkdir(rooms_dir, 0755);
	if(result)
	{
	       	//failed to create dir
		printf("%s\n", "Failed to create dir");
		return 1;
	}

	//write into file
	FILE*fileptr;
	for (int i=0; i<7; i++)
	{
		fileptr = fopen(room_names[rooms[i]->name], "w"); 
		
		//write room name	
		char roomName[20];
		strcpy(roomName, "ROOM NAME: " );
		strcat(roomName, room_names[rooms[i]->name]);
		strcat(roomName, "\n");
		fprintf(fileptr, roomName);


		//write connections
		for (int j=0; j<rooms[i]->n; j++)
			{
				char connection[25];
				strcpy(connection, "CONNECTION ");
				strcat(connection, numbers[j]);
				strcat(connection, ": ");
				strcat(connection, room_names[rooms[i]->array[j]]);
				strcat(connection, "\n");
				fprintf(fileptr, connection);
			}

		//write room type
		char roomType[22];
		strcpy(roomType, "ROOM TYPE: ");
		if (rooms[i]->type == 0)
			strcat(roomType, "START_ROOM");
		else if (rooms[i]->type == 1)
			strcat(roomType, "MID_ROOM");
		else if  (rooms[i]->type == 2)
			strcat(roomType, "END_ROOM");
		fprintf(fileptr, roomType);

		//bash command to append to Room directory
		char command[30];
		sprintf(command, "mv %s %s", room_names[rooms[i]->name], rooms_dir);
		system(command);
	}
	fclose(fileptr);

	
	//memory cleanup
	for (int i=0; i<7; i++)
		free(rooms[i]);

	return 0;
}
