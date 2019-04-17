/*Program: smallsh.c
Name: Calista Wong
Description: This is a shell in C that runs command line instructions which work like a bash shell.
Details:
Supports exit, cd, status, and comments beginning with #
Use fork(), exec(), and waitpid() to execute commands
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#define MAX_CHARS 2048
#define MAX_ARGS 512



/**********************GLOBAL VARIABLES**************************/
int i;
int j;
int currentStatus=0;
/**Process variables****/
int numProcesses=0; //keeps track of number of processes
pid_t childProcesses[10000]; //array of all process IDs running
/****Background variables****/
bool isBackground = false; //whether or not process is background
bool backgroundLock = false; //for managing ctrl-Z
bool backgroundFlag = false; //keep track of whether a & was ever detected regardless of the backgroundLock
/***Exit status***/
int statusCode; //exit status of most recent process 

/**********************FUNCTION DEFINITIONS************************/

/***For parsing arguments***/
void append(char*s, char c); //helper function to append char to string
int numArgs(char str[]); //returns number of arguments provided in a user input string
void parseArgs(char**args, char input[]); //fills in array with built in argument strings

/***For processing command***/
void executeCommand(char**args, int argumentsNum);
void outputRedirect(char**args, int i, bool fileSpecified);
void inputRedirect(char**args, int i, bool fileSpecified);

/***For signal handling***/
void sigHandler(int signal);

/***For checking and clearing any finished background processes before each iteration***/
void checkBackground();

int main()
{
  	int processShell = 1; //controls the shell loop
        char**args; //an array of arguments
	int argumentsNum;	

	/*****signal handlers*****/
	//set up struct for SIGINT
	struct sigaction SIGINT_action = {0};
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags =0;
	SIGINT_action.sa_handler = SIG_IGN; 
	sigaction(SIGINT, &SIGINT_action, NULL);	
	
	//set up struct for SIGTSTP
	signal(SIGTSTP, sigHandler);

	/************Command Processing*****************/
        while (processShell == 1)
        {
		/****checking and clearing any finished background processes***/
		checkBackground();	

		 /*****get user command from command line*****/
                printf(": ");
                char*line = NULL;
                ssize_t buffer = MAX_CHARS;
                getline(&line, &buffer, stdin);
		
               	argumentsNum = numArgs(line); //count how many arguments and also tells us if process is background
		//printf("number of arguments raw: %i\n", argumentsNum);
		
		/*
		if (argumentsNum >MAX_ARGS)
			return 0;		
		*/

		//dynamic array for arguments
		int n=0;
               	if (isBackground == false && backgroundLock ==false) //treat as normal process
               		{
                       		//printf("not background,no lock\n");
                       		n = argumentsNum;
               		}
               	else if (isBackground==false && backgroundLock==true) //treat as normal process
               		{
                       		//printf("not background, lock\n");
                       		if (backgroundFlag == true)
                               		{
                                       		n = argumentsNum-1;
                                       		argumentsNum--;
                               		}
                       		else //backgroundFlag == false
                               		n=argumentsNum;
               		}
               	else if (isBackground==true && backgroundLock==false)//treat as background process
               		{
                       		//printf("is background, no lock\n");
                       		n = argumentsNum-1;
                       		argumentsNum--;
                	}
               	//printf("We should have %i spaces in our array.\n", n);
 		args = malloc(n* sizeof(char*)); //allocate array memory for the specified number of arguments
               	for (i=0; i<n; i++)
                     	args[i] = malloc(10 * sizeof(char));
               	parseArgs(args,line); //parse arguments into an array that can be easily accessible
               	argumentsNum--; //because we allocated an extra space for the NULL at the end of each array
               	//printf("number of args without NULL: %i\n", argumentsNum);
	
		//debug

		/*	
	 	printf("list of args: \n");
               	for (i=0; i<n; i++)
                       	{
                              if (args[i] == NULL)
                                       printf("null\n");
				else
					printf("%s\n",args[i]);
                       	}
		
		*/

		/**************************Execute arguments here**********************/
		if (line[0] == '\n') 
                        {
				//printf("no argument\n");
				//no argument, only hit space
			}
		else if (strncmp(args[0],"#",1)==0 )
			{
				//comment
			}
		else if (strcmp(args[0], "cd")==0 )
			{
			  	//printf("change directory\n");
  			  	if (argumentsNum ==1) //only one argument, home directory
                                        {
                                                //printf("change to home\n");
                                                chdir(getenv("HOME"));
                                        }
                                else if (argumentsNum == 2) //two arguments, specified directory
                                        {
                                                 if (chdir(args[1]) != 0)
                                                        {
                                                                printf("No such file or directory\n",args[1]);
                                                                fflush(stdout);
                                                        }
                                        }
                                else
                                        {
                                                printf("Too many arguments\n");
                                                fflush(stdout);
                                        }

			}
		 else if (strcmp(args[0], "status")==0 )
                        {
                                //printf("get status\n");
                                if (argumentsNum ==1)
                                        {
						if (statusCode != 0 && statusCode !=1)
						{
							printf("terminated by signal %d\n", statusCode);
						}
						else
						{
							printf("exit value %d\n", statusCode);
							fflush(stdout);
						}
                                        }
                                else
                                        {
                                                printf("Too many arguments\n");
                                                fflush(stdout);
                                        }

                        }
                else if (strcmp(args[0], "exit")==0 )
                        {
			
                                //printf("exit");
                                
				 for (i=0; i<numProcesses; i++) //kill all processes
                                    kill(childProcesses[i], SIGKILL);
			
                                 processShell = 0; //exit shell
                        }
		else //process command
			{
				executeCommand(args, argumentsNum);
			}

		//free memory
		free(args);
		
		//variable reset
		if (backgroundLock==false)
                        isBackground = false; //reset the background flag if background lock is not in place
                backgroundFlag = false;
	}
}
/*************************************For parsing arguments*****************************************/
void append(char*s, char c) //helper function to append char to string
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

