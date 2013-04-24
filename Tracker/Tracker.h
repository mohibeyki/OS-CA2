
#pragma once

#include "../Common/Common.h"

typedef struct Seeder {
	unsigned short*	parts;
	int 	sockfd;
	int 	id;
	char	ip[16];
} Seeder;

typedef struct SharedFile {
	Seeder*	seeders;
	char 	fileName[BUFFER_SIZE];
	long 	size;
	int 	seedersSize;
	int 	maxSize;
} SharedFile;

typedef struct Socket {
	int 	sockfd;
	char	socketBuffer[BUFFER_SIZE];
} Socket;

void initFile(SharedFile* sharedFile, char* fileName, long long size);
void addPart(Seeder* seeder, int part);
void removePart(Seeder* seeder, int part);
void addSeeder(char* ip, int id, SharedFile* sharedFile, int sockfd);
void addPeer(char* ip, int id, SharedFile* sharedFile, int sockfd);
