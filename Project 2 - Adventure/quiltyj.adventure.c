#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>



/*=========================================================
                      Global Vars/Structs
=========================================================*/
//These are used to hold directories.
char directoryHolder[100];
char* globalCWD[100];

//Potential room types
char* potentialTypes[3] =
{
	"START_ROOM",
	"MID_ROOM",
	"END_ROOM"
};

struct room
{
	char* roomName;
	char* roomType;
	int numberOfConnections;
  char* connectionList[6];
};

//Global array to hold the rooms.
struct room* roomArray[7];

/*=========================================================
                      Function Prototypes
=========================================================*/

char* getMostRecentDirectory();
int findRoomPosition(char* passedName);
int getStartRoom();
void createRooms();
void freeMemory();
void gameLoop();
void getNames();
void makealphanumeric(char *str);
void mallocRooms();
void printRoomLayout();
void stripDisplayNames();
void writeTimeFile(void* mutex);

/*=========================================================
                      Functions
=========================================================*/
int main()
{
	//Zero out the global vars and get the cwd for timefile
  memset(directoryHolder, '\0', sizeof(directoryHolder));
	memset(globalCWD, '\0', sizeof(globalCWD));
	getcwd(globalCWD, sizeof(globalCWD));
	strcat(globalCWD, "/currentTime.txt");


	//Allocate memory.
  mallocRooms();

	//Make the rooms from the files.
  createRooms();

	//Strip out the _room suffixes.
  stripDisplayNames();

	//This function exists for testing.
	//printRoomLayout();

	//Start the actual game
	gameLoop();

	//Free up the memory
	freeMemory();

	//Return 0 per instructions
  return 0;
}

/*
	This function will control the mutex that controls writing the file. It'll
	take in an existing mutex, get the current directory, and write the time
	to the fime file. Every time it is invoked, it will overwrite the existing
	file.
*/
void writeTimeFile(void* mutex)
{
	//Buffer to hold the time and directories, then cleared out via memset.
	char timeBuffer[50];
	char workingDirectoryBuffer[50];
	memset(timeBuffer, '\0', sizeof(timeBuffer));
	memset(workingDirectoryBuffer, '\0', sizeof(workingDirectoryBuffer));

	/*
    Getting the time. I consulted the following:
	https://www.tutorialspoint.com/c_standard_library/time_h.htm
	https://en.wikibooks.org/wiki/C_Programming/time.h/time_t
    */
	time_t currentTime = time(NULL);
	struct tm* transformTime = localtime(&currentTime);

	//Using https://linux.die.net/man/3/strftime per the instructions.
	strftime(timeBuffer, 50, "%l:%M %p, %A, %B %d, %Y", transformTime);
	printf("%s\n\n", timeBuffer);

	//Now we actually set the mutex and write the data to a file.
	pthread_mutex_lock(mutex);

	//Declare the file
	FILE* fp = fopen(globalCWD, "w");

	//Null check
	if (fp == NULL)
	{
		printf("write error.\n");
		return;
	}

	//Write contents to file
	else
	{
		fprintf(fp, "%s\n", timeBuffer);
		fclose(fp);
	}

	//Unlock the mutex
	pthread_mutex_unlock(mutex);
}
/*
	This will iterate through the rooms until it gets the start.
	Nothing special, just pure linear search until it gets a hit.
*/
int getStartRoom()
{
  int i = 0;

  for (i = 0; i < 7; i++)
  {
    if (strstr(roomArray[i]->roomType, "START") != NULL)
    {
      return i;
    }
  }
}

