#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<openssl/sha.h>
#include<netdb.h>
#include<regex.h>
#include<dirent.h>
#include"zlib.h"
#include "protocol.h"
//#define ZLIB_VERSION "1.2.11"
//#define ZLIB_VERNUM 0x12b0
#define PORT 8080

unsigned char * SHA256_hash_file(char *);
void createManifestFile(char *);
void createManifest(char *, int);
void newManifestEntry(int, char *);
struct node* fileNames;

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

void createManifestFile (char * dirpath) {
	char path[strlen(dirpath)+12];
	snprintf(path, sizeof(path),"%s/.Manifest", dirpath);
	int fd = open(path, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
	close(fd);
	//createManifest(dirpath, fd);
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

void newManifestEntry (int fd, char * filepath) {
	write(fd, "1 ", 2);
	write(fd, filepath, strlen(filepath));
	write(fd, " ", 1);
	unsigned char * hash = SHA256_hash_file(filepath);
	char * write_hash = (char *) malloc(2*sizeof(char));
	int i;
	for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
		snprintf(write_hash, sizeof(write_hash), "%02.2x", hash[i]);
		write(fd, &write_hash[0], sizeof(write_hash[0]));
		write(fd, &write_hash[1], sizeof(write_hash[1]));
	}
	char n = '\n';
	write(fd, &n, sizeof(n));
	close(fd);
};

void addcmd (char * proj, char * filename) {
	char * filepath = (char *) malloc((strlen(proj)+strlen(filename)+2)*sizeof(char));
	snprintf(filepath, strlen(proj)+strlen(filename)+2, "%s/%s", proj, filename);
	char * manifestpath = (char *) malloc((strlen(proj)+11)*sizeof(char));
	snprintf(manifestpath, strlen(proj)+11, "%s/.Manifest", proj);
	printf("%s\n", manifestpath);
	if (open(filepath, O_RDWR, S_IWUSR | S_IRUSR) >= 0) {
		printf("Error: File already exists\n");
	} else {
		int fd = open(filepath, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
		close(fd);
		fd = open(manifestpath, O_RDWR | O_APPEND, S_IWUSR | S_IRUSR);
		newManifestEntry(fd, filepath);
		printf("File %s created\n", filepath);
	}
};

void rmManifestEntry(FILE * fp, char * filepath, char * manifestpath) {
	char * version = (char *) malloc(30*sizeof(char));
	char * currfile = (char *) malloc(50*sizeof(char));
	char * hash = (char *) malloc(SHA256_DIGEST_LENGTH*sizeof(char));
	char pversion[30];
	fscanf(fp, "%s\n", pversion);
	int fd = open(".replica", O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
	write(fd, "1\n", 2);
	
	while (fscanf(fp, "%s %s", version, currfile) != EOF) {
		printf("%s\n", version);
		printf("%s\n", currfile);
		if (strcmp(version, "r") != 0) {
			fscanf(fp, " %s\n", hash);
			if (strcmp(currfile, filepath) != 0) {
				write(fd, version, strlen(version));
				write(fd, " ", 1);
				write(fd, currfile, strlen(currfile));
				write(fd, " ", 1);
				write(fd, hash, strlen(hash));
				write(fd, "\n", 1);
			} /*else {
				write(fd, "r ", 2);
				write(fd, currfile, strlen(currfile));
				write(fd, "\n", 1);
			}*/
		} /*else if (strcmp(version, "r") == 0) {
			write(fd, version, strlen(version));
			write(fd, " ", 1);
			write(fd, currfile, strlen(currfile));
			write(fd, "\n", 1);
		}*/ else {
			fscanf(fp, "\n");
		}
	}
	
	fclose(fp);
	close(fd);
	remove(manifestpath);
	rename(".replica", manifestpath);
}

void removecmd (char * proj, char * filename) {
	char * filepath = (char *) malloc((strlen(proj)+strlen(filename)+2)*sizeof(char));
	snprintf(filepath, strlen(proj)+strlen(filename)+2, "%s/%s", proj, filename);
	//if (remove(filepath) == 0) {
		char * manifestpath = (char *) malloc((strlen(proj)+11)*sizeof(char));
		snprintf(manifestpath, strlen(proj)+11, "%s/.Manifest", proj);
		FILE * fp = fopen(manifestpath, "r+");
		rmManifestEntry(fp, filepath, manifestpath);
		printf("File %s removed\n", filepath);
	//} else {
		//printf("Error: File could not be removed\n");
	//}
};

int main (int argc, char **argv) {
   	char filesize[256];
   	int clientSock =-1;
   	int portNumber =-1;
   	int numberofbytes=0;
   	int x;
   	//char filenamelength[256];
   	//int numb=0;
   	int file =-1;
   	char isFile[13];
   	//int n =-1;
   	char buffer[1024];
   	struct sockaddr_in serverAddressInfo;
   	struct hostent * serverIPAddress;
   	
   	if (argc < 3) {
   		printf("Not enough arguments\n");
   	}
   	
   	if (strcmp(argv[1], "add") == 0) {
   		if (argc < 4) {
   			printf("not enough arguments for add\n");
   			return 0;
   		}
   		addcmd(argv[2], argv[3]);
   		return 0;
   	}
   	
   	if (strcmp(argv[1], "remove") == 0) {
   		if (argc < 4) {
   			printf("not enough arguments for remove\n");
   			return 0;
   		}
   		removecmd(argv[2], argv[3]);
   		return 0;
   	}
   	
       if (open("./.configure", O_RDONLY) == -1) {
        	if (argc < 4) {
        		printf("Not enough arguments\n");
        		return 0;
        	}
		if (strcmp(argv[1], "configure") == 0) {
			int fd = open("./.configure", O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
			write(fd, argv[2], strlen(argv[2]));
			write(fd, "\n", 1);
			write(fd, argv[3], strlen(argv[3]));
			close(fd);
			printf("Successfully configured\n");
			return 0;
		} else {
			printf("Error: No .configure file found\n");
			return 0;
		}
	}
	char hostname[40];
	FILE * fp = fopen("./.configure", "r");
	fscanf(fp, "%s\n", hostname);
	fscanf(fp, "%d", &portNumber);
	fclose(fp);
   	serverIPAddress = gethostbyname(hostname);
   	//printf("the Ip is %s\n",serverIPAddress);
   	if (serverIPAddress == NULL)
	{
		printf("Error, you provided an invalid server IP address\n");
        	exit(-1);   
   	}
   	clientSock = socket(AF_INET, SOCK_STREAM, 0);
  	if (clientSock < 0)
	{
     		 printf("Error, unable to create client socket\n");
     		 exit(-1);
        }
  	 printf("The socket was created successfully\n");
   	//bzero((char*)&serverAddressInfo,sizeof(serverAddressInfo));
  	memset(&serverAddressInfo, '\0', sizeof(serverAddressInfo));
   	serverAddressInfo.sin_port = htons(portNumber);
   	serverAddressInfo.sin_family = AF_INET;
   	bcopy((char *) serverIPAddress->h_addr, (char *)&serverAddressInfo.sin_addr.s_addr, serverIPAddress->h_length);
  	if (connect(clientSock,(struct sockaddr*)&serverAddressInfo, sizeof(serverAddressInfo))<0)
	{
       		printf("Error, unable to connect to a server\n");
       		exit(-1);
   	}
   	printf("You successfully connected to the server\n");
   	
   	char * message;
   	
   	if (strcmp(argv[1], "create") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "crt:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   		mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   	}
   	
   	if (strcmp(argv[1], "destroy") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "dst:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   	}
   	
   	if (strcmp(argv[1], "currentversion") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "crv:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   	}
   	
   	if (strcmp(argv[1], "update") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "upd:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   		mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   	}
   	
   	if (strcmp(argv[1], "upgrade") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "upg:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   		mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   	}
   	
   	if (strcmp(argv[1], "commit") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "cmt:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   		mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   	}
   	
   	if (strcmp(argv[1], "push") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "psh:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   	}
   	
	/*if (strcmp(argv[1], "checkout") == 0){
		int digits = 0;
		int num2 = strlen(argv[2]);
		int num = strlen(argv[2]);
		while(num2 != 0){
			num2 = num2/10;
			digits++;
		}
		char * numstr = (char *) malloc((num+1)*sizeof(char));
		numstr = itoa(num, numstr);
		printf("%s\n", numstr);
		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
		sprintf(message, 3+strlen(argv[2])+digits+2+1, "chk:%s:%s", numstr, argv[2]);
		message[3+strlen(argv[2])+digits+2] = '\0';
		printf("%s\n", message);
	}
   	/*if (strcmp(argv[1], "snd") == 0) {
   		int digits=0;
   		int num2 = strlen(argv[2]);
   		int num = strlen(argv[2]);
   		while (num2 != 0) {
   			num2 = num2/10;
   			digits++;
   		}
   		char * numstr = (char *) malloc((num+1)*sizeof(char));
   		numstr = itoa(num, numstr);
   		printf("%s\n", numstr);
   		message = (char *) malloc((3+strlen(argv[2])+digits+2+1)*sizeof(char));
   		snprintf(message, 3+strlen(argv[2])+digits+2+1, "snd:%s:%s", numstr, argv[2]);
   		message[3+strlen(argv[2])+digits+2] = '\0';
   		printf("%s\n", message);
   	}*/
   	
     	//printf("Enter your message: ");
      	//memset(buffer,'\0',256);
      	//scanf("%[^\n]",buffer);
     	// char * fileName = (char*) malloc (sizeof(char)*(sizeof(buffer)));
     	// fileName = buffer;      
     	//printf("file name: %s\n", buffer);*/
     	
     	int n = write(clientSock, message, strlen(message));
     	if(n < 0) {
     		printf("Send failed\n");
     	}
     	
     	memset(buffer, '0', 256);
     	n = read(clientSock, buffer, 255);
     	buffer[n] = '\0';
	//checkout
	/*if (strcmp(argv[1], "checkout") == 0)
	{
		char path[BUFF];
		char* projectName
		gzFile outfile;
		int childfd;
		sprintf(path,".server_repo%s", projectName);
		//pthread_mutex_lock(&mlock3);//Locking mutex
		if(folderExist(path))
		{
			sendMsg("Exist", childfd);
			DIR *dir = opendir(path);
			struct dirent* entry;

			if(!(dir = opendir(path)))
			{
				return;
			}
			FILE *fp;
			char ch;
			int size = 0;
	
			unsigned long totalRead = 0;
			unsigned long totalWrite = 0;
			while((entry = readdir(dir)) != NULL)
			{
				if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && 
					strcmp(entry->d_name, "zippedFile.gz") != 0){
				int x = 0;
				char buffer[BUFF];
				char fileLen[BUFF];
				char filePath[BUFF];
				sprintf(filePath, "%s/%s", path, entry->d_name);
				gzwrite(outFile, filePath, strlen(filePath));

				if(entry->d_type == DT_DIR){
					//directory
					char path[1024];
					snprintf(path, sizeof(path), "%s/%s", projectName, entry->d_name);
					gzwrite(outFile, "D -1\n", strlen("D -1\n"));
					checkout(path, outFile, childfd);
				}else {
					//files
					gzwrite(outFile, " R ", strlen(" R "));
					fp = fopen(filePath, "rb");
					while((x = fread(fileLen, 1, sizeof(fileLen),fp)) > 0){
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	char numStr[BUFF];
	sprintf(numStr, sizeof(numStr), "%d", size);
	gzwrite(outFile, numStr, strlen(numStr));
	fclose(fp);
	
	x = 0;
	fp = fopen(filePath, "rb");
	gzwrite(outFile, "\n", strlen("\n"));
	while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0){
		totalRead += x;
	if(entry->d_type == DT_DIR){
		//directory
		char path[1024];
		sprintf(path, sizeof(path), "%s/%s", projectName, entry->d_name);
		gzwrite(outFile, "D -1\n", strlen(" D -1\n"));
		checkout(path, outFile, childfd);
	}else {
		//files
		gzwrite(outFile, " R ", strlen(" R "));
		fp = fopen(filePath, "rb");
		while((x = fread(fileLen, 1, sizeof(fileLen), fp)) > 0){
		}
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		char numStr[BUFF];
		sprintf(numStr, sizeof(numStr), "%d", size);
		gzwrite(outFile, numStr, strlen(numStr));
		fclose(fp);
		
		x = 0;
		fp = fopen(filePath, "rb");
		gzwrite(outFile, "\n", strlen("\n"));
		while((x = fread(buffer, 1, sizeof(buffer), fp)) > 0){
			totalRead += x,
			sprintf(buffer, "%s", buffer);
			gzwrite(outFile, buffer, x);
		}
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		fclose(fp);
		}
	}
}
closedir(dir);
}else{
	sendMsg("No path", childfd);
	exit(0);
}
//pthread_mutex_unlock(&mLock3);
}
}
}*/
/*char* project;
int sd;

if (strcmp(argv[1], "checkout") == 0)
	{
//void checkout(char* project, int sd) {
    //printf("project %s\n", project);
    if(exists("./server", project) != 1) {
        send(sd, "project not found", 18, 0);
        return;
    }
    send(sd, "Sending project...\n", 43, 0);
    //printf("project name %s\n", project);
    
    char* projectpath = (char*)malloc((9+strlen(project))*sizeof(char));
    projectpath[0] = '.';
    projectpath[1] = '/';
    projectpath[2] = 's';
    projectpath[3] = 'e';
    projectpath[4] = 'r';
    projectpath[5] = 'v';
    projectpath[6] = 'e';
    projectpath[7] = 'r';
    projectpath[8] = '/';
    projectpath = strcat(projectpath, project);
    
    //printf("project path %s\n", projectpath);
    traverseDir(projectpath);
    
    char buffer[256];
    
    while(fileNames != NULL) {
        bzero(buffer, 256);
        //printf("linked list %s\n", fileNames->name);
        read(sd, buffer, 256);
        if(strcmp(buffer, "next") == 0) {
            send(sd, fileNames->name, strlen(fileNames->name), 0);
            bzero(buffer, 256);
            read(sd, buffer, 256);
            int fd = open(fileNames->name, O_RDONLY);
            char* stringContent = "empty\a";
            if(fsize(fileNames->name) >= 1) {
                stringContent = readFile(fd, fileNames->name);
                //printf("content %s\n", stringContent);
            }
            send(sd, stringContent, strlen(stringContent), 0);
        }
        fileNames = fileNames->next;
    }
    
    read(sd, buffer, 256);
    send(sd, "done", 5, 0);
    bzero(buffer, 256);
    close(sd);
}*/
    		
     	// getting Manifest from server
     	if (strcmp(argv[1], "create") == 0 || strcmp(argv[1], "update") == 0 || strcmp(argv[1], "commit") == 0 || strcmp(argv[1], "upgrade") == 0) {
     		char manisize[30];
		int maniS;
		int i;
		for (i=4; i<strlen(buffer); i++) {
			if (buffer[i] == ':') {
				break;
			}
			manisize[i-4] = buffer[i];
		}
		manisize[i-4] = '\0';
		//printf("%s\n", namelength);
		maniS = atoi(manisize);
		
		char * manispath;
		if (strcmp(argv[1], "create") == 0) {
			manispath = (char *) malloc((strlen(argv[2])+10+1)*sizeof(char));
			snprintf(manispath, strlen(argv[2])+10+1, "%s/.Manifest", argv[2]);
			printf("%s\n", manispath);
		} else if (strcmp(argv[1], "update") == 0 || strcmp(argv[1], "upgrade") == 0 || strcmp(argv[1], "commit") == 0) {
			manispath = (char *) malloc((strlen(argv[2])+11+1)*sizeof(char));
			snprintf(manispath, strlen(argv[2])+11+1, "%s/.ManifestS", argv[2]);
			printf("%s\n", manispath);
		}
		
		int mansfd = open(manispath, O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
		
		int j;
		i++;
		for (j=0; j<maniS; j++) {
			write(mansfd, &buffer[i], 1);
			i++;
		}
		close(mansfd);
		
		if (strcmp(argv[1], "update") == 0) {
	     		FILE * sfp = fopen(manispath, "r");
	     		char * manpath = (char *) malloc((strlen(argv[2])+10+1)*sizeof(char));
	     		snprintf(manpath, strlen(argv[2])+10+1, "%s/.Manifest", argv[2]);
	     		FILE * cfp = fopen(manpath, "r");
	     		char * cversionStr = (char *) malloc(30*sizeof(char));
	     		char * sversionStr = (char *) malloc(30*sizeof(char));
	     		int cversion, sversion;
	     		fscanf(cfp, "%s\n", cversionStr);
	     		fscanf(sfp, "%s\n", sversionStr);
	     		cversion = atoi(cversionStr);
	     		sversion = atoi(sversionStr);
	     		printf("Manifest versions: c%d s%d\n", cversion, sversion); 
	     		
	     		char * cfversionStr = (char *) malloc(30*sizeof(char));
	     		char * sfversionStr = (char *) malloc(30*sizeof(char));
	     		int cfversion, sfversion;
	     		char * cfcurrfile = (char *) malloc(50*sizeof(char));
	     		char * sfcurrfile = (char *) malloc(50*sizeof(char));
	     		//char * cfhash = (char *) malloc(SHA256_DIGEST_LENGTH*sizeof(char));
	     		unsigned char * PreCFlivehash = (unsigned char *) malloc(SHA256_DIGEST_LENGTH*2*sizeof(unsigned char));
	     		char * cflivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
	     		char * sfhash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
	     		
	     		char * updpath = (char *) malloc((strlen(argv[2])+8+1)*sizeof(char));
	     		snprintf(updpath, strlen(argv[2])+8+1, "%s/.Update", argv[2]);
	     		int ufd = open(updpath, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
	     		
	     		maniEntry * manicmp = NULL;
	     		
	     		while (fscanf(sfp, "%s %s %s\n", sfversionStr, sfcurrfile, sfhash) != EOF) {
	     			sfversion = atoi(sfversionStr);
	     			maniEntry * maniNew = (maniEntry *) malloc(sizeof(maniEntry));
	     			maniNew->fname = (char *) malloc((strlen(sfcurrfile)+1)*sizeof(char));
	     			snprintf(maniNew->fname, strlen(sfcurrfile)+1, "%s", sfcurrfile);
	     			maniNew->vserver = sfversion;
	     			maniNew->shash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
	     			snprintf(maniNew->shash, SHA256_DIGEST_LENGTH*2+1, "%s", sfhash);
	     			maniNew->next = NULL;
	     			
	     			maniNew->vclient = -1;
	     			maniNew->chash = NULL;
	     			maniNew->clivehash = NULL;
	     			
	     			maniEntry * ptr = manicmp;
	     			if (ptr == NULL) {
	     				manicmp = maniNew;
	     			} else {
	     				while (ptr->next != NULL) {
	     					ptr = ptr->next;
	     				}
	     				ptr->next = maniNew;
	     			}
	     		}
	     		
	     		while (fscanf(cfp, "%s %s %*s\n", cfversionStr, cfcurrfile) != EOF) {
	     			cfversion = atoi(cfversionStr);
	     			maniEntry * ptr = manicmp;
	     			
	     			if (ptr == NULL) {
	     				maniEntry * maniNew = (maniEntry *) malloc(sizeof(maniEntry));
		     			maniNew->fname = (char *) malloc((strlen(cfcurrfile)+1)*sizeof(char));
		     			snprintf(maniNew->fname, strlen(cfcurrfile)+1, "%s", cfcurrfile);
		     			maniNew->vclient = cfversion;
		     			//maniNew->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
		     			//snprintf(maniNew->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
		     			
		     			PreCFlivehash = SHA256_hash_file(cfcurrfile);
					for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
						sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
					}
					cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
					maniNew->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
					snprintf(maniNew->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
					
					maniNew->vserver = -1;
					maniNew->shash = NULL;
		     			maniNew->next = NULL;
		     			
		     			manicmp = maniNew;	
	     			} else {
	     				int match = 0;
	     				while (ptr->next != NULL) {
	     					if (strcmp(ptr->fname, cfcurrfile) == 0) {
	     						ptr->vclient = cfversion;
	     						//ptr->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
		     					//snprintf(ptr->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
		     					PreCFlivehash = SHA256_hash_file(cfcurrfile);
							for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
								sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
							}
							cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
							ptr->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
							snprintf(ptr->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
							match = 1;
		     					break;
		     				}
	     					ptr = ptr->next;
	     				}
	     				if (ptr->next == NULL && match == 0) {
	     					if (strcmp(ptr->fname, cfcurrfile) == 0) {
	     						ptr->vclient = cfversion;
		     					//ptr->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
			     				//snprintf(ptr->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
			     				PreCFlivehash = SHA256_hash_file(cfcurrfile);
							for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
								sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
							}
							cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
							ptr->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
							snprintf(ptr->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
			     				break;
	     					} else {
	     						maniEntry * maniNew = (maniEntry *) malloc(sizeof(maniEntry));
					     		maniNew->fname = (char *) malloc((strlen(cfcurrfile)+1)*sizeof(char));
					     		snprintf(maniNew->fname, strlen(cfcurrfile)+1, "%s", cfcurrfile);
					     		maniNew->vclient = cfversion;
					     		//maniNew->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
					     		//snprintf(maniNew->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
					     		
					     		PreCFlivehash = SHA256_hash_file(cfcurrfile);
							for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
								sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
							}
							cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
							maniNew->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
							snprintf(ptr->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
							maniNew->vserver = -1;
							maniNew->shash = NULL;
				     			maniNew->next = NULL;
					     			
					     		ptr->next = maniNew;
					     	}
	     				}
	     			}
	     		}
	     		
	     		maniEntry * ptr = manicmp;
	     		while (ptr != NULL) {
	     			if (cversion == sversion) {
	     				if (ptr->vserver == -1) {
	     					printf("U %s\n", ptr->fname);
	     				} else {
	     					if (ptr->vclient == ptr->vserver) {
	     						if (strcmp(ptr->clivehash, ptr->shash) != 0) {
	     							printf("U %s\n", ptr->fname);
	     						}
	     					}
	     				}
	     			} else {
	     				if (ptr->vserver == -1) {
	     					printf("D %s\n", ptr->fname);
	     					write(ufd, "D ", 2);
	     					write(ufd, ptr->fname, strlen(ptr->fname));
	     					write(ufd, "\n", 1);
	     				} else if (ptr->vclient == -1) {
	     					printf("A %s\n", ptr->fname);
	     					write(ufd, "A ", 2);
	     					write(ufd, ptr->fname, strlen(ptr->fname));
	     					write(ufd, "\n", 1);
	     				} else {
	     					if (ptr->vclient == ptr->vserver) {
	     						if (strcmp(ptr->clivehash, ptr->shash) != 0) {
	     							printf("U %s\n", ptr->fname);
	     						}
	     					} else {
	     						if (strcmp(ptr->clivehash, ptr->shash) == 0) {
	     							printf("M %s\n", ptr->fname);
	     							write(ufd, "M ", 2);
	     							write(ufd, ptr->fname, strlen(ptr->fname));
	     							write(ufd, "\n", 1);
	     						} else {
	     							printf("Conflict error %s: must resolve before upgrade\n", ptr->fname);
	     						}
	     					}
	     				}
	     			}
	     			ptr = ptr->next;
	     		}
	     		remove(manispath);
	     		fclose(cfp);
	     		fclose(sfp);
	     		close(ufd);
     		} else if (strcmp(argv[1], "upgrade") == 0) {
     			char * upgpath = (char *) malloc((strlen(argv[2])+9+1)*sizeof(char));
	     		snprintf(upgpath, strlen(argv[2])+9+1, "%s/.Upgrade", argv[2]);
	     		FILE * upgfp = fopen(upgpath, "r");
     			if (upgfp == NULL) {
     				printf("Error: no upgrade file found");
     			} else {
     				char * code = (char *) malloc(2*sizeof(char));
     				char * fname = (char *) malloc(50*sizeof(char));
     				while (fscanf(upgfp, "%s %s\n", code, fname) != EOF) {
					if (strcmp(code, "D") == 0) {
						remove(fname);
					} else if (strcmp(code, "M") == 0) {
						
					} else {
						
					}
				}
				free(code);
				free(fname);
     			}
     			fclose(upgfp);
     			remove(upgpath);
     			free(upgpath);
     			remove(manispath);
     		} else if (strcmp(argv[1], "commit") == 0) {
     			char * upgpath = (char *) malloc((strlen(argv[2])+9+1)*sizeof(char));
	     		snprintf(upgpath, strlen(argv[2])+9+1, "%s/.Upgrade", argv[2]);
	     		FILE * fp = fopen(upgpath, "r");
	     		int size;
	     		if (fp != NULL) {
	     			fseek(fp, 0, SEEK_END);
	     			size = ftell(fp);
	     		} else {
	     			size = -1;
	     		}
	     		if (size > 0) {
	     			printf("Error: must resolve .Update before commit\n");
	     		} else {
	     			printf("%s\n", manispath);
		     		FILE * sfp = fopen(manispath, "r");
		     		if (sfp == NULL) {
		     			printf("wtf\n");
		     		}
			    	char * manpath = (char *) malloc((strlen(argv[2])+10+1)*sizeof(char));
			     	snprintf(manpath, strlen(argv[2])+10+1, "%s/.Manifest", argv[2]);
			     	FILE * cfp = fopen(manpath, "r");
			     	char * cversionStr = (char *) malloc(30*sizeof(char));
			     	char * sversionStr = (char *) malloc(30*sizeof(char));
			     	int cversion, sversion;
			     	fscanf(cfp, "%s\n", cversionStr);
			     	fscanf(sfp, "%s\n", sversionStr);
			     	cversion = atoi(cversionStr);
			     	sversion = atoi(sversionStr);
			     	
			     	printf("%s\n", sversionStr);
			     	printf("%d %d\n", cversion, sversion);
			     	
			     	if (cversion != sversion) {
			     		printf("Error: client version of project does not match server version\n");
			     	} else {
			     		char * compath = (char *) malloc((strlen(argv[2])+8+1)*sizeof(char));
				     	snprintf(compath, strlen(argv[2])+8+1, "%s/.Commit", argv[2]);
				     	int cfd = open(compath, O_RDWR | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR);
			     		
			     		char * cfversionStr = (char *) malloc(30*sizeof(char));
				     	char * sfversionStr = (char *) malloc(30*sizeof(char));
				     	int cfversion, sfversion;
				     	char * cfcurrfile = (char *) malloc(50*sizeof(char));
				     	char * sfcurrfile = (char *) malloc(50*sizeof(char));
				     	char * cfhash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
				     	unsigned char * PreCFlivehash = (unsigned char *) malloc(SHA256_DIGEST_LENGTH*2*sizeof(unsigned char));
				     	char * cflivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
				     	char * sfhash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
			     		
			     		maniEntry * manicmp = NULL;
	     		
			     		while (fscanf(sfp, "%s %s %s\n", sfversionStr, sfcurrfile, sfhash) != EOF) {
			     			sfversion = atoi(sfversionStr);
			     			maniEntry * maniNew = (maniEntry *) malloc(sizeof(maniEntry));
			     			maniNew->fname = (char *) malloc((strlen(sfcurrfile)+1)*sizeof(char));
			     			snprintf(maniNew->fname, strlen(sfcurrfile)+1, "%s", sfcurrfile);
			     			maniNew->vserver = sfversion;
			     			maniNew->shash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
			     			snprintf(maniNew->shash, SHA256_DIGEST_LENGTH*2+1, "%s", sfhash);
			     			maniNew->next = NULL;
			     			
			     			maniNew->vclient = -1;
			     			maniNew->chash = NULL;
			     			maniNew->clivehash = NULL;
			     			
			     			maniEntry * ptr = manicmp;
			     			if (ptr == NULL) {
			     				manicmp = maniNew;
			     			} else {
			     				while (ptr->next != NULL) {
			     					ptr = ptr->next;
			     				}
			     				ptr->next = maniNew;
			     			}
			     		}
			     		
			     		while (fscanf(cfp, "%s %s %s\n", cfversionStr, cfcurrfile, cfhash) != EOF) {
			     			cfversion = atoi(cfversionStr);
			     			maniEntry * ptr = manicmp;
			     			
			     			if (ptr == NULL) {
			     				maniEntry * maniNew = (maniEntry *) malloc(sizeof(maniEntry));
				     			maniNew->fname = (char *) malloc((strlen(cfcurrfile)+1)*sizeof(char));
				     			snprintf(maniNew->fname, strlen(cfcurrfile)+1, "%s", cfcurrfile);
				     			
				     			maniNew->vclient = cfversion;
				     			maniNew->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
				     			snprintf(maniNew->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
				     			
				     			PreCFlivehash = SHA256_hash_file(cfcurrfile);
							for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
								sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
							}
							cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
							maniNew->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
							snprintf(maniNew->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
							
							maniNew->vserver = -1;
							maniNew->shash = NULL;
				     			maniNew->next = NULL;
				     			
				     			manicmp = maniNew;	
			     			} else {
			     				int match = 0;
			     				while (ptr->next != NULL) {
			     					if (strcmp(ptr->fname, cfcurrfile) == 0) {
			     						ptr->vclient = cfversion;
			     						ptr->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
				     					snprintf(ptr->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
				     					PreCFlivehash = SHA256_hash_file(cfcurrfile);
									for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
										sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
									}
									cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
									ptr->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
									snprintf(ptr->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
									match = 1;
				     					break;
				     				}
			     					ptr = ptr->next;
			     				}
			     				if (ptr->next == NULL && match == 0) {
			     					if (strcmp(ptr->fname, cfcurrfile) == 0) {
			     						ptr->vclient = cfversion;
				     					ptr->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
					     				snprintf(ptr->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
					     				PreCFlivehash = SHA256_hash_file(cfcurrfile);
									for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
										sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
									}
									cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
									ptr->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
									snprintf(ptr->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
					     				break;
			     					} else {
			     						maniEntry * maniNew = (maniEntry *) malloc(sizeof(maniEntry));
							     		maniNew->fname = (char *) malloc((strlen(cfcurrfile)+1)*sizeof(char));
							     		snprintf(maniNew->fname, strlen(cfcurrfile)+1, "%s", cfcurrfile);
							     		maniNew->vclient = cfversion;
							     		maniNew->chash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
							     		snprintf(maniNew->chash, SHA256_DIGEST_LENGTH*2+1, "%s", cfhash);
							     		
							     		PreCFlivehash = SHA256_hash_file(cfcurrfile);
									for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
										sprintf((char *)&(cflivehash[i*2]), "%02x", PreCFlivehash[i]);
									}
									cflivehash[SHA256_DIGEST_LENGTH*2+1] = '\0';
									maniNew->clivehash = (char *) malloc((SHA256_DIGEST_LENGTH*2+1)*sizeof(char));
									snprintf(maniNew->clivehash, SHA256_DIGEST_LENGTH*2+1, "%s", cflivehash);
									maniNew->vserver = -1;
									maniNew->shash = NULL;
						     			maniNew->next = NULL;
							     			
							     		ptr->next = maniNew;
							     	}
			     				}
			     			}
			     		}
			     		maniEntry * ptr = manicmp;
			     		/*while (ptr != NULL) {
			     			printf("%s %d %d\n", ptr->fname, ptr->vclient, ptr->vserver);
			     			printf("%s %s %s\n", ptr->clivehash, ptr->chash, ptr->shash);
			     			ptr = ptr->next;
			     		}
			     		
			     		ptr = manicmp;*/
			     		
	     				while (ptr != NULL) {
	     					if (ptr->vclient == -1) {
	     						write(cfd, "D ", 2);
	     						write(cfd, ptr->fname, strlen(ptr->fname));
	     						write(cfd, "\n", 1);
	     					} else if (ptr->vserver == -1) {
	     						write(cfd, "A ", 2);
	     						write(cfd, ptr->fname, strlen(ptr->fname));
	     						write(cfd, "\n", 1);
	     					} else {
	     						if (ptr->vclient == ptr->vserver) {
	     							if (strcmp(ptr->clivehash, ptr->chash) != 0) {
	     								write(cfd, "O ", 2);
	     								write(cfd, ptr->fname, strlen(ptr->fname));
	     								write(cfd, "\n", 1);
	     							}
	     						} else {
	     							if (strcmp(ptr->chash, ptr->shash) != 0) {
	     								printf("Error: must synch with repo before commit\n");
	     								close(cfd);
	     								remove(compath);
	     								break;
	     						
	     							}
	     						}
	     					}
	     					ptr = ptr->next;
	     				}
				     	close(cfd);
			    	}
			     	fclose(sfp);
			     	fclose(cfp);
	     		}
	     		remove(manispath);
	     	}
     	}
     	
     	if (n < 0) {
     		printf("Receive failed\n");
     	}
     	
     	printf("%s\n", buffer);
     	
     	if (strcmp(argv[1], "commit") == 0) {
     		char * compath = (char *) malloc((strlen(argv[2])+8+1)*sizeof(char));
		snprintf(compath, strlen(argv[2])+8+1, "%s/.Commit", argv[2]);
	     	int cfd = open(compath, O_RDONLY, S_IWUSR | S_IRUSR);
     		
     		int size = lseek(cfd, 0, SEEK_END);
		lseek(cfd, 0, SEEK_SET);
		char * commitsend = (char *) malloc(1024*sizeof(char));
		int i;
		for (i=0; i<size; i++) {
			read(cfd, &commitsend[i], 1);
		}
		commitsend[i] = '\0';
     		
     		n = write(clientSock, commitsend, strlen(commitsend));
     		if (n < 0) {	
     			printf("Receive failed\n");
     		}
     		
     		char * comreceive = (char *) malloc(1024*sizeof(char));
		n = read(clientSock, comreceive, 1023);
		comreceive[n] = '\0';
		printf("%s", comreceive);
		free(comreceive);
     	}	
     	
     	close(clientSock);
        
	return 0;
}
