void serverGET(char *filename, int socket);
void serverPUT(char *filename, int socket);
void serverLS(int socket, char* path);
void serverCWD(int socket);
void serverCD(int socket, char* path);

