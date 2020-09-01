#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include<openssl/sha.h>
#include<regex.h>
#include "protocol.h"
//char buffer[256];
//char *fileName;
char client_message[1024];
char buffer[1024];
project * projectList;
pthread_mutex_t lock;

void * pthread (void *newSock);
unsigned char * SHA256_hash_file(char *);
void createManifestFile(char *);
void createManifest(char *, int);

char * itoa(int num, char *str){
	if (str == NULL)
    	return NULL;
    	
   	sprintf(str,"%d",num);
   	return str;
}

// generates SHA256 hash of a file
unsigned char * SHA256_hash_file (char * filename) {
	int fd = open(filename, O_RDONLY);
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	char buff;
	char * input = (char *) malloc((size)*sizeof(char));
	int i;
	
	unsigned char * hash = (unsigned char *) malloc(SHA256_DIGEST_LENGTH*sizeof(unsigned char));
	for (i=0; i<size; i++) {
		read(fd, &buff, sizeof(buff));
		input[i] = buff;
	}
	
	SHA256(input, size, hash);
	return hash;
};

void createNewManifest (char * dirpath) {
	char path[strlen(dirpath)+12];
	snprintf(path, sizeof(path), "%s/.Manifest", dirpath);
	int fd = open(path, O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
	char c[2] = "1\n";
	write(fd, c, 2);
	close(fd);
}

void createManifestFile (char * dirpath) {
	char path[strlen(dirpath)+12];
	snprintf(path, sizeof(path),"%s/.Manifest", dirpath);
	int fd = open(path, O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
	createManifest(dirpath, fd);
};

void createManifest (char * dirpath, int fd) {
	DIR * proj = opendir(dirpath);
	struct dirent * entry;
	
	while (entry = readdir(proj)) {
		if (entry->d_type == DT_DIR) {
			char path[1024];
		    	if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		        	continue;
		    	snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
			createManifest(path, fd);
		} else {
			if (strcmp(entry->d_name, ".Manifest") != 0) {
				char path[1024];
				snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
				path[strlen(dirpath)+strlen(entry->d_name)+1] = '\0';
				write(fd, path, strlen(path));
				char s = ' ';
				write(fd, &s, sizeof(s));
				unsigned char * hash = SHA256_hash_file(path);
				char * write_hash = (char *) malloc(2*sizeof(char));
				int i;
				for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
					snprintf(write_hash, sizeof(write_hash), "%02.2x", hash[i]);
					write(fd, &write_hash[0], sizeof(write_hash[0]));
					write(fd, &write_hash[1], sizeof(write_hash[1]));
				}
				char n = '\n';
				write(fd, &n, sizeof(n));
			}
        	}
	}
};

void * socketThread (void * arg) {
	int newSock = *((int *) arg);
	
	//pthread_mutex_lock(&lock);
	int i;
	int n=0;
	int start, end, length;
	char buff;
	char * message = (char *) malloc(1024*sizeof(char));
	/*for (i=0; i<1023; i++) {
		n = read(newSock, &buff, 1);
	}*/
	n = read(newSock, message, 1023);
	message[n] = '\0';
	char * cmd = (char *) malloc(4*sizeof(char));
	for (i=0; i<3; i++) {
		cmd[i] = message[i];
	}
	cmd[3] = '\0';
	if (n < 0) {
		printf("error reading from socket\n");
	}
	
	printf("%s\n", cmd);
	printf("%s\n", message);
	
	char * projnm;
	
	if (strcmp(cmd, "crt") == 0 || strcmp(cmd, "dst") == 0 || strcmp(cmd, "crv") == 0 || strcmp(cmd, "cmt") == 0 | strcmp(cmd, "psh") == 0) {
		char namelength[30];
		int nameL;
		for (i=4; i<strlen(message); i++) {
			if (message[i] == ':') {
				break;
			}
			namelength[i-4] = message[i];
		}
		namelength[i-4] = '\0';
		//printf("%s\n", namelength);
		nameL = atoi(namelength);
		char * projname = (char *) malloc(30*sizeof(char));
		for (i=4+strlen(namelength)+1; i<strlen(message); i++) {
			//printf("%c", message[i]);
			projname[i-(4+strlen(namelength)+1)] = message[i];
		}
		projname[i-(4+strlen(namelength)+1)] = '\0';
		char projpath[12+strlen(projname)+2+1];
		snprintf(projpath, 12+strlen(projname)+1, "server_repo/%s", projname);
		printf("%s\n", projpath);
		projnm = (char *) malloc((12+strlen(projname)+2+1)*sizeof(char));
		snprintf(projnm, 12+strlen(projname)+2+1, "%s", projpath);
		if (strcmp(cmd, "crt") == 0) {
			char projvpath[12+strlen(projname)+2+1];
			snprintf(projvpath, 12+strlen(projname)+2+1, "server_repo/%s/1", projname);
			printf("%s\n", projvpath);
			mkdir(projpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			mkdir(projvpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			createNewManifest(projvpath);
			
			project * newproj = (project *) malloc(sizeof(project));
			newproj->name = (char *) malloc((strlen(projname)+1)*sizeof(char));
			strcpy(newproj->name, projname);
			newproj->currVersion = 1;
			newproj->next = NULL;
			
			if (projectList == NULL) {
				projectList = newproj;
				//printf("%s\n", projectList->name);
			} else {
				project * ptr = projectList;
				while (ptr->next != NULL) {
					//printf("%s\n", ptr->name);
					ptr = ptr->next;
				}
				ptr->next = newproj;
				/*ptr = projectList;
				while (ptr != NULL) {
					printf("%s\n", ptr->name);
					ptr = ptr->next;
				}*/
			}
		} else if (strcmp(cmd, "dst") == 0) {
			char sysc[7+strlen(projpath)+1];
   			snprintf(sysc, 7+strlen(projpath)+1, "rm -rf %s", projpath);
   			//printf("%s\n", sysc);
   			system(sysc);
   		} else if (strcmp(cmd, "crv") == 0) {
   			project * ptr = projectList;
   			while (ptr != NULL) {
   				if (strcmp(ptr->name, projname) == 0) {
   					printf("%d\n", ptr->currVersion);
   				}
   				ptr = ptr->next;
   			}
   		}
   		if (strcmp(cmd, "crt") == 0 || strcmp(cmd, "upd") == 0 || strcmp(cmd, "upg") == 0 || strcmp(cmd, "cmt") == 0) {
   			project * ptr = projectList;
   			while (ptr != NULL) {
   				if (strcmp(ptr->name, projname) == 0) {
   					break;
   				}
   				ptr = ptr->next;
   			}
   			int digits=0;
   			int num2 = ptr->currVersion;
   			int num = ptr->currVersion;
   			while (num2 != 0) {
   				num2 = num2/10;
   				digits++;
   			}
   			char * numstr = (char *) malloc((num+1)*sizeof(char));
   			numstr = itoa(num, numstr);
   			char manifestpath[strlen(projpath)+1+digits+10+1];
   			snprintf(manifestpath, strlen(projpath)+1+digits+10+1, "%s/%s/.Manifest", projpath, numstr);
   			char * manifestcpy;
   			int manifd = open(manifestpath, O_RDONLY, S_IWUSR | S_IRUSR);
			int size = lseek(manifd, 0, SEEK_END);
			lseek(manifd, 0, SEEK_SET);
			manifestcpy = (char *) malloc((size+1)*sizeof(char));
			for (i=0; i<size; i++) {
				read(manifd, &manifestcpy[i], 1);
			}
			manifestcpy[i] = '\0';
			close(manifd);
			
			digits=0;
   			num2 = size;
   			num = size;
   			while (num2 != 0) {
   				num2 = num2/10;
   				digits++;
   			}
   			char * sizestr = (char *) malloc((num+1)*sizeof(char));
   			sizestr = itoa(num, sizestr);
			
			char * response = (char *) malloc((3+1+strlen(sizestr)+1+strlen(manifestcpy)+1)*sizeof(char));
			snprintf(response, 3+1+strlen(sizestr)+1+strlen(manifestcpy)+1, "sdm:%s:%s", sizestr, manifestcpy);
			n = write(newSock, response, strlen(response)+1);
			free(response);
		}
		if (strcmp(cmd, "psh") == 0) {
			
		}
   		free(projname);
	}
	free(message);
	n = write(newSock, "I got your message", 19);
	if (n < 0) {
		printf("error writing to socket\n");
	}
	
	if (strcmp(cmd, "cmt") == 0) {
		char * comreceive = (char *) malloc(1024*sizeof(char));
	
		n = read(newSock, comreceive, 1023);
		comreceive[n] = '\0';
		printf("%s\n", comreceive);
		
		printf("%s\n", projnm);
		
		char projcdir[12+strlen(projnm)+8+1];
		snprintf(projcdir, strlen(projnm)+8+1, "%s/Commits", projnm);
		mkdir(projcdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		
		char projcname[strlen(projnm)+8+6+1];
		snprintf(projcname, strlen(projnm)+8+6+1, "%s/Commits/comm1", projnm);
		int cfd = open(projcname, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
		
		for (i=0; i<strlen(comreceive); i++) {
			write(cfd, &comreceive[i], 1);
		}
		
		close(cfd);
		
		free(comreceive);
		n = write(newSock, "Commit received\n", 15);
	}
	
	free(cmd);
	//pthread_mutex_unlock(&lock);
	
	/*recv(newSock, client_message, 1024, 0);
	printf("%s\n", client_message);
	
	pthread_mutex_lock(&lock);
	char * message = malloc(sizeof(client_message)+20);
	strcpy(message, "Hello Client: ");
	strcat(message, client_message);
	strcat(message, "\n");
	strcpy(buffer, message);
	free(message);
	pthread_mutex_unlock(&lock);
	
	sleep(1);
	send(newSock, buffer, 13, 0);*/
	printf("Exit socketThread\n");
	close(newSock);
	pthread_exit(NULL);
};

/*char *itoa(int num, char * str)
{
	if (str == NULL)
	return NULL;
	sprintf(str, "%d", num);
	return str;
};
void FileContent(char*fileName, int socket)
{
	char numofbytes[20];
	char wrongFileMessage [] = "Wrong file name\n";
	//char fileName;

	int fd = open(fileName, O_RDONLY);
	if (fd < 0)
	{
		write(socket, wrongFileMessage, strlen(wrongFileMessage));
		printf("Wrong file name\n");
	}
	else
	{
		write(socket, "opened file\n", sizeof("opened file\n"));
		printf("file name sent\n");
		int status = 0;
		char buffer;
		int bytes = 0;
		do
		{
			status = read(fd, &buffer, 1);
			bytes = bytes + 1;
		}
		while(status > 0);
			itoa (bytes, numofbytes);
			int i = 0;
			
			while (numofbytes[i] != '\0')
				i++;
			write(socket, numofbytes, i);
			memset(numofbytes, '\0', i);
			i = 0;

			char * text = (char*)malloc(sizeof(char)*bytes+1);
			fd = open(fileName, O_RDONLY);
			if (fd < 0)
			{
				printf("Failed to open file\n");
				exit(-1);
			}
				memset(fileName, '\0', strlen(fileName));
				bytes = 0;
				do
				{
					status = read(fd, &buffer, 1);
					text[bytes] = buffer;
					bytes = bytes + 1;
				}
				while(status > 0);
				write(socket, text, bytes);
				memset(text, '\0', bytes);
		}	
	}
};*/
void thread(char * port) {
	int maxNumbClients = 50, newSock;
	int portNumber;
	struct sockaddr_in addressOfServer;
	struct sockaddr_in addressOfNewConnection;
	int serverSock;
	//Create Socket
	portNumber = atoi(port);
	socklen_t addr_size;

	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock < 0) {
		printf("Connection failed\n");
		exit(-1);
	}
	printf("The server socket was created\n");
	memset(&addressOfServer, '\0', sizeof(addressOfServer));
	addressOfServer.sin_family = AF_INET;
	addressOfServer.sin_addr.s_addr = INADDR_ANY;
	addressOfServer.sin_port = htons(portNumber);
	if(bind(serverSock, (struct sockaddr *)&addressOfServer, sizeof(addressOfServer))<0) {
		printf("Error, couldn't bind to the port\n");
		exit(-1);
	}
	printf("Binded successfully with port %d\n", portNumber);
	//Listen
	if (listen(serverSock,maxNumbClients) < 0) {
		printf("Socket Failed\n");
		exit(-1);
	}
	
	mkdir("./server_repo", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	projectList = NULL;
	
	/*int length = 0;
	char * message = NULL;*/
	int i=0;
	pthread_t tid[60];
	pthread_t id;
	pthread_mutex_init(&lock, NULL);
	//Accept
	while (1) {
		printf("Connection awaiting\n");
		newSock = accept(serverSock, (struct sockaddr *) &addressOfNewConnection, &addr_size);
		
		if (newSock >= 0) {
			printf("new client with IP %s and %d connected\n", inet_ntoa(addressOfNewConnection.sin_addr), ntohs(addressOfNewConnection.sin_port));
		}
		
		if(pthread_create(&tid[i], NULL, socketThread, &newSock) != 0) {
			printf("Failed to create thread\n");
		}
		
		pthread_join(tid[i++], NULL);
		
		if (i>=50) {
			i=0;
			while (i<50) {
				pthread_join(tid[i++], NULL);
			}
			i=0;
		}
	}
};
/*void checkout(char* project, int sock) {
    if(exists(".", project) == 1) {
        printf("Already have project\n");
        //return;
    }
    send(sock, "checkout ", 9, 0);
    send(sock, project, strlen(project), 0); //project name
    char buffer[1024];
    bzero(buffer, 1024);
    read(sock, buffer, 1024);
    if(strcmp(buffer, "project not found") == 0) {
        printf("Message from server: %s\n", buffer);
        bzero(buffer, 1024);
        return;
    }
    else {
        mkdir(project, 0777);
    }
    int n;
    
    send(sock, "next", 5, 0);
    while(1) {
        //send(sock, "next", 5, 0);
        bzero(buffer, 1024);
        n = read(sock, buffer, 1024); //file path
        if(strcmp(buffer, "done") == 0) { //why ???
            break;
        }
        char* filepath = (char*)malloc(256*sizeof(char));
        int i, j;
        for(i = 0; i < strlen(buffer); i++) {
            j = i+9;
            filepath[i] = buffer[j];
        }
        char* path = dirpath(project, filepath);
        printf("path: %s\n", path);
        mkdir(path, 0777);
        printf("file %s\n", filepath);
        int fd = open(filepath, O_RDWR | O_TRUNC | O_CREAT, 0644);
        send(sock, "next", 5, 0);
        bzero(buffer, 1024);
        read(sock, buffer, 1024); //contents
        //printf("contents %s\n", buffer);
        if(strcmp(buffer, "empty\a") != 0) {
            write(fd, buffer, strlen(buffer));
        }
        send(sock, "next", 5, 0);
    }
    bzero(buffer, 1024);
}*/
int main (int argc, char **argv) {

	if (argc < 2) {
		printf(" incorrect number of arguments\n");
		return -1;
	}
	thread(argv[1]);

	return 0;
}
