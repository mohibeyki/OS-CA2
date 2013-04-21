
#pragma once

typedef struct Seeder {
	char 	ip[16];
	short*	parts;
} Seeder;

typedef struct SharedFile {
	Seeder* seeders;
	char* fileName;
	long size;
	int seedersSize;
	int maxSize;
} SharedFile;

void initFile(SharedFile* sharedFile, char* fileName, long size);
void addPart(Seeder* seeder, int part);
void removePart(Seeder* seeder, int part);
void addSeeder(char* ip, SharedFile* sharedFile);
