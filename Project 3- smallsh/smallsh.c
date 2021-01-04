/*
  John Quilty
  CS344
  Smallsh 
*/
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
Broad Works Consulted:
- Lectures
- Brian Fraser's Youtube Channel: https://www.youtube.com/watch?v=TXGREvxPbL4&list=PL-suslzEBiMrqFeagWE9MMWR9ZiYgWq89
- The C Programming Language. 2nd Edition by Brian Kernighan and Dennis Ritchie
- http://man7.org -- Linux man pages
- https://www.tutorialspoint.com/c_standard_library/c_function_strncmp.htm -- strncmp manual
*/

#define MAX_ARG 512
#define MAX_LENGTH 2048

//Bool to check foreground/background
//False means it is in the foreground.
//True means it is in backgroundFlag.
bool foregroundStatus = false;

//Prototypes
void catchSIGTSTP(int signo);
void catchSIGINT(int signo);
void displayExit(int status);
void executeCommand(bool isBackground, char* command[], char input[], char output[], int* childStatus, struct sigaction sa);
void processInput(bool* isBackground, char* command[], char inputFile[], char outputFile[], int pid);

int main(int argc, char *argv[])
{
  //Some variables taken from 3.1 at 24:00
  bool isBackground = false;
  bool isFinished = false; //Bool to track our status. True will be used to exit.
  char inputFile[MAX_ARG]; //Used for stdin redirection
  char outputFile[MAX_ARG]; //Used for stdout redirection
  char* commandBuffer[MAX_LENGTH]; //Holds input and is manipulated and parsed
  char* token; //Token to check for redirect
  int exitVar = 0; //Holds value for status.
  int i; //counter
  int pid = getpid(); //PID

  //pid_t spawnPid = -5;

  //Set everything to null in buffers
  memset(commandBuffer, '\0', sizeof(commandBuffer));
  memset(inputFile, '\0', sizeof(inputFile));
  memset(outputFile, '\0', sizeof(outputFile));

  //Declaring sigaction per Lecture 3.3 at 37:40
  //Taken from 3.3 reading
  //Also consulted manpage: http://man7.org/linux/man-pages/man2/sigaction.2.html
  struct sigaction SIGINT_action = {0};
  SIGINT_action.sa_handler = catchSIGINT;
  sigfillset(&SIGINT_action.sa_mask);
  sigaction(SIGINT, &SIGINT_action, NULL);

  //Now do the same for SIGTSTP, I just adjusted the code
  struct sigaction SIGTSTP_action = {0};
  SIGTSTP_action.sa_handler = catchSIGTSTP;
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  //Do-while loop will continue until isFinished is set to true.
  do {
    //Process the input. isBackground not passed by value because it is
    //possible it needs to be altered.
    processInput(&isBackground, commandBuffer, inputFile, outputFile, pid);

    //If we're done, flip the switch
    if (strcmp(commandBuffer[0], "exit") == 0)
    {
      isFinished = true;
    }

    //Check for comments, just continue if we get the first char a #
    else if (commandBuffer[0][0] == '#')
    {
      continue;
    }

    //Check for just hitting enter, continue to a new line if that's the case.
    else if (commandBuffer[0][0] == '\0')
    {
      continue;
    }

    //Do status
    else if (strcmp(commandBuffer[0], "status") == 0)
    {
      //Print exit value, flush stdout.
      displayExit(exitVar);
      fflush(stdout);
    }

    //Check for cd
    else if (strcmp(commandBuffer[0], "cd") == 0)
    {
      //Check for second argument
      if (commandBuffer[1] == NULL)
      {
        //https://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm
        chdir(getenv("HOME")); //equivalent of cd ~
      }

      else
      {
        //Change directory, give error if it's a bad value
        if (chdir(commandBuffer[1]) == -1)
        {
          printf("Error: Directory does not exist.\n");
          fflush(stdout);
        }
      }
    }

    //If not a builtin, execute the command
    else
    {
      executeCommand(isBackground, commandBuffer, inputFile, outputFile, &exitVar, SIGINT_action);
    }

    //Flush everything for next pass
    memset(commandBuffer, '\0', sizeof(commandBuffer));
    memset(inputFile, '\0', sizeof(inputFile));
    memset(outputFile, '\0', sizeof(outputFile));
    fflush(stdout);
    isBackground = false;

  } while(isFinished == false);

  return 0;
}


