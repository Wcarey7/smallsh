//Wade Carey CS344 Spring 2022

#include <errno.h>     
#include <fcntl.h>     
#include <limits.h>    
#include <signal.h>    
#include <stdio.h>    
#include <stdlib.h>   
#include <string.h>    
#include <sys/stat.h>  
#include <sys/types.h> 
#include <sys/wait.h>  
#include <unistd.h>   



#define MAX_ARGS        512 // max arguments on command line
#define MAX_LENGTH     2048 // max command line length
#define MAX_PIDS       1000 // max PIDs


// define bool as type
typedef enum { false, true } bool;



//global variables
int completed_cur = 0;
int cur = 0;                   // index to add argPTR bg process in bgpid[]
int signalNum = 0;
pid_t fgpid = INT_MAX;         // running foreground process
pid_t bgpid[MAX_PIDS];         // array of open background process IDs
pid_t completed_pid[MAX_PIDS]; // array of completed bg process IDs



//function prototypes
void getCommand(int *numArgs, char **argPTR);
void bgHandler(int sig, siginfo_t* info, void* vp);
void reapZombies();
void changeDirectory(int *numArgs, char **argPTR);
void sigintHandler();
void varExpand(char *input);
void exitShell();





int main(int argc, char** argv)
{
    
    bool isBackgroundProcess = false;
    bool repeat = true;
    pid_t cpid;
    char *args[MAX_ARGS + 1];
    char input[MAX_LENGTH];
    char *token;

    int bgExitStatus;
    int bgStatus; 
    int exitStatus;
    int fd;
    int fd2;
    int i;
    int j;
    int numArgs;
    int status;

    //sigaction struct for background processes
    struct sigaction background_act;
    background_act.sa_sigaction = bgHandler;     
    background_act.sa_flags = SA_SIGINFO|SA_RESTART;
    sigfillset(&(background_act.sa_mask));
    // set up signal handler for completed child process
    sigaction(SIGCHLD, &background_act, NULL);


    //sigaction struct for foreground processes
    struct sigaction foreground_act;
    foreground_act.sa_handler = sigintHandler;
    foreground_act.sa_flags = SA_RESTART;
    sigfillset(&(foreground_act.sa_mask));
    sigaction(SIGINT, &foreground_act, NULL); 


    //sigaction struct to ignore interrupts 
    struct sigaction restOfTheTime_act;
    restOfTheTime_act.sa_handler = SIG_IGN;
    restOfTheTime_act.sa_flags = SA_RESTART;
    sigfillset(&(restOfTheTime_act.sa_mask));
    sigaction(SIGINT, &restOfTheTime_act, NULL); 


    //initialize arrays for bg processes
    for (i = 0; i < MAX_PIDS; i++)
    {
        completed_pid[i] = bgpid[i] = INT_MAX;
    }   

    //memory for arg array. FREE IT
    for (i = 0; i <= MAX_ARGS; i++)
    {
        args[i] = (char *) malloc((MAX_LENGTH + 1) * sizeof(char)); 
    } 




    //DO WHILE repeat == true
    do
    {
        //array of pointers to the malloc'd args array
        char **argPTR = args;

        //null the array
        for (i = 0; i <= MAX_ARGS; i++)
        {
            strcpy(args[i], "\n");
        }

        // clear input buffer each iteration
        strcpy(input, "\0");
 
        i = 0;

        //reapZombies();
        while (i < MAX_PIDS && completed_pid[i] != INT_MAX)
        {
            //wait on current process
            completed_pid[i] = waitpid(completed_pid[i], &bgStatus, 0);

            //print process id and exit status
            if (WIFEXITED(bgStatus)) 
            {
                bgExitStatus = WEXITSTATUS(bgStatus);
                printf("background pid %d is done: exit value %d.\n", completed_pid[i], bgExitStatus);
            }
            else
            {
                bgExitStatus = WTERMSIG(bgStatus);
                printf("background pid %d is done: terminated by signal %d\n", completed_pid[i], bgExitStatus);
 
            }

            //remove current ps from open background process array
            j = 0;
            while (j < MAX_PIDS && bgpid[j] != INT_MAX)
            { 
                if (bgpid[j] == completed_pid[i])
                {
                    // replace value of current bg process 
                    bgpid[j] = INT_MAX;
 
                    int k = j;                       
                    while (k + 1 < MAX_PIDS && bgpid[k+1] != INT_MAX)
                    {
                        bgpid[k] = bgpid[k+1];
                        bgpid[k+1] = INT_MAX;
                        k++;
                    }    
                    // adjust cur index value & make room for new PID  
                    cur--; 
                }

                j++;
            }

            completed_pid[i] = INT_MAX;

            i++; 
        }

        //reset completed bg process array index
        completed_cur = 0;
 
        fflush(stdin);
        fflush(stdout);

        do {
            printf(": ");
            fgets(input, MAX_LENGTH, stdin);
            strtok(input, "\n"); // get rid of newline from fgets
        } while(input[0] == '#'|| strlen(input) < 1);

        fflush(stdin);

         //variable expansion $$
        for (i = 0; i < strlen(input); i ++) 
        {
            if ( (input[i] == '$')  && (input[i + 1] == '$') && (i + 1 < strlen(input))) 
            {
                char *temp = strdup(input);
                temp[i] = '%';
                temp[i + 1] = 'd';
                sprintf(input, temp, getpid());
                free(temp);
            }
        } 

        numArgs = 0;
        token = strtok(input, " "); // check for multiple args

         //tokenize arguments into args array
        while (token != NULL && numArgs < MAX_ARGS)  
        {
 
            // this serves to ignore leading / duplicate / trailing spaces 
            if (strlen(token) == 0)
            {
                continue;
            }   

            // copy current arg to arg array
            strcpy(*argPTR, token);

            // increment argument  counter
            numArgs++;
 
            // get argPTR arg, if any
            token = strtok(NULL, " ");

            // increment pointer unless last iteration
            if (token != NULL)
            {
                *argPTR++;
            } 
        }

        // remove newline char from last arg, if any
        token = strtok(*argPTR, "\n"); 
        if (token != NULL)
        {
            strcpy(*argPTR, token);
        }


/* **********************************************************
 * 
 * BUILT IN COMMANDS
 * 
 * *********************************************************/


        // if command is bg process
        if (strcmp(args[numArgs - 1], "&") == 0)
        {
            // set variable appropriately for later 
            isBackgroundProcess = true;

            // decrement number of args since ampersand will be removed
            numArgs--; 

        }
        else
        {
            *argPTR++;
        }

        if (strcmp(args[0], "exit") == 0)
        {

            i = 0;
            while (i < MAX_PIDS && bgpid[i] != INT_MAX)
            {

 
                kill(bgpid[i], SIGKILL);
                i++;
            }

            // free allocated memory
            for (i = 0; i <= MAX_ARGS; i++)
            {

                free(args[i]); 
            }  

            // exit the shell; set repeat to false
            repeat = false;

        }
        else if (strcmp(args[0], "cd") == 0)
        { 
            if (numArgs == 1)
            {
                chdir(getenv("HOME"));
            }

            else
            {
                chdir(args[1]);
            }

        }
        else if (strcmp(args[0], "status") == 0)
        { 

            if (WIFEXITED(status))
            {
                exitStatus = WEXITSTATUS(status);
                printf("exit value %d\n", exitStatus);
            }
            else if (signalNum != 0)
            {
                printf("terminated by signal %d\n", signalNum);
            } 

        }
        
        //run through args for system calls
        else 
        {
            cpid = fork();

            if (cpid == 0) 
            {
                bool checkStatus = false; 
                bool redirectInput = false;
                bool redirectOutput = false;
                int inputOffset = 0;
                int outputOffset = 0;

                if (numArgs > 4 && strcmp(args[numArgs-4], "<") == 0)
                {

                    // set flag to redirect input
                    redirectInput = true;

                    // set target for input path
                    inputOffset = 3; 
                }
                else if (numArgs > 2 && strcmp(args[numArgs-2], "<") == 0)
                {
                    // set flag to redirect input
                    redirectInput = true;

                    // set target for input path
                    inputOffset = 1; 
                }
                if (numArgs > 4 && strcmp(args[numArgs-4], ">") == 0)
                {
                    // set flag to redirect output
                    redirectOutput = true;

                    // set target for output path
                    outputOffset = 3; 
                }
                else if (numArgs > 2 && strcmp(args[numArgs-2], ">") == 0)
                {
                    // set flag to redirect output
                    redirectOutput = true;

                    // set target for output path
                    outputOffset = 1; 
                }

                // redirect stdin for bg process to dev/null if no path provided
                if (isBackgroundProcess == true && redirectInput == false)
                {
                    fd = open("/dev/null", O_RDONLY);

                    checkStatus = true;      
                }
                else if (redirectInput == true)
                {
                    fd = open(args[numArgs - inputOffset], O_RDONLY);

                    checkStatus = true;  
                }

                if (checkStatus == true)
                {
                    if (fd == -1)
                    {
                        printf("smallsh: cannot open %s for input\n", args[numArgs - inputOffset]);
                        exit(1); 
                    }

                    fd2 = dup2(fd, 0);
                    if (fd2 == -1)
                    {
                        printf("smallsh: cannot open %s for input\n", args[numArgs - inputOffset]);
                        exit(1);
                    }   
                }

                if (redirectOutput == true)
                {
                    fd = open(args[numArgs - outputOffset], O_WRONLY|O_CREAT|O_TRUNC, 0644);

                    if (fd == -1)
                    {
                        printf("smallsh: cannot open %s for output\n", args[numArgs - outputOffset]);
                        exit(1); 
                    }

                    fd2 = dup2(fd, 1);
                    if (fd2 == -1)
                    {
                        printf("smallsh: cannot open %s for output\n", args[numArgs - outputOffset]);
                        exit(1);
                    }   
                }

                // get the greater of the offsets, if any
                i = 0;
                if (inputOffset > outputOffset)
                {
                    i = inputOffset + 1;
                }
                else if (outputOffset > inputOffset)
                {
                    i = outputOffset + 1;
                }

                // move the pointer to omit the input redirection from array
                for (j = i; j > 0; j--)
                {
                    *argPTR--;
                }

                // add NULL pointer to last array index (only in child)
                *argPTR = NULL;

                // exec using path version use Linux built-ins
                execvp(args[0], args);

                // to catch bad filename
                printf("%s", args[0]);
                fflush(NULL);
                perror(" ");  
 
                exit(1); // end child process
            }
            else if (cpid == -1) // parent process
            {   
                // if unable to fork print error
                printf("%s", args[0]);
                fflush(NULL);                 
                perror(" ");
            } 
            else
            {
                //parent process continues here

                //if command is bg process
                if (isBackgroundProcess == true)
                {
                    // then print process id when begins
                    printf("background pid is %d\n", cpid);

                    // reset boolean value for argPTR iteration
                    isBackgroundProcess = false;

                    // add process id to array of background processes
                    if (cur < MAX_PIDS)
                    {  
                        bgpid[cur++] = cpid;
                    }
                } 
                else
                {
                    // reset value of signal number
                    signalNum = 0;                     

                    // assign cpid to global variable
                    // for access in signal handlers  
                    fgpid = cpid;

                    // set interrupt handler for fg process 
                    sigaction(SIGINT, &foreground_act, NULL);

                    // wait for fg child process
                    fgpid = waitpid(fgpid, &status, 0);

                    // restore to ignore interrupts
                    sigaction(SIGINT, &restOfTheTime_act, NULL);

                    // reset global variable so signal handlers know
                    // there is no active fg process
                    fgpid = INT_MAX;

                    // if process was terminated by signal, print message
                    if (signalNum != 0)
                    {
                        printf("terminated by signal %d\n", signalNum);
                    }   
                }
            }
        }

    } 
    while(repeat == true);

    return 0;
}