char buffer[5];
int numArgs(char *str) //function to return number of args in a string, reference: https://www.geeksforgeeks.org/count-words-in-a-given-string/ (ALSO: tells us whether this is a background status or not)
{
	int length = strlen(str);
	int count=0; //the total number of arguments

	for (i=0; i<length; i++)
	{
		if (str[i] == ' ' || str[i] =='\n' || str[i]=='\t')
				count++;
		else if (str[i] == '$')
			{
				append(buffer, '$');
				if (strcmp(buffer, "$$")==0 && str[i+1]=='$')
					{
						count++;
						memset(buffer, 0, strlen(buffer));
					}
			}

		if (str[i] == '&' && backgroundLock == true)
			{
				//printf("background will not be processed\n");
        	                isBackground = false;
	                        backgroundFlag = true;

			}
		if (str[i] == '&' && backgroundLock == false)
			{
				//printf("background will be processed\n");
        	                isBackground = true;
                	        backgroundFlag = true;
	
			}

	}

	memset(buffer, 0, strlen(buffer));
	return count+1; //add 1 because we have NULL at the end
}

char temp[10];
void parseArgs(char**args,char input[])
{
        j=0; //iterate through the array
        int strLen = strlen(input);
	char mypid[10];
	sprintf(mypid, "%d", getpid());

	for (i=0; i<strLen; i++)
        {
		if (input[i] == ' ' || input[i] == '\n' )
		{
			if (strcmp(temp, "$$")==0)
				strcpy(args[j],mypid);
			else
				strcpy(args[j],temp);
			j++;
			memset(temp, 0, strlen(temp));					
		}	
	 	else if (input[i] != '&' )
		{ 
                	append(temp, input[i]);
			if (strcmp(temp, "$$")==0 && input[i+1]=='$')
				{
					strcpy(args[j],mypid);
					j++;
					memset(temp, 0, strlen(temp));						
				}
		}
	}

	if (backgroundFlag==true)
                args[j-1] = NULL;
        else
                args[j] = NULL;

        memset(temp, 0, strlen(temp)); 
}	

/*****************************************For processing command**************************************/

void executeCommand(char**args, int argumentsNum)
{
 	pid_t pid = fork();
        int fd; //file descriptor
        int ret; //return value
	
	if (pid == -1)
		{
			perror("Error while forking");
			//exit(1);
		}
	else if (pid == 0) //child process
		{
			if (isBackground==false)
			{
				signal(SIGINT, SIG_DFL);	
				
			}
			bool redirect = false; //flag to see whether we redirected or not
			for (i=0; i<argumentsNum; i++) //for redirection processing
				{
					bool fileSpecified = true;
					if (args[i+1] == NULL)
					{
							fileSpecified=false; //no file is specified
							//printf("no file specified\n");
					}

					/*****Process file redirection functions*****/		
					if (strcmp(args[i],">")==0 ) //output redirection
					{
							redirect = true; //update flag				
							outputRedirect(args,i,fileSpecified);
					}
					
					else if (strcmp(args[i],"<")==0) //input redirection
					{
							redirect = true;
							inputRedirect(args,i,fileSpecified);
					}
				}
			if (redirect == false) //for normal processing
				{
					if ( execvp(args[0], args) == -1)  //there was an error
                                    	{
                                              	perror("exec");
						exit(1);
					}	
				}
			/******only gets here if there is an error******/
			//kill(pid, SIGTERM);
			//return;
			//exit(1);
		}
	
	else if (pid > 0 ) //parent process
		{
			if (isBackground==false ) //foreground process
				{
					int status=-5;
					pid_t actual = waitpid(pid, &status, 0);
				
					if ( actual == -1 ) //error in waiting  
					{
						perror("wait");
						exit(1);
					}
					else if (actual==0) //child still running
					{
						//printf("killing child\n");
						kill(pid, SIGTERM);
						//wait(NULL);
					}
					else if (WIFSIGNALED(status) !=0) //terminated by 2
					{
						printf("terminated by signal %d\n", WTERMSIG(status));
						fflush(stdout);
						statusCode = WTERMSIG(status);
					}
					else if(WIFEXITED(status)) //complete exit
					{
						statusCode = WEXITSTATUS(status);
					}
				}
			else if (isBackground==true) //background process
				{
						printf("background pid is %d\n", pid);
						childProcesses[numProcesses] = pid; //store pid in array 
						numProcesses++;
				}
		}
	
}

