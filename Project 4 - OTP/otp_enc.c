#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
John Quilty
CS344
OTP

Broad Works Cited:
- The sample server and client code
- I looked at my CS372 projects to remember how I did something but didn't copy any code
- Beej's Guide: https://beej.us/guide/bgnet/html/
- Kernighan & Ritchie's C Book
- Al Sweigart's Hacking Secret Ciphers With Python (https://inventwithpython.com/hacking/)
  This was just for details on logic behind the OTP/Vigenere algorithms, no code was actually used.
- Lectures/Readings
*/

//Prototypes
int returnCharacterCount(char* file);
//Taken from sample code
void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

//By and large this is based off the sample code. I also looked at my CS372 Projects
//FTP and Chat for structure but didn't re-use any code.
int main(int argc, char *argv[])
{
	char buffer[2048];
	char cipher[100000];
	int socketFD, portNumber, charsWritten, charsRead;
	int characterCount;
	int charsRecieved = 0;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;

	//Memset everything out
	memset(buffer, '\0', sizeof(buffer));
	memset(cipher, '\0', sizeof(cipher));
	if (argc < 3) { fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	//Read in how many characters we have
	characterCount = returnCharacterCount(argv[1]);
  //Open up the file and start reading.
  FILE* fp = fopen(argv[1], "r");

  //allocate memory for the plaintext
  char plainText[characterCount];
  //Zero out the string
  memset(plainText, '\0', characterCount);
  //Using fgets to copy from file, just like Adventure, which I double checked on this
  fgets(plainText, characterCount, fp);
  //Close out file
  fclose(fp);

  //Now we have to read the key.
  //Make array for key of same size
  char key[characterCount];
  memset(key, '\0', characterCount);
  //Open file from argument
  fp = fopen(argv[2], "r");
  //Read from file, copy into variable
  fgets(key, characterCount, fp);
  //close file stream
  fclose(fp);
  //print

  //Verify that the text is valid ASCII range
	int i;
  for (i = 0; i < strlen(plainText); i++)
  {
    if (plainText[i] != 32 && (plainText[i] < 65 || plainText[i] > 90)) {
      perror("ERROR: This can only accept capital letters and spaces.");
      exit(2);
    }
  }

  //Check that plaintext and key are the same legnth, otherwise it doesn't work
  if (strlen(plainText) != strlen(key))
  {
    //printf("Key and text length do not match.\n");
    perror("Key and text length do not match.\n");
    return 1;
  }

  // Set up the socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (socketFD < 0) error("CLIENT: ERROR opening socket");

	//Taken from Beej's guide, I used setsocopt to make this socket reusable.
	//https://beej.us/guide/bgnet/html/#setsockoptman
	int optVal = 1;
	setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));


	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");


	//Send test message. This is per the suggestion in the program outline.
	//This is largely a reuse of code from the example code
	//Naming it with Enc/Dec makes them mutually exclusive per the directions
	char* testMessage = "ConnectivityPingEnc";
	charsWritten = send(socketFD, testMessage, strlen(testMessage), 0);
	//printf("Charswritten is %d\n", charsWritten);
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");

	charsRead = 0;

	//Loop while waiting for data
	while (charsRead == 0)
	{
		charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0);
	}

	if (charsRead < 0) error("CLIENT: ERROR writing from the socket");

	//Give error if not the right program or the connection isn't good
	if (strcmp(buffer, "Nope.avi") == 0)
	{
		fprintf(stderr, "Connection issue. Verify that the proper encoding or decoding program is being used.\n");
		exit(2);
		return 1;
	}

	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	sprintf(buffer, "%d", characterCount);

	//printf("Buffer after characterCount write is %s\n", buffer);
	charsWritten = send(socketFD, buffer, sizeof(buffer), 0);
	//Clear out buffers
	memset(buffer, '\0', sizeof(buffer));
	//Write to buffer
	sprintf(buffer, "%d", characterCount);

	//Clear out buffers and reset counters
	memset(buffer, '\0', sizeof(buffer));
	charsRead = 0;
	charsWritten = 0;

	//Now we take in data to give it the go-ahead. This will just loop until
	//we get data
	while (charsRead == 0)
	{
		charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	}

	//See if we can proceed, if so send the data
	if (strcmp(buffer, "Continue") != 0)
	{
		printf("Client: No continue\n");
		return 1;
	}

	//Send the plaintext
	charsWritten = 0;
	while (charsWritten <= characterCount)
	{
		memset(buffer, '\0', sizeof(buffer));
		charsWritten += send(socketFD, plainText, sizeof(buffer), 0);
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		memset(buffer, '\0', sizeof(buffer));
	}

	//Send the key
	charsWritten = 0;
	while (charsWritten <= characterCount)
	{
		memset(buffer, '\0', sizeof(buffer));
		charsWritten += send(socketFD, key, sizeof(buffer), 0);
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		memset(buffer, '\0', sizeof(buffer));
	}

	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse

	//Counting variables
	charsRead = 0;
	int counter = 0;

	while (charsRead <= characterCount)
	{
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
		charsRecieved  = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
		charsRead += charsRecieved;
		charsRecieved = 0; //Reset
		strcat(cipher, buffer);

		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
		counter++;
	}

	close(socketFD); // Close the socket

	//Add newline per directions and print
	strcat(cipher, "\n");
	printf("%s", cipher);

	return 0;
}

//This will count how many characters there are. I'm assuing a single line
//for the text
int returnCharacterCount(char* file)
{
	//Counters, place variable, and file based on paramenter.
	bool isDone = false;
	int counter = 1;
	int currentCharacter;
	FILE* fd = fopen(file, "r");

	//I looked at K&R, they describe fgetsc on pg 246, going over I/O library functions
	while (isDone == false)
	{
		currentCharacter = fgetc(fd);

		//Checks for EOF, newline, Null
		if (currentCharacter == EOF || currentCharacter == '\n' || currentCharacter == '\0')
		{
			isDone = true;
		}

		else
		{
			counter++;
		}
	}
	//Close file, return count value.
	fclose(fd);
	return counter;
}
