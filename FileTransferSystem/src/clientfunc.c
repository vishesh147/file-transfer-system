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


int SendFileOverSocket(int socketDesc, char* filename){
	struct stat	obj;
	int fileDesc, fileSize;
	printf("Sending file: "BOLD BLUE"%s"RESET" ...\n", filename);
	if(stat(filename, &obj) != 0){
		printf("Error opening file\n");
		return 0;
	}
	FILE* fp = fopen(filename, "rb");
	fileDesc = fileno(fp);
	fileSize = obj.st_size;
	write(socketDesc, &fileSize, sizeof(int));
	sendfile(socketDesc, fileDesc, NULL, fileSize);
	printf("File sent successfully.\n\n");
	return 1;
}

void GET(char* filename, int socketDesc){
	char* basec, *bfilename;
	basec = strdup(filename);
	bfilename = basename(basec);
	char requestMsg[BUFSIZ], replyMsg[BUFSIZ];
	int fileSize;
	char* data;
	if(access(bfilename, F_OK) != -1){
		char flag;
		printf("File already exists locally. Do you want to overwrite the existing file ? "BOLD GREEN"Y"RESET"/"BOLD RED"N"RESET"\n");
		scanf("%c", &flag);
		getchar();
		if(flag == 'N'){
			printf("GET request aborted. %s already exists on local device.\n", bfilename);
			return;	
		}
	}
	strcpy(requestMsg, "GET ");				
	strcat(requestMsg, filename);				// Build request message.

	write(socketDesc, requestMsg, strlen(requestMsg));

	int t;
	t = recv(socketDesc, replyMsg, BUFSIZ, 0);
	replyMsg[t]='\0';

	if (strncmp(replyMsg, "OK", 2) == 0){
	
		printf("Receiving Data...\n");      				// File exists on Server.
		write(socketDesc, "OK", strlen("OK"));
		recv(socketDesc, &fileSize, sizeof(int), 0);		// Receiving file size and allocating memory
		data = malloc(fileSize+1);
		
		FILE *fp = fopen(bfilename, "wb"); 				   // Create file with given 'filename' and open it. 
		t = recv(socketDesc, data, fileSize, 0);
		data[t] = '\0';
		fputs(data, fp);								 // Write data received to file.
		fclose(fp);	
		printf("File: "BOLD BLUE"%s"RESET" Received; Size: %d bytes\n", bfilename, t);
	}
	else{
		printf("Error performing GET request. File does not exist at server.\n");
	}
}

void PUT(char *filename, int socketDesc){
	int	fileSize, fileDesc, t;
	char c;
	char *data;
	char requestMsg[BUFSIZ], replyMsg[BUFSIZ], clientResp[2];
	// Get a file from server
	strcpy(requestMsg, "PUT ");
	strcat(requestMsg, filename);
	printf("Trying to PUT %s to server. \n", filename);
	if (access(filename, F_OK) != -1){
		// Sending PUT request to server.
		write(socketDesc, requestMsg, strlen(requestMsg));
		t = recv(socketDesc, replyMsg, BUFSIZ, 0);
		replyMsg[t]='\0';
		if (strncmp(replyMsg, "OK", 2) == 0){
			// Proceed to Send file
			SendFileOverSocket(socketDesc, filename);
		}
		else if(strncmp(replyMsg, "NOK", 3) == 0){
			// File present at server.
			printf("File already exists on remote server. Do you want to overwrite the existing file ? "BOLD GREEN"Y"RESET"/"BOLD RED"N"RESET"\n");
			scanf("%c", &c);
			getchar();
			if(c == 'Y'){			// Overwrite file and send 'Y' to server	
				printf("Overwriting existing file: %s\n",filename );
				strcpy(clientResp, "Y");
				write(socketDesc, clientResp, strlen(clientResp));
				t = recv(socketDesc, clientResp, BUFSIZ, 0);
				clientResp[t] = '\0';
				SendFileOverSocket(socketDesc, filename);
				printf("File uploaded successfully.\n");
			}
			else{					// Send 'N' to server and exit
				printf("ABORT: PUT operation aborted.\n");
				strcpy(clientResp, "N");
				write(socketDesc, clientResp, strlen(clientResp));
				return;
			}
		}
		else{
			// Server can't create file.
			printf("Error: File upload failed.\n");
		}
	}
	else{
		// File not found locally.
		printf("Error: File does not exist on local device.\n");
		return;
	}
}


void rPWD(int socketDesc){
	int t;
	char requestMsg[BUFSIZ], recvMsg[BUFSIZ];
	strcpy(requestMsg, "rPWD");
	write(socketDesc, requestMsg, strlen(requestMsg));
	t = recv(socketDesc, recvMsg, BUFSIZ, 0);
	recvMsg[t]='\0';
	printf("Remote present working dir: "BOLD GREEN"%s"RESET"\n", recvMsg);
	return;
}


void rLS(int socketDesc, char* path){
	int t;
	char requestMsg[BUFSIZ], recvMsg[BUFSIZ];
	strcpy(requestMsg, "rLS ");
	strcat(requestMsg, path);
	write(socketDesc, requestMsg, strlen(requestMsg));
	while((t = recv(socketDesc, recvMsg, BUFSIZ, 0)) != 0){
		recvMsg[t]='\0';
		if(!strncmp(recvMsg, "Total", 5)){
			break;
		}
		if(!strncmp(recvMsg, "error", 5)){
			printf("Error: Invalid Path\n");
			break;
		}
		printf(GREEN "%s\n" RESET, recvMsg);
		write(socketDesc, "success", strlen("success"));
	}
	printf("\n%s\n", recvMsg);
	return;
}

void rCD(int socketDesc, char* path){
	if(!path){
		printf("Error: Invalid path\n");
		return;
	}
	int t;
	char requestMsg[BUFSIZ], recvMsg[BUFSIZ];
	strcpy(requestMsg, "rCD ");
	strcat(requestMsg, path);
	write(socketDesc, requestMsg, strlen(requestMsg));
	t = recv(socketDesc, recvMsg, BUFSIZ, 0);
	recvMsg[t]='\0';
	printf("Remote dir changed to: "BOLD GREEN"%s"RESET"\n", recvMsg);
	return;
}

void CD(char* path){
	if(!path){
		printf("Error: Invalid path\n");
		return;
	}
	chdir(path);
	printf("Changed local working dir successfully.\n");
}

void LS(char* path){
	DIR* dr = opendir(path);
	struct dirent* de;
	int total = 0;
	if (dr == NULL){
		printf("Error opening dir.\n");
        return;
	}
	char npath[BUFSIZ];
	struct stat stbuf;
    while((de = readdir(dr)) != NULL){
    	if (strncmp(de->d_name, ".", 1) && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0){
    		strcpy(npath, path);
    		strcat(npath, "/");
    		strcat(npath, de->d_name);
    		if(stat(npath, &stbuf) != 0){
    			printf("Error opening stat.\n");
    		}
    		if(S_ISREG(stbuf.st_mode)) printf(BOLD GREEN "%s\n" RESET, de->d_name);
    	    else if(S_ISDIR(stbuf.st_mode)) printf(BOLD BLUE "%s\n" RESET, de->d_name);
    	    else if(S_ISLNK(stbuf.st_mode)) printf(BOLD RED "%s\n" RESET, de->d_name);
    		total++;
    	}
    }
    closedir(dr);
    printf("\nTotal: %d\n", total);
    return;
}