//Taken from  https://oregonstate.instructure.com/courses/1780106/pages/3-dot-3-advanced-user-input-with-getline
void catchSIGINT(int signo)
{
  char* message = "SIGINT. Use CTRL-Z to Stop.\n";
  write(STDOUT_FILENO, message, 28);
}

//Basing on the above catchSIGINT
void catchSIGTSTP(int signo)
{
  //A SIGTSTP signal means that we want to swap foreground/background,
  //so both branches will flip it's status.
  //Write is used per the 3.3 reading
  if (foregroundStatus == false)
  {
    foregroundStatus = true;
    char* outputMessage = "Entering foreground-only mode (& is now ignored)\n"; //Taken from instructions
    write(STDOUT_FILENO, outputMessage, 50);
    fflush(stdout);
  }

  //Pretty much the same as the above, it just flips the foregroundStatus
  //the other way.
  else if (foregroundStatus == true)
  {
    foregroundStatus = false;
    char* outputMessage = "Exiting foreground-only mode\n";
    write(STDOUT_FILENO, outputMessage, 30);
    fflush(stdout);
  }
}

void processInput(bool* isBackground, char* command[], char inputFile[], char outputFile[], int pid)
{
  //Used for tracking a newline char
  bool newlineFound = false;
  //Function-specific string array to parse on. It gets cleared via memset.
  char textArray[MAX_LENGTH];
  memset(textArray, '\0', sizeof(textArray));

  //Counting vars
  int i = 0;
  int j = 0;

  printf(": "); //Colon
  fflush(stdout);
  fgets(textArray, MAX_LENGTH, stdin); //Get the actual input

  //Look for the newline, this will swap to true
  //after it's found and top parsing
  while (newlineFound == false && i <= MAX_LENGTH)
  {
    //Convert to null terminator
    if (textArray[i] == '\n')
    {
      textArray[i] = '\0';
      newlineFound = true;
    }
    i++;
  }

  //Check for a blank
  if (!strcmp(textArray, ""))
  {
    command[0] = strdup("");
    return;
  }

  //Tokenize and look for spaces
  char* token = strtok(textArray, " ");

  for (i = 0; token; i++)
  {
    //Check for the direct/background symbols
    if (!strcmp(token, "&"))
    {
      *isBackground = true;
    }

    //Check for stdin
    else if (!strcmp(token, "<"))
    {
      token = strtok(NULL, " ");
      strcpy(inputFile, token);
    }

    //Check for stdout
    else if (!strcmp(token, ">"))
    {
      token = strtok(NULL, " ");
      strcpy(outputFile, token);
    }

    /*I ran into weird memory allocation issues originally. Upon research,
    I found these: https://stackoverflow.com/questions/39694197/when-is-it-a-good-idea-to-use-strdup-vs-malloc-strcpy?noredirect=1&lq=1
    https://www.geeksforgeeks.org/strdup-strdndup-functions-c/

    So I'll be using strdup instead of strcpy

    Also, using snprint to append the PID per this page: https://stackoverflow.com/questions/34260350/appending-pid-to-filename
    */
    else
    {
      command[i] = strdup(token);

      //while (command[i][j] != NULL)
      //Someone in slack suggested using this syntax with command[i][j];, though I don't remember who
      for (j = 0; command[i][j]; j++)
      {
        if (command[i][j] == '$' && command[i][j+1] == '$')
        {
          //Setting to null
          command[i][j] = '\0';
          //Also suggested by someone in slack, I want to say it was Jacob Seawell
          //Re-checking K&R, they actually touch on this on Page 110/111, so I may have read up on it there
          snprintf(command[i], MAX_LENGTH, "%s%d", command[i], pid);
        }
      //  j++;
      }
    }

    //i++;
    token = strtok(NULL, " ");

  }

  //while(token != NULL);

}