/*
	This function will read in the rooms folders in the pwd, then return which one
	is the most recent.
*/
char* getMostRecentDirectory()
{
	/*
		This is taken from the starter code given to us in the 2.4
		readings: https://oregonstate.instructure.com/courses/1738958/pages/2-dot-4-manipulating-directories
	*/

	{
        int newestDirTime = -1; // Modified timestamp of newest subdir examined
      	char targetDirPrefix[32] = "quiltyj.rooms."; // Prefix we're looking for
      	char newestDirName[256]; // Holds the name of the newest dir that contains prefix
		char directoryBuffer[100];
      	memset(newestDirName, '\0', sizeof(newestDirName));
		memset(directoryBuffer, '\0', sizeof(directoryBuffer));

		getcwd(directoryBuffer, sizeof(directoryBuffer));

      	DIR* dirToCheck; // Holds the directory we're starting in
      	struct dirent *fileInDir; // Holds the current subdir of the starting dir
      	struct stat dirAttributes; // Holds information we've gained about subdir

      	dirToCheck = opendir("."); // Open up the directory this program was run in

      	if (dirToCheck > 0) // Make sure the current directory could be opened
      	{
        	while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
        	{
          	if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
          	{
            	//printf("Found the prefex: %s\n", fileInDir->d_name);
            	stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry
    
            	if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
            	{
                  	newestDirTime = (int)dirAttributes.st_mtime;
                  	memset(newestDirName, '\0', sizeof(newestDirName));
                  	strcpy(newestDirName, fileInDir->d_name);
            	}
          	}
    	}
  	}

  	closedir(dirToCheck); // Close the directory we opened

		/*
			Now we move the data into the character array used as
			a parameter.
		*/

	strcpy(directoryHolder, newestDirName);
	return directoryBuffer;
	    }
}


/*
	This function will read in the names of the rooms from the directory that
	has already been determined to be the latest. It assumes that getMostRecentDirectory
	has already been run.

	I modified some of the 2.4 starter code for learning the dirent, DIR, etc
	structs.
*/
void getNames()
{
  DIR* currentDirectory;
  struct dirent *direntName;
  int counter = 0;

	/*
		This just iterates through the files and copies the names. It does so based
		on the file name, not the field within the text file itself. I did this because
		I had some trouble in getting strtok to cooperate with me on getting
		all three of the different types of fields. I decided since it worked for two,
		and the names were already in the filename, I can just get them from the filename.
	*/
  if ((currentDirectory = opendir(directoryHolder)) != NULL)
  {
    while ((direntName = readdir(currentDirectory)) != NULL)
    {
			//This will pass over any nonsense files that may be there and copy the name.
      if (strlen(direntName->d_name) > 5)
      {
        roomArray[counter]->roomName = direntName->d_name;
        counter++;
      }
    }
  }
}

/*
	I had a problem with reading from the files having non-alphanumeric characters
	that could cause problems. Anything less than the full character array was having
	newlines on printing. I googled "c function strip non alphanumeric characters"
	to see if there was anything in stdio, string.h, etc for it, and this was the
	second result: https://gist.github.com/Morse-Code/5310046

	This is a single function to just make an alteration on the string, so it
	should be okay.
*/

void makealphanumeric(char *str)
{
    unsigned long i = 0;
    unsigned long j = 0;
    char c;

    while ((c = str[i++]) != '\0')
    {
        if (isalnum(c))
        {
            str[j++] = c;
        }
    }
    str[j] = '\0';
}

