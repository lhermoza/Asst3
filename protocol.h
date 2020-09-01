#ifndef PROTOCOL_H
#define PROTOCOL_H
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

typedef struct project {
	char * name;
	int currVersion;
	struct project * next;
} project;

typedef struct maniEntry{
	char *fname;
	int vclient;
	int vserver;
	char *chash;
	char *shash;
	char *clivehash;
	struct maniEntry * next;
} maniEntry;
/*enum protocol_type
{
	SUCC = 0,
	FAIL,
	CHECKOUT,
	UPDATE,
	UPGRADE,
	COMMIT,
	PUSH,
	CREATE,
	DESTROY,
	ADD,
	REMOVE,
	CURRENTVERSION,
	HISTORY,
	ROLLBACK
};
typedef enum protocol_type protocol_type;

struct data_t
{
	protocol_type type;
	char project_name[256];
	char filepath[256];
	//whatever we need
};
typedef struct data_t data_t;

typedef enum protocol_type protocol_type;*/

//void parse(const char* buf, data_t* data);

/*void* client_handler(void* arg)
{
	int socket_fd = *(int*)arg;
	char buffer_recv[1024], buffer_send[1024];
	int len_recv, len_send;

	len_recv = read(socket_fd, buffer_recv, 1024);

	data_t data;
	parse(buffer_recv, &data);
	if(data.type == CREATE)
	{
		char cmd[256];
		sprintf("mkdir %s", data.project_name);
		exec(cmd);
	}
	else if(data.type == UPGRADE)
	{
		//
		//
		//
	}
	else if(data.type == DESTROY)
	{
		char cmd[256];
		sprintf("rm -r %s", data_project_name);
		exec(cmd);
	}
	
	else if(data.type == CHECKOUT)
	{
		//
		//
		//
	}
	else if(data.type == UPDATE)
	{
		//
		//
		//
	}
	else if(data.type == COMMIT)
	{
		//
		//
		//
	}
	else if(data.type == PUSH)
	{
		//
		//
		//
	}
	else if(data.type == ADD)
	{
		//
		//
		//
	}
	else if(data.type == REMOVE)
	{
		//
		//
		//
	}
	else if(data.type == CURRENTVERSION)
	{
		//
		//
		//
	}
	else if(data.type == HISTORY)
	{
		//
		//
		//
	}
	else if(data.type == ROLLBACK)
	{
		//
		//
		//
	}
}*/
#endif
