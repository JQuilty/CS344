#include <assert.h>
#include <ctype.h>
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
bool validatePort(char* str);
void encryptMessage(char* message, char* key, int length);
void error(const char *msg);

int main(int argc, char* argv[])
{
  //Pretty much all this is based off of server.c sample file.
  //I added a length,  and my spawnpid variable
  int childExitStatus;
  int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
  int length;
  int optVal = 1; //From beej
  pid_t spawnPid; //From smallsh
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

  //Validate proper port number
  if (validatePort(argv[1]) == false)
  {
    exit(1);
  }

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

  //setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");

  //Set up a while loop to be constantly listening.
  while(1)
  {
    listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

     // Accept a connection, blocking if one is not available until one connects
	   sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
     establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
     if (establishedConnectionFD < 0) error("ERROR on accept");

     //Now we set up the forking into a new process. Adapted from 3.1 lecture code at 25:00
     //Reused from smallsh
     spawnPid = fork();

     //Note I re-used some of the structure of the forking from smallsh
     switch(spawnPid)
     {
       case -1:
                perror("Hull breach!\n");
       case 0:
       {
                //I moved buffer down here. It's the only place where it's used
                //and each child should have its own
                char buffer[2048];
                char cipher[100000];
                char key[100000];
                char message[100000];

                //Counters for how many characters we have recieved/sent
                int charsRecieved = 0;
                int charsSent = 0;

                //Clear everything out
                memset(buffer, '\0', sizeof(buffer));
                memset(cipher, '\0', sizeof(cipher));
                memset(key, '\0', sizeof(key));
                memset(message, '\0', sizeof(message));

                //Second while loop, just recv's as long as there's nothing
                while (charsRead == 0)
                {
                  charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer) - 1, 0); // Read the client's message from the socket
                }

                if (charsRead < 0) error("ERROR reading from socket");

                //Send in failure. Sample code didn't have one for failure, so I made one.
                //Uses Enc at end to make sure enc/dec can't be used with each other.
                if (strcmp(buffer, "ConnectivityPingEnc") != 0)
                {
                  //Nope https://www.youtube.com/watch?v=gvdf5n-zI14
                  charsRead = send(establishedConnectionFD, "Nope.avi", 8, 0);
                  exit(2);
                }

                else
                {
                  //Clear out buffer, send confirmation
                  //Send a Success message back to the client
                  memset(buffer, '\0', sizeof(buffer));
                  charsRead = send(establishedConnectionFD, "Yep", 3, 0); // Send success back
                  charsRead = 0; //reset

                  //While loop, waits and listens for the size of the file
                  while (charsRead == 0)
                  {
                    //buffer is -1 to account for newline/null/eof
                    charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer)-1, 0);
                  }

                  //Convert buffer to an int.
                  int messageSize = atoi(buffer);

                  //Send client a message to continue
                  charsRead = send(establishedConnectionFD, "Continue", 8, 0);
                  charsRead = 0; //Reset

                  //Get the message
                  while (charsRead < messageSize)
                  {
                    memset(buffer, '\0', sizeof(buffer));
                    charsRecieved = recv(establishedConnectionFD, buffer, sizeof(buffer)-1, 0);
                    charsRead += charsRecieved;
                    charsRecieved = 0; //reset

                    //concat into the message
                    strcat(message, buffer);
                    //Reset the buffer
                    memset(buffer, '\0', sizeof(buffer));
                  }

                  //Reset our counting variables
                  charsRecieved = 0;
                  charsRead = 0;

                  //Do the same thing, but with the key. We assume key is the same size of message
                  while (charsRead < messageSize)
                  {
                    memset(buffer, '\0', sizeof(buffer));
                    charsRecieved = recv(establishedConnectionFD, buffer, sizeof(buffer)-1, 0);
                    charsRead += charsRecieved;

                    //concat into the message
                    strcat(key, buffer);
                    //Reset the buffer
                    memset(buffer, '\0', sizeof(buffer));
                    //Reset our counting variable
                    charsRecieved = 0;
                  }

                  //Clear buffer
                  memset(buffer, '\0', sizeof(buffer));

                  //Now we do the actual encryption work.
                  encryptMessage(message, key, messageSize);

                  //Now we send it back.
                  charsSent = 0;
                  while (charsSent < messageSize)
                  {
                    charsSent = send(establishedConnectionFD, message, sizeof(message), 0);
                  }

                  exit(0);
                }
              }

        default:
        {
          //This will check if spawnpid has finished, return 0 immediately if it hasn't.
          //Based on "Check if any process has completed, return immediately with 0 if none have:"
          //In 3.1 at 20:14
          //This is reused from smallsh
          pid_t childPID = waitpid(spawnPid, &childExitStatus, WNOHANG);
        }
      }
          //Close this established connection
          close(establishedConnectionFD);
  }
  close(listenSocketFD); // Close the listening socket
	return 0;
}
//This will check the port slot in the arguments,
//making sure it's actually a positive integer within the right range.
bool validatePort(char* str)
{
  int portCheck = atoi(str);

  if (portCheck > 65535 || portCheck < 1024)
  {
    printf("Invalid port range. Please use a port between 1024-65535\n");
    return false;
  }

  else
  {
    return true;
  }
}

/*
  For this, I consulted the linked Wikipedia article from the instructions,
  as well as Al Sweigart's Hacking Secret Ciphers With Python: https://inventwithpython.com/hacking/
  It's Python, but he goes over how the algorithms themselves work in a simpler way.
  Specifically I read Chapters 21 and 22, on the Vigenere Cipher and One Time Pad.
  I didn't use any code (and couldn't, he uses python libraries and functions),
  just consulting his explanations of the algorithms.

    I consulted this ASCII table: http://www.asciitable.com/
*/
void encryptMessage(char* message, char* key, int length)
{
  int i;
  for (i = 0; i < length - 1; i++)
  {
    //Cast to an int
    int messagePosition = (int)message[i];
    int keyPosition = (int)key[i];


    //If ASCII 32 (space, put as 26)
    if (messagePosition == 32)
    {
      messagePosition = 26;
    }

    else
    {
      messagePosition -= 65;
    }

    if (keyPosition == 32)
    {
      keyPosition = 26;
    }

    else
    {
      keyPosition -=65;
    }


    messagePosition += keyPosition;
    messagePosition = messagePosition % 27;
    messagePosition += 65;

    //Convert to a space if it hits 91
    if (messagePosition == 91)
    {
      messagePosition = 32;
    }

    //cast back to a char
    message[i] = (char)messagePosition;
  }

  //Add a null at the end
  message[i] = '\0';
}

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues
