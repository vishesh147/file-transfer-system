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

pthread_mutex_t lock;

bool SendFileOverSocketServer(int socketDesc, char* filename){

	struct stat	obj;
	int fileDesc, fileSize;

	printf("Sending file: "BOLD BLUE"%s"RESET" ...\n", filename);
	if(stat(filename, &obj) != 0){
		printf("Error opening file\n");
		return 0;
	}
	FILE* fp = fopen(filename, "rb");
	fileDesc = fileno(fp);					// Open file
	fileSize = obj.st_size;				   // Save object size

	write(socketDesc, &fileSize, sizeof(int));		// Send filesize 

	sendfile(socketDesc, fileDesc, NULL, fileSize);  	// Send file

	printf("File sent successfully.\n\n");
	return true;
}


void serverGET(char *filename, int socket){
	pthread_mutex_lock(&lock);
	char serverResp[BUFSIZ], clientResp[BUFSIZ];
	printf("Performing client request: GET\n");

	// Check if file exists
	if (access(filename, F_OK) != -1){
		strcpy(serverResp, "OK"); 								// Send OK if file exists
		write(socket, serverResp, strlen(serverResp));
		int t = recv(socket, clientResp, BUFSIZ, 0);
		clientResp[t] = '\0';
		SendFileOverSocketServer(socket, filename);     		// Send File
	}
	else{
		printf("Error: File does not exist at server. ABORT called.\n");
		strcpy(serverResp, "ABORT");			// Sending ABORT if failed 
		write(socket, serverResp, strlen(serverResp)); 
	}
	pthread_mutex_unlock(&lock);
	return;
}


void serverPUT(char *filename, int socket){
	pthread_mutex_lock(&lock);
	int c, r;

	char* basec, *bfilename;
	basec = strdup(filename);
	bfilename = basename(basec);

	printf("Performing client request: PUT\n");

	char serverResp[BUFSIZ], clientResp[BUFSIZ];
	if(access(bfilename, F_OK) != -1){
		strcpy(serverResp, "NOK");			// Notify client that file already exists on remote server.
		write(socket, serverResp, strlen(serverResp));

		r = recv(socket, clientResp, BUFSIZ, 0);
		clientResp[r]='\0';

		if(!strncmp(clientResp, "N", 1)){
			printf("PUT request aborted. "BOLD BLUE"%s"RESET" already exists on server.\n", bfilename);
			pthread_mutex_unlock(&lock);
			return;
		}
		printf("Proceeding to overwrite file.\n");
		strcpy(serverResp, "OK");
		write(socket, serverResp, strlen(serverResp));
	}
	else{
		// Send acknowledgement "OK"
		strcpy(serverResp, "OK");
		write(socket, serverResp, strlen(serverResp));
	}
	
	printf("Receiving Data...\n");
	int fileSize;
	char *data;
	
	recv(socket, &fileSize, sizeof(int), 0);			// Receiving file size and allocating memory
	data = malloc(fileSize+1);

	FILE *fp = fopen(bfilename, "wb");						// Create file with given 'filename' (basname) and open it. 
	r = recv(socket, data, fileSize, 0);
	data[r] = '\0';
	printf("File: "BOLD BLUE"%s"RESET" Received; Size: %d bytes\n", bfilename, r);
	r = fputs(data, fp);
	fclose(fp);
	pthread_mutex_unlock(&lock);
}


void serverLS(int socket, char* path){
	pthread_mutex_lock(&lock);
	DIR* dr = opendir(path);
	struct dirent* de;
	if (dr == NULL){
		printf("Error opening dir.\n");
		printf("%d", errno);
		write(socket, "error", strlen("error"));
        pthread_mutex_unlock(&lock);
        return;
	}
	char clientResp[BUFSIZ], serverMsg[BUFSIZ];
	int total = 0;
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
    		if(S_ISREG(stbuf.st_mode)){
    			strcpy(serverMsg, BOLD);
    			strcat(serverMsg, GREEN);
    			strcat(serverMsg, de->d_name);
    			strcat(serverMsg, RESET);
    		} 
    	    else if(S_ISDIR(stbuf.st_mode)){
    	    	strcpy(serverMsg, BOLD);
    			strcat(serverMsg, BLUE);
    			strcat(serverMsg, de->d_name);
    			strcat(serverMsg, RESET);
    	    }
    	    else if(S_ISLNK(stbuf.st_mode)){
    	    	strcpy(serverMsg, BOLD);
    			strcat(serverMsg, RED);
    			strcat(serverMsg, de->d_name);
    			strcat(serverMsg, RESET);
    	    }
    	    //printf("%s\n", de->d_name);
    	    write(socket, serverMsg, strlen(serverMsg));
    	    int r = recv(socket, clientResp, BUFSIZ, 0);
    	    total++;
    	}
    }
    char tot[32];
    sprintf(tot, "Total: %d", total);
    write(socket, tot, strlen(tot));
    closedir(dr);
    pthread_mutex_unlock(&lock);
    return;
}

void serverCWD(int socket){
	pthread_mutex_lock(&lock);
	char cwd[BUFSIZ];
	getcwd(cwd, sizeof(cwd));
	write(socket, cwd, strlen(cwd));
	pthread_mutex_unlock(&lock);
    return;
}

void serverCD(int socket, char* path){
	pthread_mutex_lock(&lock);
	char cwd[BUFSIZ];
	chdir(path);
	getcwd(cwd, sizeof(cwd));
	write(socket, cwd, strlen(cwd));
	pthread_mutex_unlock(&lock);
    return;
}

