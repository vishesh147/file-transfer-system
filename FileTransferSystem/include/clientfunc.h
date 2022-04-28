void GET(char *filename, int socketDesc);
void PUT(char *file_name, int socket_desc);
void rPWD(int socketDesc);
void rLS(int socketDesc, char* path);
void rCD(int socketDesc, char* path);
void CD(char* path);
void LS(char* path);
