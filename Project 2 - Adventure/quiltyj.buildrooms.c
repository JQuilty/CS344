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

/*The struct for the room.

It has two char arrays/strings for the name and for the type.

The totalConnections variable is an int to keep track of how many connections
a room has. Every time a connection is added, this should be incremented,clear
so any checks to see if you can add another room will check this first,
then if it's free it'll add another room.

listOfConnections is a pointer to an array of pointers for the list of connections
that the room has.
*/

struct room
{
	char* roomName;
	char* roomType;
	int totalConnections;
  struct room** listOfConnections;
};

int roomNames[7];

/*
	Pool of names. They are based off the dungeons from The Legend of Zelda:
	Ocarina of Time.

	- Inside the Deku Tree
	- Dodongo's Cavern
	- Inside Jabu-Jabu's Belly
	- Forest Temple
	- Fire Temple
	- Ice Cavern
	- Water Temple
	- Shadow Temple
	- Spirit Temple
	- Ganon's Castle
*/
char* potentialNames[10] =
{
	"DEKUTRE",
	"DGOVAVN",
	"JABUJABU",
	"FOREST",
	"FIRE",
	"ICECVRN",
	"WATER",
	"SHADOW",
	"SPIRIT",
	"GANONCS"
};

/*=========================================================
                      Function Prototypes
=========================================================*/

bool CanAddConnectionFrom(struct room* x);
bool ConnectionAlreadyExists(struct room* x, struct room* y);
bool IsSameRoom(struct room* x, struct room* y);
bool isGraphFull(struct room** roomArray);
int getRandomNumber();
int isAlreadyUsed(int number, int* checkArray);
struct room* GetRandomRoom(struct room** roomArray);
struct room** generateRooms();
void AddRandomConnection(struct room** roomArray);
void assignRoomNames();
void ConnectRoom(struct room* x, struct room* y);
void createDirectory();
void randomizer(int arr[], int n);
void swap(int *a, int *b);
void writeRoomsToDisk(struct room** roomArray);


int main()
{
	//Seed the RNG, srand is a part of stdio*/
	srand(time(NULL));;

	//Counter variable since this is c89
	int i;
	assignRoomNames();

	struct room** roomBoard = generateRooms();

	while (!isGraphFull(roomBoard))
	{
		AddRandomConnection(roomBoard);
	}


	/*
	//Here for testing purposes.
	for (i = 0; i < 7; i++)
	{
		int k;
		printf("Room %d (%s) Connection List:\n", i, roomBoard[i]->roomName);

		for (k = 0; k < roomBoard[i]->totalConnections; k++)
		{
			printf("%s\n", roomBoard[i]->listOfConnections[k]->roomName);
		}

		printf("Room Type: %s\n", roomBoard[i]->roomType);
	}
	*/

	createDirectory();
	writeRoomsToDisk(roomBoard);
	return 0;
}

/*
This function makes the directory with the specified format.
*/
void createDirectory()
{
	//Using getpid() as described in lectures
	int pid = getpid();

	/*
	Setting the array for the name, and then
	zeroing everything out with a null character
	per the suggestions in the lectures.

	I also consulted this article: https://www.geeksforgeeks.org/memset-c-example/
	*/
	char name[40];
	memset(name, '\0', sizeof(name));

	/*
	Write directory name.
	I consulted the lectures as well as
	https://www.geeksforgeeks.org/snprintf-c-library/
	*/
	sprintf(name, "./quiltyj.rooms.%d", pid);

	/*
	Now we check for the existence of an existing directory.
	I used the lectures, this StackOverFlow thread: https://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
	, this man page: https://linux.die.net/man/2/stat
	as well as Kernghan/Ritchie's book on C.

	I know it's generally not 100% safe/best practice to go full 777 for permissions,
	but I don't want any errors for a simple project.
	*/

	struct stat st = {0};

	if (stat(name, &st) == -1)
	{
		mkdir(name, 0777);
	}

	else
	{
		printf("Directory with that name already exists.\n");
	}
}