void outputRedirect(char**args, int i, bool fileSpecified)
{
	int fd; //file descriptor
        int ret; //return value
		
	//reformat arguments for exec	
	char*commandArgs[2];
	commandArgs[0] = args[i-1];
	commandArgs[1] = NULL;

	//printf("output redirection\n");

	if (isBackground == false && fileSpecified==true)
		fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (isBackground == true && fileSpecified==false)
		fd = open("/dev/null", O_WRONLY);        

	if (fd < 0) //cannot open the output file
        {
                perror("open");
		exit(1);
	}
	ret = dup2(fd,STDOUT_FILENO); 
	if (ret < 0)
	{
		perror("dup2");
		exit(1);
	}
	if (execvp(commandArgs[0], commandArgs) == -1)
	{
		perror("exec");
		exit(1);
	}
	close(fd);
	fflush(stdout);
}

void inputRedirect(char**args, int i, bool fileSpecified)
{
	int fd; //file descriptor
        int ret; //return value

	//reformat argument for exec
	char*commandArgs[2];
	commandArgs[0] = args[i-1];
       	commandArgs[1] = NULL;

	if (fileSpecified == true)
		{
			if (access(args[i+1], R_OK) == -1) //unable to open input file
			{	
				printf("Cannot open %s for input redirection\n", args[i+1]);
				fflush(stdout);
			}
			else
			{
				//printf("input redirection\n");
				fd = open(args[i+1], O_RDONLY, 0);
				if (fd < 0)
				{
					perror("open");
					exit(1);
				}		
				ret = dup2(fd, STDIN_FILENO);
				if (ret < 0)
				{		
					perror("dup2");
					exit(1);
				}
				if (execvp(commandArgs[0], commandArgs)==-1)
				{
					perror("exec");
					exit(1);
				}
				close(fd);
				fflush(stdout);
			}	
		}
	else if (fileSpecified==false && isBackground==true)
		{
			//printf("input redirection, no file specified\n");
			fd = open("/dev/null", O_RDONLY, 0); 
			if (fd < 0)
			{
				perror("open");
				exit(1);
			}		
			ret = dup2(fd, STDIN_FILENO);
			if (ret < 0)
			{		
				perror("dup2");
				exit(1);
			}
			if (execvp(commandArgs[0], commandArgs)==-1)
			{
				perror("exec");
				exit(1);
			}
			close(fd);
			fflush(stdout);
		}
}
/***********************************For Signal handling***************************************/

void sigHandler(int signal)
{
	if (signal == 20) //SIGTSTP aka CTRL-Z: switch background mode
		{
			//signal(SIGTSTP, sigHandler);
			//printf("switching background status: ");
			if (backgroundLock == false)
                	{
                        	backgroundLock=true;
                       		printf("\nEntering foreground-only mode (& is now ignored)\n");
                        	fflush(stdout);
                	}
        		else //backgroundLock == true
                	{
                        	backgroundLock=false;
                        	printf("\nExiting foreground-only mode\n");
                        	fflush(stdout);
                	}

		}
}


/*******************************************For checking finished background processes*************/
void checkBackground() //source: https://stackoverflow.com/questions/7155810/example-of-waitpid-wnohang-and-sigchld
{
	int status;
	for (i=0; i<numProcesses; i++)
		{
			if (waitpid(childProcesses[i], &status, WNOHANG) > 0) //proess finished or terminated 
				{
					if (WIFEXITED(status)) //the child process has exited	
						{
							printf("background pid %d is done: exit value %d\n", childProcesses[i], WEXITSTATUS(status));
							fflush(stdout);
						}
					if (WIFSIGNALED(status)) //child process was terminated with a signal
						{
							//printf("Child process %d terminated with signal %d\n", childProcesses[i], WEXITSTATUS(status));
							printf("background pid %d is done: terminated by signal %d\n", childProcesses[i], WTERMSIG(status));

						}		
				}
		}
}



