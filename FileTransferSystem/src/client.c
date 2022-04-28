
#include "clientfunc.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <libgen.h>

#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"

#define MAX_ARGS 100
#define MAX_PATH 1024
#define CMD_SIZE 1024
#define CMDCOUNT 9

void help(){
	printf("FileTransfer Manual: \n"
		"--> GET <file1> <file2> ... : Download files from server\n"
		"--> PUT <file1> <file2> ... : Upload files to server\n"
		"--> LS <dir-path> : List files in dir on local device\n"
		"--> rLS <dir-path> : List files in dir on remote server\n"
		"--> CD <dir-path> : Change directory on local device\n"
		"--> rCD <dir-path> : Change directory on remote server\n"
		"--> rPWD : Get present working directory on remote server\n"
		"--> exit : Disconnect from server and exit\n\n");
}


void initShell(){
	printf("Welcome to VSK File Transfer. Command List :\n"
		"--> GET <file1> <file2> ... : Download files from server\n"
		"--> PUT <file1> <file2> ... : Upload files to server\n"
		"--> LS <dir-path> : List files in dir on local device\n"
		"--> rLS <dir-path> : List files in dir on remote server\n"
		"--> CD <dir-path> : Change directory on local device\n"
		"--> rCD <dir-path> : Change directory on remote server\n"
		"--> rPWD : Get present working directory on remote server\n"
		"--> exit : Disconnect from server and exit\n"
		"\nUse help command to print this message again.\n\n");
}


void printPrompt(char* user){
	char cwd[MAX_PATH];									//gets current working directory
    getcwd(cwd, sizeof(cwd));
	fprintf(stderr, BOLD CYAN "VSK-FileTransfer@%s:"YELLOW"%s"RESET"$ ", user, cwd);
}

char* readCommand(){
	char* cmd = NULL;
	ssize_t bufsize = 0;							//auto buffer size by getline()
	if(getline(&cmd, &bufsize, stdin) == -1){
		if (feof(stdin)) {
      		exit(EXIT_SUCCESS);  					//received EOF (CTRL + D)
    	} 
    	else {
      		perror("Error reading command.\n");
      		exit(EXIT_FAILURE);
    	}
	}
	return cmd;
}

int tokenizeCmd(char* cmd, char** args){
	const char delimiters[] = " \n";
	char* token = strtok(cmd, delimiters);
	int i = 0;
	while(token != NULL){
		args[i++] = token;
		token = strtok(NULL, delimiters);
	}
	return i;
}

int handleCmd(int socketDesc, int arg_c, char** args){
	int cmdindex = -1;
	char* cmdlist[CMDCOUNT] = {"exit", "help", "GET", "PUT", "rLS", "rPWD", "rCD", "CD", "LS"};
	for(int i=0; i<CMDCOUNT; i++){
		if(strcmp(args[0], cmdlist[i]) == 0){
			cmdindex = i;
			break;
		}
	}
	switch(cmdindex){
		case 0: 
		write(socketDesc, "exit", strlen("exit"));	
		exit(EXIT_SUCCESS);
		break;

		case 1:
		help();
		break;

		case 2:
		if(arg_c == 1){
			printf("Usage: GET <file> ...\n");
			break;
		}
		for(int i=1; i<arg_c; i++){
			GET(args[i] , socketDesc);
		}
		break;

		case 3:
		if(arg_c == 1){
			printf("Usage: PUT <file> ...\n");
			break;
		}
		for(int i=1; i<arg_c; i++){
			PUT(args[i] , socketDesc);
		}
		break;

		case 4:
		if(arg_c == 1){
			rLS(socketDesc, ".");
			break;
		}
		rLS(socketDesc, args[1]);
		break;

		case 5:
		rPWD(socketDesc);
		break;

		case 6:
		if(arg_c == 1){
			printf("Usage: rCD <dir-path> ...\n");
			break;
		}
		rCD(socketDesc, args[1]);
		break;

		case 7:
		if(arg_c == 1){
			printf("Usage: CD <dir-path> ...\n");
			break;
		}
		CD(args[1]);
		break; 

		case 8:
		if(arg_c == 1){
			LS(".");
			break;
		}
		LS(args[1]);
		break;


		default:
		printf("\n%s: Invalid command.\n", args[0]);
		return -1;
	}
	return 0;
}


int main(int argc, char* argv[]){

	if(argc!=3){
		printf("Invalid arguments\n");
		return 0;
	}

	struct sockaddr_in server;
	char msg_request[BUFSIZ], msg_reply[BUFSIZ], filename[BUFSIZ];

	//Create Socket
	int socketDesc = socket(AF_INET, SOCK_STREAM, 0);
	if(socketDesc == -1){
		perror("Couldn't create socket.\n");
		return -1;
	}
	char serverIP[BUFSIZ];
	int serverPort;
	strcpy(serverIP, argv[1]);
	serverPort = atoi(argv[2]);

	server.sin_addr.s_addr = inet_addr(serverIP);		//converts string to ip address
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPort);				//converts port to network byte order

	// Connect to server
	if (connect(socketDesc, (struct sockaddr *)&server, sizeof(server)) < 0){
		perror("Connection to server failed.\n");
		return 1;
	}
	
	char clientUser[BUFSIZ], clientPass[BUFSIZ];
	char serverResp[BUFSIZ];
	int t = recv(socketDesc, serverResp, BUFSIZ, 0); serverResp[t] = '\0';
	printf("%s", serverResp);
	scanf(" %s", clientUser);
	write(socketDesc, clientUser, strlen(clientUser));
	t = recv(socketDesc, serverResp, BUFSIZ, 0); serverResp[t] = '\0';
	printf("%s", serverResp);
	scanf(" %s", clientPass);
	write(socketDesc, clientPass, strlen(clientPass));
	t = recv(socketDesc, serverResp, BUFSIZ, 0); serverResp[t] = '\0';
	
	if(!strncmp(serverResp, "false", 5)){
		printf("Authentication Failed!\n");
		return -1;
	}

	printf("Connection Successful.\n\n");

	char *command;
    char* arguements[MAX_ARGS];
    initShell();
    getchar();
    while(1){
        printPrompt("vishesh");
        command = readCommand();
        if(!command){
            exit(EXIT_SUCCESS);
        }
        if(command[0] == '\0' || strcmp(command, "\n") == 0){
            free(command);
            continue;
        }
        int argCount = tokenizeCmd(command, arguements);				//return number of arguements
        handleCmd(socketDesc, argCount, arguements);
        free(command);
    }
    exit(EXIT_SUCCESS);

}




