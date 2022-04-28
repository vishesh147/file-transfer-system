#include "serverfunc.h"
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

#define MAXTHREADS 10
#define CMD_SIZE 1024
#define PATHSIZE 100
#define MAX_ARGS 100


void* ConnectionHandler(void *socketDesc);
int GetCommandFromRequest(char* request);
char* GetArgumentsFromRequest(char* request);
int tokenizeRequest(char* cmd, char** args);
bool checkAuth(int socketDesc);


int main(int argc, char* argv[]){

	if(argc!=2){
		printf("Invalid arguments\n");
		return 0;
	}

	int c = sizeof(struct sockaddr_in);
	struct sockaddr_in server, client;
	char requestMsg[BUFSIZ], replyMsg[BUFSIZ], filename[BUFSIZ];

	//Create Socket
	int socketDesc = socket(AF_INET, SOCK_STREAM, 0);
	if(socketDesc == -1){
		perror("Error: Socket couldn't be created.\n");
		return -1;
	}

	int serverPort = atoi(argv[1]);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(serverPort);				//converts port to network byte order

	if(bind(socketDesc, (struct sockaddr *)&server, sizeof(server)) < 0){
		perror("Bind failed.\n");
		return -1;
	}

	listen(socketDesc, 3);
	pthread_t threads[MAXTHREADS];
	int i = 0;
	int socketClient;
	int* newSocket;

	int t;
	char auth[BUFSIZ];
	
	while(socketClient = accept(socketDesc, (struct sockaddr *)&client, (socklen_t*)&c)){
		if(i >= MAXTHREADS){
			break;
		}
		pthread_t snifferThread;
		newSocket = malloc(1);
		*newSocket = socketClient;
		pthread_create(&threads[i], NULL, ConnectionHandler, (void*)newSocket);
		i++;
	}

	for(int i=0; i<MAXTHREADS; i++){
		pthread_join(threads[i], NULL);
	}

	if(socketClient<0){
		perror("Accept failed.\n");
		return -1;
	}

	return 0;
}


// Callback when a new connection is set up
void* ConnectionHandler(void *socketDesc){
	int	choice;
	int socket = *(int*)socketDesc;

	char reply[BUFSIZ], serverResp[BUFSIZ], clientRequest[BUFSIZ], filename[BUFSIZ], path[BUFSIZ];
	char* args[MAX_ARGS];
	char *data;
	printf("Connection request recieved. Waiting for Authentication.\n");
	if(!checkAuth(socket)){
		printf("Authentication Failed!\n\n");
		free(socketDesc);
		pthread_exit(0);
		return 0;
	}
	printf("\nAuthentication Successful. New client connected.\n");
	while(1){	
		printf("\nWaiting for command...\n");
		int l = recv(socket, clientRequest, BUFSIZ, 0);
		clientRequest[l]='\0';
		printf("Command Recieved: %s\n", clientRequest);
		int argCount = tokenizeRequest(clientRequest, args);
		choice = GetCommandFromRequest(clientRequest);
		switch(choice){
			case 1:
				if(argCount < 2){
					printf("Error: Invalid arguments\n");
					break;
				}
				for(int i=1; i<argCount; i++){
					strcpy(filename, args[i]);
					serverGET(filename, socket);
				}
				break;
			
			case 2:
				if(argCount < 2){
					printf("Error: Invalid arguments\n");
					break;
				}
				for(int i=1; i<argCount; i++){
					strcpy(filename, args[i]);
					serverPUT(filename, socket);
				}
				break;
			
			case 3:
				if(argCount == 1){
					serverLS(socket, ".");
					break;
				}
				strcpy(path, args[1]);
				serverLS(socket, path);
				break;

			case 4:
				serverCWD(socket);
				break;

			case 5:
				if(argCount < 2){
					printf("Error: Invalid arguments\n");
					break;
				}
				strcpy(path, args[1]);
				serverCD(socket, path);
				break;

			case 6:
				printf("\nClient Disconnected\n\n\n");
				free(socketDesc);   
				return 0;
		}
	}
	free(socketDesc);   
	pthread_exit(0);
	return 0;
}


int GetCommandFromRequest(char* request){
	char cmd[CMD_SIZE];
	strcpy(cmd, request);
	int i = 0;
	while(request[i] != ' ' && request[i] != '\0')
		i++;

	strncpy(cmd, request, i-1);
	cmd[i] = '\0';
		
	if(!strcmp(cmd, "GET"))
		return 1;
	else if(!strcmp(cmd, "PUT"))
		return 2;
	else if(!strcmp(cmd, "rLS"))
		return 3;
	else if(!strcmp(cmd, "rPWD"))
		return 4;
	else if(!strcmp(cmd, "rCD"))
		return 5;
	else if(!strcmp(cmd, "exit") || !strcmp(cmd, "EXIT"))
		return 6;
	return 0;
}

char* GetArgumentsFromRequest(char* request){
	char *arg = strchr(request, ' ');
	return arg + 1;
}

int tokenizeRequest(char* cmd, char** args){
	const char delimiters[] = " \n";
	char* token = strtok(cmd, delimiters);
	int i = 0;
	while(token != NULL){
		args[i++] = token;
		token = strtok(NULL, delimiters);
	}
	return i;
}


bool checkAuth(int socketDesc){
	int t;
	char clientUser[BUFSIZ], clientPass[BUFSIZ];
	write(socketDesc, "Enter Username: ", strlen("Enter Username: "));
	t = recv(socketDesc, clientUser, BUFSIZ, 0);
	clientUser[t] = '\0';
	write(socketDesc, "Enter Password: ", strlen("Enter Password: "));
	t = recv(socketDesc, clientPass, BUFSIZ, 0);
	clientPass[t] = '\0';
	
	FILE* fp = fopen("./auth/authdata.csv", "r");
	if(!fp){
		printf("Error opening auth file.\n");
		return 1;
	}
	char* userpass = NULL;
	ssize_t bufsize = 0;							//auto buffer size set by getline()
	bool check = false;
	while(getline(&userpass, &bufsize, fp) != -1){
		char* user = strtok(userpass, ",\n");
		char* pass = strtok(NULL, ",\n");	
		if(strcmp(clientUser, user)==0 && strcmp(clientPass, pass)==0){
			check = true;
			break;
		}
	}
	fclose(fp);
	free(userpass);

	if(check){
		write(socketDesc, "true", strlen("true"));
		return 1;
	}
	else{
		write(socketDesc, "false", strlen("false"));
		return 0;
	}

	free(clientUser);
	free(clientPass);

}