void writeRoomsToDisk(struct room** roomArray)
{

	//PID
	int pid = getpid();

	//Variables for counting
	int rooms = 7;
	int connections = 0;

	/*
	Variables for the actual files.
	Per lecture suggestions, I'm filling the arrays
	with null terminators before doing anything else.
	*/

	FILE* fp;
	char name[40];
	char fileData[300];
	char directoryString[300];
	memset(name, '\0', sizeof(name));
	memset(fileData, '\0', sizeof(fileData));
	memset(directoryString, '\0', sizeof(directoryString));

	//Writing the directory name
	sprintf(directoryString, "./quiltyj.rooms.%d", pid);


	//For loop to write the actual contents of the file.
	int i;
	for (i = 0; i < rooms; i++)
	{
		//Zero out the file, then write the strings
		memset(fileData, '\0', sizeof(fileData));
		sprintf(fileData, "ROOM NAME: %s\n", roomArray[i]->roomName);

		//Now we make a list of connections and write them to the file.
		//This starts at 1 since the first pass will be for the first connection.
		int numberOfConnections = 1;

		int j;
		for (j = 0; j < roomArray[i]->totalConnections; j++)
		{
				sprintf(fileData + strlen(fileData), "CONNECTION %d: %s\n", numberOfConnections, roomArray[i]->listOfConnections[j]->roomName);
				numberOfConnections++;
		}

		//Put in the room type
		sprintf(fileData + strlen(fileData), "ROOM TYPE: %s\n", roomArray[i]->roomType);


		/*
		Write to file, check for errors if there's a problem

		I based this off code from the lecture 2.4 and from Kernghan/Ritchie Chapter 7.5, who
		instruct on fprintf, it returning null, and closing the file.
		*/

		sprintf(name, "%s/%s_room", directoryString, roomArray[i]->roomName);
		fp = fopen(name, "w+");

		//Check for successful creation
		if (fp == NULL)
		{
			printf("Write error.\n");
		}

		else
		{
			fprintf(fp, "%s", fileData);
			fclose(fp);
		}
	}
}

/*
    Checks the passed in room to see if it's at capacity.
    If it's less than 6, it returns true, otherwise returns false.

    This is based on the pseudocode.
*/
bool CanAddConnectionFrom(struct room* x)
{
    if (x->totalConnections < 6)
        {
            return true;
        }

    else
        {
            return false;
        }
}


/*
    This will check for existing connections between two structs. It'll start by
    using strcmp as covered in the lecture to compare two strings. It'll look for x
    -> to the connectionList array, then the name of the struct in the connectionList.
    If they're the same, it'll return true. It'll check up to totalConnections since
    if it's a room with only one or two connections, there's no point in going up to
    the max six, so totalConnections is the boundary variable.

    This is structured off the pseduocode provided.

*/
bool ConnectionAlreadyExists(struct room* x, struct room* y)
{
	int i;

  for (i = 0; i < x->totalConnections; i++)
  {

		if (strcmp(x->listOfConnections[i]->roomName, y->roomName) == 0)
		{
      return true;
    }

	}

    return false;
}

/*
 * Here we pass in the two structs to connect. This function will
 * add the pointers to the connectionlist arrays, then increment the totalConnections.
 * I originally had this doing these operations twice, but
 * when I realized that ConnectRoom is invoked twice in AddRandomConnection
 * I just cut those out since they were calling on opposite operators
 * and I didn't want to alter the structure further.
 *
 * totalConnections can be used safely since the array is n-1,
 * and after incrementing via ++ we get the equivalent number. IE,
 * if we're putting it in array index 2 (the third index),
 * when incrementing we now show totalConnections at 3,
 * which is correct because of 0 1 2 being filled.
 *
 * This is structured from the provided starter code
 */

 void ConnectRoom(struct room* x, struct room* y)
 {
       x->listOfConnections[x->totalConnections] = y;
       x->totalConnections++;
 }

/*
   Returns true if Rooms x and y are the same Room, false otherwise.
   This is structured from the psuedocode
*/

bool IsSameRoom(struct room* x, struct room* y)
{
		if (x == y)
		{
        return true;
    }

    else
    {
        return false;
    }
}

/*
    This function will give us a random room when invoked. It starts off by calling rand() then moduloing by 7 since we have 7 rooms.
    Then we make a pointer variable, assign it's value to the address of the particular room in the array, and return the pointer to the room
    struct. This requires that the array already be filled.

    This is based off the provided starter code
*/
struct room* GetRandomRoom(struct room** roomArray)
{
    int random = (rand() % 7);
    struct room* functionPointer;
    functionPointer = &roomArray[random];
    return functionPointer;
}

int getRandomNumber()
{
	return rand() % 7;
}

/* Adds a random, valid outbound connection from a Room to another Room
 This is the pseudocode from the lecture used as a starting point then filled out

 From the provided starter code I changed the top declarations to struct pointers and added parameters as I altered the other
 functions given by the starter code
*/
void AddRandomConnection(struct room** roomArray)
{
	struct room* A;
	struct room* B;
	srand(time(NULL));
	int randNum;

  /*Using the GetRandomRoom, we get a pointer to a room and check if we can add a connection.*/
  while(true)
  {
		A = roomArray[randNum];

		if (CanAddConnectionFrom(A) == true)
    {
      break;
    }
  }


  do
  {
		randNum = getRandomNumber();
		B = roomArray[randNum];
  }

  while(CanAddConnectionFrom(B) == false || IsSameRoom(A, B) == true || ConnectionAlreadyExists(A, B) == true);


  //Calling connectroom does the actual linking via pointers. The structs will have their links persist even after function termination
  ConnectRoom(A, B);
  ConnectRoom(B, A);
}