//Displays exit statuses
void displayExit(int status)
{
  //Based on 3.1 Lecture at 26:00 and 30:00
  if (WIFEXITED(status))
  {
    int exitStatus = WEXITSTATUS(status);
    printf("exit value %d\n", exitStatus);
  }

  //Based on 3.1 Lecutre at 28:23, I again cut out making another int.
  else if (WIFSIGNALED(status) != 0)
  {
    printf("Terminated by signal %d\n", WTERMSIG(status));
    fflush(stdout);
  }
}

//Executes commands
void executeCommand(bool isBackground, char* command[], char input[], char output[], int* childStatus, struct sigaction sa)
{
  //Largely adapted from the 3.1 lecture code at 25:00
  pid_t spawnPid = -5;
  int childExitStatus = -5;

  //Taken from 3.4 in conjunction with case 0
  int sourceFD;
  int targetFD;
  int result;

  //Perform the fork
  //This whole switch statement based on code in 3.1 at 23:00
  spawnPid = fork();
  switch(spawnPid)
  {
    case -1:
            perror("Hull Breach!\n");
            exit(1);
            break;
    case 0:
            //Set the default handlers and actions
            //This is for SIGINT/CTRL+C
            //Setting to SIG_DFL because per lecture 3.2 that's the only way to get
            //it to survive an exec call.
            sa.sa_handler = SIG_DFL;
            sigaction(SIGINT, &sa, NULL);

            //Based on code from 3.4 at 14:10
            if (strcmp(input, ""))
            {
              sourceFD = open(input, O_RDONLY);
              if (sourceFD == -1) { perror("source open()"); exit(1); }


              result = dup2(sourceFD, 0);
              if (result == -1) { perror("source dup2()"); exit(2); }

              //Close out file, taken from lecture 3.4 at 18:12
              fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
            }

            //Based on code from 3.4 at 14:10
            if (strcmp(output, ""))
            {
              targetFD = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
              if (targetFD == -1) { perror("Unable to open file. Error.\n"); exit(1); }


              result = dup2(targetFD, 1);
              if (result == -1) { perror("Error with the output file.\n"); exit(2); }

              //Close out file, taken from lecture 3.4 at 18:12
              fcntl(targetFD, F_SETFD, FD_CLOEXEC);
            }

            //Casting to char* const* per https://linux.die.net/man/3/execvp
            //Executing file.
            if (execvp(command[0], (char* const*)command))
            {
              //If bad, prints error and purges stdout
              printf("%s:no such file or directory\n", command[0]);
              fflush(stdout);
              exit(3);
            }

            break;

    default:
            //If we're not told to be in the foregorund by either var
            if (foregroundStatus == false && isBackground == true)
            {
              //Based on code from 3.1 at 22:00 and 3.2

              //This will check if spawnpid has finished, return 0 immediately if it hasn't.
              //Based on "Check if any process has completed, return immediately with 0 if none have:"
              //In 3.1 at 20:14
              pid_t childPID = waitpid(spawnPid, childStatus, WNOHANG);

              //Print PID per example output
              printf("background pid is %d\n", spawnPid);
              //Flush stdout
              fflush(stdout);
            }

            else
            {
              //This will check if spawnpid has finished, return 0 immediately if it hasn't.
              //Again taken from the code at 20:14 in 3.1
              pid_t childPID = waitpid(spawnPid, childStatus, 0);
            }

            //Now we check for the terminated processes
            //Based on "check if any process has completed, return immediately with 0 if none have"
            //In 3.1 at 20:14
            while ((spawnPid = waitpid(-1, childStatus, WNOHANG)) != 0)
            {
              printf("Child process %d terminated.\n", spawnPid);
              fflush(stdout);
              displayExit(*childStatus);
            }
  }
}
