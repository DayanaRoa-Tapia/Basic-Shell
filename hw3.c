
#include <unistd.h>
#include "csapp.h"
#include <sys/types.h>
#include <sys/wait.h>

#define MAXARGS 128

/* Function protypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv, char **cmd1, char **cmd2, int *semi);
int builtin_command(char **argv);
void sigint_handler(int sig);
void sigint_handlerZ(int sig);

int main(){
	char cmdline[MAXLINE];

	while(1){
		printf("CS361 >");
		fgets(cmdline, MAXLINE, stdin);
		if(feof(stdin)){
			exit(0);
		}

		
		//Intall the SIGINT handler
                if(signal(SIGINT, sigint_handler) == SIG_ERR){
                        unix_error("caught sigint");
                }
		if(signal(SIGTSTP, sigint_handlerZ) == SIG_ERR){
			unix_error("caught sigtstp");
		}
		else{

               	//Evaluate
               		eval(cmdline);
		}
	
	}
	
}


/* eval - Evaluate a command line */
void eval(char *cmdline){
	char *argv[MAXARGS];
	char buf[MAXLINE];
	int bg;
	
	pid_t pid;
	pid_t pid1;
	pid_t pid2;

	char *cmd1[MAXARGS/2];
	char *cmd2[MAXARGS/2];

	int pipefd[2];
	pipe(pipefd);

	int semi=0;

	strcpy(buf, cmdline);
	bg = parseline(buf, argv, cmd1, cmd2, &semi);
	if(argv[0] == NULL){
		return; //Ignore empty lines
	}
	if(!builtin_command(argv)){
		//printf("cmd1: %s  cmd2: %s\n", cmd1[0],cmd2[0]);
		//printf("semicolon: %d\n", semi);
		
		//Handles command piping
		//
		if(cmd1[0] != NULL && cmd2[0] != NULL && semi == 0){
			if((pid1 = fork()) == 0){
				close(pipefd[0]);
				dup2(pipefd[1], 1);
				if(execvp(cmd1[0], cmd1) < 0){
					printf("%s: Command not found. \n", cmd1[0]);
					exit(0);
				}
			}
			else{
				if((pid2 = fork()) == 0){
					close(pipefd[1]);
					dup2(pipefd[0], 0);
					if(execvp(cmd2[0], cmd2) < 0){
						printf("%s: Command not found. \n", cmd2[0]);
						exit(0);
					}
				}
			}
			int stat;
			if(waitpid(pid1, &stat, 0)<0){
				unix_error("waitfg: waitpid error");
			}
			//printf("pid:%d status:%d\n", pid1, WEXITSTATUS(stat));
			close(pipefd[1]);
			int stat2;
			if(waitpid(pid2, &stat2, 0)<0){
				unix_error("waitfg:(2) waitpis error");
			}
			printf("pid:%d status:%d\n", pid1, WEXITSTATUS(stat));
			printf("pid:%d status:%d\n", pid2, WEXITSTATUS(stat2));
			return;
		}
		
		//handles semicolon seperated sommands
		if(cmd1[0] != NULL && cmd2[0] != NULL && semi == 1){
			if((pid1 = fork()) == 0){
				close(pipefd[0]);
				dup2(pipefd[1],1);
				if(execvp(cmd1[0], cmd1) < 0){
					printf("%s: Command not found.\n", cmd1[0]);
					exit(0);
				}
			}
			else{
				int stat;
				if(waitpid(pid1, &stat, 0) < 0){
					unix_error("waitfg-semi: waitpid error");
				}
				printf("pid:%d status:%d\n", pid1, WEXITSTATUS(stat));
				
				if((pid2 = fork()) == 0){
					close(pipefd[1]);
					dup2(pipefd[0],0);
					if(execvp(cmd2[0], cmd2) < 0){
						printf("%s: Command not found.\n", cmd2[0]);
						exit(0);
					}
				}
				close(pipefd[1]);
				int stat2;
				if(waitpid(pid2, &stat2, 0) < 0){
					unix_error("waitpid-semi: waitpid2 error");
				}
				printf("pid:%d status:%d\n", pid2, WEXITSTATUS(stat2));
			}
			return;
		}


		//handles single command
		else {
			if((pid = fork()) == 0){ /* Child runs user job */

			if(execvp(argv[0], argv) < 0){
				printf("%s: Command not found.\n", argv[0]);
				exit(0);
			}
		}
		//}

		/* Parent waits for foreground job to terminate */
		if(!bg){
			int status;
			if(waitpid(pid, &status, 0)<0){
				unix_error("waitfg: waitpid error");
			}
			printf("pid:%d status:%d\n", pid, status);
		}
		else{
			printf("%d %s", pid, cmdline);
		}
	}
		
	}
	return;
}

/* If first arg is builtin command, run it and return true */
int builtin_command(char **argv){
	
	if(!strcmp(argv[0], "exit")){
		exit(0);
	}
	if(!strcmp(argv[0], "&")){
		return 1;
	}
	return 0;
}


		
/*parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv, char **cmd1, char **cmd2, int *semi){
	char *delim;  //Points to first space delimeter
	int argc;     // number of args
	int bg;       // background job?

	int cmd1c;
	int cmd2c;


	buf[strlen(buf)-1] = ' ';
	while(*buf && (*buf == ' ')){
		buf++;
	}

	/* Build the argv list */
	argc = 0;
	cmd1c = 0;
	cmd2c =0;
	int found = 0;
	while ((delim = strchr(buf, ' '))){
		//printf("found %d\n", found);
		if(*buf == ';'){
			*semi = 1;
		}
		if(*buf == '|' || *buf == ';'){
			found = 1;
			*delim = '\0';
			buf = delim + 1;
			if(*buf && (*buf == ' ')){
				buf++;
			}
			continue;
		}
		if(found == 0){
			cmd1[cmd1c++] = buf;
			argv[argc++] = buf;
			*delim = '\0';
			buf = delim + 1;
		}
		else if(found == 1){
			cmd2[cmd2c++] = buf;
			argv[argc++] = buf;
			*delim = '\0';
			buf = delim + 1;
		}
		//else if {	
		
		//	argv[argc++] = buf;
		//}
		//*delim = '\0';
		//buf = delim + 1;
		while(*buf && (*buf == ' ')){   /*ignore white spaces*/
			buf++;
		}
	}
	argv[argc] = NULL;
	cmd1[cmd1c] = NULL;
	cmd2[cmd2c] = NULL;

	//ignore blank line
	if(argc == 0){
		return 1;
	}

	/* Should the job run in the backgrounf? */
	if ((bg = (*argv[argc-1] == '&')) != 0){
		argv[--argc] = NULL;
	}

	return bg;
}

// SIGINT handler
void sigint_handler(int sig){
	printf("\ncaught SIGINT!\n");
//	exit(0);
	
}

// SIGTSTP handler
void sigint_handlerZ(int sig){
	printf("\ncaught sigtstp\n");
}


//From csapp.c
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}