/*
    isGraphFull is a simple bool to say if you've filled up all the rooms fully.
    To do this, it just iterates through the room structs, and if even a single one of them are
    less than three, we know there's still work to do, so it'll return false.

    I'm going to invoke this in main as a method to fill the rooms, where as
    long as it's false it can keep going. Maybe not the most efficient way to do it,
    since it has to check by means of exhaustion, but it logically works.

    This is based off of the supplied pseudocode.
*/
bool isGraphFull(struct room** roomArray)
{
	int i;

  for (i = 0; i < 7; i++)
  {
    if (roomArray[i]->totalConnections < 3 )
    {
      return false;
    }
  }

	return true;
}

/*
	This will give the rooms their names.
*/

void assignRoomNames()
{
  //Counters since this is c89
  int i;

	/*
	I originally had it set up where it would take a rand, then check
	to see if that was used already, and if not add it to the array that
	holds the names. I don't know why it wouldn't work, but I came up with a solution
	to just shuffle the names and assign them into an array since there are only 10
	elements.

	I used this algorithm: https://www.geeksforgeeks.org/shuffle-a-given-array-using-fisher-yates-shuffle-algorithm/
	*/

	int indexArray[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	int n = 10;
	randomizer(indexArray, n);

	for (i = 0; i < 10; i++)
	{
		roomNames[i] = indexArray[i];
	}
}

/*
  This is a function that is going to check if an index is already used
  when we assign the names into the array.
*/
int isAlreadyUsed(int number, int* checkArray)
{
  int k;
  for (k = 0; k < 8; k++)
  {
    if (number == checkArray[k])
    {
      return 1;
    }
  }

  return 0;
}

/*
	This will malloc all the memory we need as well as construct the rooms
	with their attributes.
*/
struct room** generateRooms()
{
  //Allocate initial memory
  struct room** roomMap = malloc(sizeof(struct room*) * 7);

	/*
		This will seed the RNG, then pick two ints in range to be the start
		and ending rooms.
	*/
	srand(time(NULL));
	int start_int = getRandomNumber();
	int end_int = getRandomNumber();

	//Does a check to make sure the random nums are not duplicates.
	while (end_int == start_int)
	{
		end_int = getRandomNumber();
	}

  int i;
  for (i = 0; i < 8; i++)
  {
    /*
    Allocate memory for each room and attirbutes.
    I'm going to give all of them memory for six connections for
    simplicity. The name and type have sizes based on directions.
    */
    roomMap[i] = malloc(sizeof(struct room));
    roomMap[i]->listOfConnections = malloc(sizeof(struct room) * 6);
    roomMap[i]->roomName = malloc(sizeof(char) * 8);
    roomMap[i]->roomType = malloc(sizeof(char) * 11);
		roomMap[i]->totalConnections = malloc(sizeof(int));


    //Copy over the names
    strcpy(roomMap[i]->roomName, potentialNames[roomNames[i]]);


    //Set up connection quanity and addresses.
    //Initially set everything to NULL, I think
    //this was suggested in the lectures but I'd
    //also like to do it for safety.
    roomMap[i]->totalConnections = 0;

    int j;
    for (j = 0; j < 6; j++)
    {
      roomMap[i]->listOfConnections[j] = NULL;
    }

    //Set value of roomtype based either position or if they are
		//the same as the randomly generated start and end ints.
    if(i == start_int)
    {
      roomMap[i]->roomType = "START_ROOM";
    }

    else if (i == end_int)
    {
      roomMap[i]->roomType = "END_ROOM";
    }

    else
    {
      roomMap[i]->roomType = "MID_ROOM";
    }
  }

  //return the array
  return roomMap;
}

/*
	Sourced from https://www.geeksforgeeks.org/shuffle-a-given-array-using-fisher-yates-shuffle-algorithm/
*/
void swap (int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

/*
	This is going to randomize the pointers and allow me to randomly copy names.
	Sourced from: https://www.geeksforgeeks.org/shuffle-a-given-array-using-fisher-yates-shuffle-algorithm/
*/

void randomizer(int arr[], int n)
{
	srand(time(NULL));


	int i;
	for (i = n-1; i > 0; i--)
		{
			int j = rand() % (i+1);
			swap(&arr[i], &arr[j]);
		}
}