void
sigintHandler()
{
    // if interrupt signal occurs while fg process is running, kill it
    if (fgpid != INT_MAX)
    {
        // kill the foreground process
        kill(fgpid, SIGKILL);
 
        // set global variable for status messages
        signalNum = 2;  
    }  
    return;
}



void
changeDirectory(int *numArgs, char **argPTR)
{
    int numberArgs = *numArgs;
    //if no args, change to directory specified in HOME
    if (numberArgs == 1)
    {
        chdir(getenv("HOME"));
    }

    //if one arg change to dir provided
    else
    {
        chdir(argPTR[1]);
    }

}


void
getCommand(int *numArgs, char **argPTR)
{
    char input[MAX_LENGTH];
    char *token;
    int i;

    //clear standard in n' out
    strcpy(input, "\0");
    fflush(stdin);
    fflush(stdout);

    //Prompt user store in input array then flush stdin
    printf(": ");
    fgets(input, MAX_LENGTH, stdin);
    fflush(stdin);

    token = strtok(input, " "); 

    // loop to process args up to max 512
    while (token != NULL && *numArgs < MAX_ARGS)  
    {
        
        //ignore leading / duplicate / trailing spaces 
        if (strlen(token) == 0)
        {
            continue;
        }   

        //copy current string in token into pointer to args array
        strcpy(*argPTR, token);

        //numberArgs++;
        *numArgs++;

        //argPTR arg
        token = strtok(NULL, " ");

        //increment pointer if not last loop
        if (token != NULL)
        {
            *argPTR++;
        } 
    }

    // if any newline character remove it
    token = strtok(*argPTR, "\n"); 
    if (token != NULL)
    {
        strcpy(*argPTR, token);
    }

}


void
bgHandler(int sig, siginfo_t* info, void* vp)
{
    pid_t ref_pid = info->si_pid; 

    // if signal is not from fg process, process it here
    if (ref_pid != fgpid && completed_cur < MAX_PIDS)
    {
        // be displayed in main loop
        completed_pid[completed_cur++] = ref_pid;
    } 

    return;
}


void
exitShell()
{
    // kill any processes or jobs that shell has started
    int i = 0;
    while (i < MAX_PIDS && bgpid[i] != INT_MAX)
    {

        kill(bgpid[i], SIGKILL);
        i++;
    }

    fflush(stdout);

    exit(0);
}