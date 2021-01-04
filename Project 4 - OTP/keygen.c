#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[])
{
  //Check for argument count
  if (argc != 2)
  {
    printf("Invalid argument count.\n");
    return 1;
  }
  //seed RNG
  srand(time(NULL));

  //Convert string to int
  int keylength = atoi(argv[1]);

  //Make array for key, length+1 to handle the null at the end
  char keyArray[keylength + 1];

  //Clear out the key array
  memset (keyArray, '\0', sizeof(keyArray));

  char* characterArray = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  //Randomly assign a character to an index
  //Will fill everything but the last
  int i;
  for (i = 0; i < keylength; i++)
  {
    keyArray[i] = characterArray[rand() % 27];
  }

  //Making last newline per instructions
  keyArray[keylength] = '\n';

  //Printing with newline per directions
  printf("%s", keyArray);
  return 0;
}