/*
	This large function does the heavy lifting of reading in the files and re-creating
	the room array struct with the attributes specified in the files.
*/
void createRooms()
{
	//Making vars and memsetting them.
  char pwdBuffer[200];
  char name[200];
  char roomBuffer[200];
  char newName[200];
  memset(pwdBuffer, '\0', sizeof(pwdBuffer));
  memset(name, '\0', sizeof(name));
  memset(newName, '\0', sizeof(newName));
  memset(roomBuffer, '\0', sizeof(roomBuffer));

	//Getting the most recent directory and copying that to the buffer.
  getMostRecentDirectory();
  strcpy(pwdBuffer, directoryHolder);

	//Getting the files ready to be opened and parsed.
  FILE* fp;
  int counter = 0;
  getNames();

	//I consulted https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/
	//on how to change a directory, since it has to look in a subdirectory.
  chdir(pwdBuffer);

	//This while loop will loop over the files.
	while (counter < 7)
  {
		//Reading in the files
    fp = fopen(roomArray[counter]->roomName, "r");

		//Clearing buffers
    memset(pwdBuffer, '\0', sizeof(pwdBuffer));
    memset(name, '\0', sizeof(name));

		//Another counter
		int j = 0;

		//I watched this video: https://www.youtube.com/watch?v=cWdsIDLXBJA&t=237s
		//It helped me with going over fgets.
    while (fgets(pwdBuffer, sizeof(pwdBuffer), fp) != NULL)
    {
			//There's two different buffers here, this is due to issues I had with
			//strtrok, so I figured I could just do two operations on different strings.
			char tempConnection[25];
			memset(tempConnection, '\0', sizeof(tempConnection));
			strcpy(tempConnection, pwdBuffer);
      char* token = strtok(pwdBuffer, ": ");
      strcpy(roomBuffer, strtok(NULL, " "));
      token = strtok(NULL, " ");

      strcpy(newName, token);


			//If it gets a connection, it will copy the data over. Another temp
			//string is made to run memcpy on to get rid of the non alphanumeric
			//chars. Then it adds the connection and increments the connection counter.
			if (strstr(tempConnection, "CONN") != NULL)
			{
				char* nameHolder[8];
				memset(nameHolder, '\0', sizeof(nameHolder));
				memcpy(nameHolder, &tempConnection[13], 8);
				makealphanumeric(nameHolder);
				strcpy(roomArray[counter]->connectionList[j], nameHolder);
				roomArray[counter]->numberOfConnections += 1;
				j++;
			}

			//This just uses strstr to determine the room type. I put mid first
			//since it will be the most common and would cut down on checks, with all
			//but two of the files requiring no extra checks.
      else if (strstr(pwdBuffer, "ROOM") != NULL)
      {
        if (strstr(newName, "MID") != NULL)
        {
          roomArray[counter]->roomType = potentialTypes[1];
        }

        else if(strstr(newName, "START") != NULL)
        {
          strcpy(roomArray[counter]->roomType, "START_ROOM");
        }

        else if(strstr(newName, "END"))
        {
          strcpy(roomArray[counter]->roomType, "END_ROOM");
        }

      }

		}

		//Close out the file, increment our counter.
		fclose(fp);
    counter++;
	}
}

/*
	This will allocate all the memory for the rooms. Nothing special, largely
	just a rework of the same thing in the buildrooms.
*/
void mallocRooms()
{
  int i;
  for (i = 0; i < 8; i++)
  {
    /*
    Allocate memory for each room and attirbutes.
    I'm going to give all of them memory for six connections for
    simplicity even if they can have as few as three.
    The name and type have sizes based on directions.
    */
    roomArray[i] = malloc(sizeof(struct room));;
    roomArray[i]->roomName = malloc(sizeof(char) * 8);
    roomArray[i]->roomType = malloc(sizeof(char) * 11);
    roomArray[i]->numberOfConnections = malloc(sizeof(int));
    roomArray[i]->numberOfConnections = 0;

    int j;
    for (j = 0; j < 6; j++)
    {
        roomArray[i]->connectionList[j] = malloc(sizeof(char) * 6);
        memset(roomArray[i]->connectionList[j], '\0', sizeof(roomArray[i]->connectionList[j]));
    }
  }
}

/*
  This function gets rid of the _room suffixes on the end of the room names.
  I had it going in the function that makes the name, but then that means it
  can't read from the actual file because it ends in _room and we don't want
	that as the displayed name. So I use this to strip them out after the data
	has been read in, and it displays correctly.

	I just used strtok to go through it and get rid of anything past the underscore.
*/
void stripDisplayNames()
{
  char* strippedName[20];
  memset(strippedName, '\0', sizeof(strippedName));
	int i;

  for (i = 0; i < 7; i++)
  {
  	strcpy(strippedName, roomArray[i]->roomName);
  	char* token = strtok(strippedName, "_");
  	strcpy(roomArray[i]->roomName, strippedName);
  }
}

/*
	A function purely for testing, this will print out the entire room layout
	and the connections. Nothing special, it just iterates through the room list.
*/
void printRoomLayout()
{
	int i;
	for (i = 0; i < 7; i++)
	{
		printf("Name: %s\n", roomArray[i]->roomName);
		printf("Type: %s\n", roomArray[i]->roomType);
		int j;

		for (j = 0; j < roomArray[i]->numberOfConnections; j++)
		{
			if (roomArray[i]->connectionList[j] != NULL)
			{
				printf("Connection %d: %s\n", j+1, roomArray[i]->connectionList[j]);
			}
		}

		printf("Number of Connections: %d\n", roomArray[i]->numberOfConnections);
		printf("\n");
	}
}

/*
	Just deallocates memory
*/
void freeMemory()
{
	int i;
	int j;
	for (i = 0; i < 7; i++)
	{
		for (j = 0; j < roomArray[i]->numberOfConnections; j++)
		{
			free(roomArray[i]->connectionList[j]);
		}

		free(roomArray[i]);

	}
}

/*
	This will just iterate through the roomArray and return what position
	a given name matches up to.
*/
int findRoomPosition(char* passedName)
{
	int i;
	for (i = 0; i < 7; i++)
	{
		if (strstr(roomArray[i]->roomName, passedName) != NULL)
		{
			return i;
		}
	}
}

//The main loop of the game. Details in the function itself.
void gameLoop()
{
	char inputArray[50]; //Array for user input
	int currentPosition; //Holds our current position
	int chosenPath[1000]; //Holds the path you took, 1000 ought to be enough.
	int counter;
	int stepCounter = 0; //Holds your number of steps taken.
	int isFinished = 0; //Var that holds the win condition.
	int isValidInput; //Var for acting as a bool to see if input is valid
	memset(chosenPath, '/0', sizeof(chosenPath));
	memset(inputArray, '/0', sizeof(inputArray));

	//Following suggestions of directions, I am declaring the mutex in the beginning of main
	//_t and _destroy taken from 2.3 lecture at 32:20
	pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

	//Setting up the path
	chosenPath[stepCounter] = getStartRoom();

	do
	{

		currentPosition = chosenPath[stepCounter]; //Update position
		counter = 0;
		isValidInput = 0; //Acts as a bool, if never changed, it will loop and prompt for valid input

		printf("CURRENT LOCATION: %s\n", roomArray[currentPosition]->roomName);
		printf("POSSIBLE CONNECTIONS:");

		//Printing connections
		while(counter < (roomArray[currentPosition]->numberOfConnections-1))
		{
			printf(" %s, ", roomArray[currentPosition]->connectionList[counter]);
			counter++;
		}

		//One additional print because the formatting must be exact and there's a period in the example.
		printf(" %s.\n", roomArray[currentPosition]->connectionList[counter]);

		//Clear the input buffer per suggestions.
		memset(inputArray, '\0', sizeof(inputArray));

		//Get input
		printf("WHERE TO? >");
		scanf("%50s", inputArray);
		printf("\n");

		//Look for the name in the respective room's connections.
		for (counter = 0; counter < roomArray[currentPosition]->numberOfConnections; counter++)
		{

			//If time is selected
			if (strcmp(inputArray, "time") == 0)
			{
				/*
					This rest of the loop is based off of code from the 2.3 lecture. I also read the following:
					https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
					https://www.geeksforgeeks.org/multithreading-c-2/
					https://www.youtube.com/watch?v=sD__p9cbQpc

					It'll make the new thread, print out the time, and write it to file.
				 */

				 isValidInput = 1;

				 //Declare the pthread
				 pthread_t timeThread;
				 int timeHolder = pthread_create(&timeThread, NULL, writeTimeFile, &myMutex);

				 timeHolder = pthread_join(timeThread, NULL);

				 //break out of the loop
				 break;
			}

			//If any other valid name is entered
			else if (strcmp(inputArray, roomArray[currentPosition]->connectionList[counter]) == 0)
			{
				isValidInput = 1;
				stepCounter++; //Increment steps
				char tempString[8]; //Temporarily holds the name
				memset(tempString, '/0', sizeof(tempString));
				strcpy(tempString, roomArray[currentPosition]->connectionList[counter]);;
				currentPosition = findRoomPosition(tempString);

				chosenPath[stepCounter] = currentPosition;

				//If the end room is encountered
				if (strcmp(roomArray[currentPosition]->roomType, "END_ROOM") == 0)
				{
					printf("YOU HAVE FOUND THE END ROOM! CONGRATULATIONS!\n");
					printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS: \n", stepCounter);

					counter = 1;
					//Print out the path taken
					while (counter <= stepCounter)
					{
						printf("%s\n", roomArray[chosenPath[counter]]->roomName);
						counter++;
					}

					//This switch will terminate the do-while loop.
					isFinished = 1;
				}

			}
		}

		//Any other input that isn't time or a valid name
		if (isValidInput == 0)
		{
			printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
		}
	}

	while(isFinished == 0);

	//Destroy mutex
	pthread_mutex_destroy(&myMutex);

}
