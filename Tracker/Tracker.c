
#include "../Common/Common.h"
#include "Tracker.h"

int main(int argc, char *argv[]) {
	int 		sockfd, newsockfd, portno;
	socklen_t 	client;
	char 		buffer[BUFFER_SIZE];
	struct 		sockaddr_in serv_addr, cli_addr;
	int 		n;
	int 		numOfFiles = 0;
	int 		maxNumOfFiles = 10;

	SharedFile*	files;

	files = (SharedFile *) malloc(sizeof(SharedFile) * maxNumOfFiles);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	listen(sockfd, 5);
	client = sizeof(cli_addr);
	newsockfd = accept(sockfd,
			(struct sockaddr *) &cli_addr,
			&client);
	if (newsockfd < 0)
		error("ERROR on accept");

	memset(buffer, 0, BUFFER_SIZE);

	while (strcmp(buffer, "EXIT") != 0) {
		memset(buffer, 0, BUFFER_SIZE);
		n = recv(newsockfd, buffer, BUFFER_SIZE, 0);
		if (n < 0)
			error("ERROR reading from socket");

		if (strncmp(buffer, "GT", 2) == 0) {

			int index = indexOf(buffer, ' ');
			char fileName[BUFFER_SIZE];
			strcpy(fileName, buffer + index + 1);
			int part = atoi(fileName);
			strncpy(fileName, buffer + 2, index - 2);

			int j = 0, flag = 0;
			for (; j < numOfFiles; ++j) {
				if (strcmp(fileName, files[j].fileName) == 0) {
					int i = 0;
					for (; i < files[j].seedersSize; ++i) {
						if (((files[j].seeders[i].parts[part / 8]) >> (part % 8)) % 2) {
							flag = 1;
							strcpy(buffer, files[j].seeders[i].ip);
							n = send(newsockfd, buffer, strlen(buffer), 0);
							if (n < 0)
								error("ERROR writing to socket");
							removePart(&(files[j].seeders[i]), part);
							break;
						}
					}
					break;
				}
			}
			if (flag == 0) {
				n = send(newsockfd, "-1", 2, 0);
				if (n < 0)
					error("ERROR writing to socket");
			}
		} else if (strncmp(buffer, "SH", 2) == 0) {
			if (numOfFiles == maxNumOfFiles) {
				maxNumOfFiles += 10;
				files = (SharedFile*) realloc(files, sizeof(SharedFile) * maxNumOfFiles);
			}

			int index = indexOf(buffer, ' ');
			char fileName[BUFFER_SIZE];
			strcpy(fileName, buffer + index + 1);
			long size = atoi(fileName);
			strncpy(fileName, buffer + 2, index - 2);

			initFile(&(files[numOfFiles]), fileName, size);
			numOfFiles++;

			inet_ntop(AF_INET, &cli_addr, buffer, BUFFER_SIZE);
			addSeeder(buffer, &(files[numOfFiles]));

			n = send(newsockfd, "OK", 2, 0);
			if (n < 0)
				error("ERROR writing to socket");

		} else if (strncmp(buffer, "GI", 2) == 0) {
			int j = 0, flag = 0;
			for (; j < numOfFiles; ++j) {
				if (strcmp(buffer + 2, files[j].fileName) == 0) {
					flag = 1;
					itoal(buffer, files[j].size);
					n = send(newsockfd, buffer, strlen(buffer), 0);
					if (n < 0)
						error("ERROR writing to socket");
					break;
				}
			}
			if (flag == 0) {
				n = send(newsockfd, "-1", 2, 0);
				if (n < 0)
					error("ERROR writing to socket");
			}
		} else if (strncmp(buffer, "JD", 2) == 0) {

			int index = indexOf(buffer, ' ');
			char fileName[BUFFER_SIZE], sip[BUFFER_SIZE];
			strncpy(sip, buffer + 2, index - 2);
			strcpy(buffer, buffer + index + 1);
			strcpy(fileName, buffer + index + 1);
			long part = atol(fileName);
			strncpy(fileName, buffer, index);
			inet_ntop(AF_INET, &cli_addr, buffer, BUFFER_SIZE);

			int j = 0, flag = 0;
			for (; j < numOfFiles; ++j) {
				if (strcmp(fileName, files[j].fileName) == 0) {
					int i = 0;
					for (; i < files[j].seedersSize; ++i) {
						if (strcmp(files[j].seeders[i].ip, sip) == 0 || strcmp(files[j].seeders[i].ip, buffer) == 0) {
							flag = 1;
							n = send(newsockfd, "OK", 2, 0);
							if (n < 0)
								error("ERROR writing to socket");
							addPart(&(files[j].seeders[i]), part);
						}
					}
					break;
				}
			}
			if (flag == 0) {
				n = send(newsockfd, "-1", 2, 0);
				if (n < 0)
					error("ERROR writing to socket");
			}

		} else {
			n = send(newsockfd, "Invalid", 7, 0);
			if (n < 0)
				error("ERROR writing to socket");
		}
	}
	close(newsockfd);
	close(sockfd);
	return 0;
}

void addPart(Seeder* seeder, int part) {
	seeder->parts[part / 8] |= 1 << (part % 8 - 1);
}

void removePart(Seeder* seeder, int part) {
	seeder->parts[part / 8] &= ~(1 << (part % 8 - 1));
}

void addSeeder(char* ip, SharedFile* sharedFile) {
	if (sharedFile->seedersSize == sharedFile->maxSize) {
		sharedFile->maxSize += 10;
		sharedFile->seeders = (Seeder*) realloc(sharedFile->seeders, sizeof(Seeder) * sharedFile->maxSize);
	}
	Seeder seeder;
	strcpy(seeder.ip, ip);

	seeder.parts = (short*) malloc(sizeof(short) * sharedFile->size / 8);
	int i = 0;

	for (; i < sharedFile->size / 8; ++i)
		seeder.parts[i] = 255;

	sharedFile->seeders[sharedFile->seedersSize ++] = seeder;
}

void initFile(SharedFile* sharedFile, char* fileName, long size) {
	sharedFile->seeders = (Seeder*) malloc(sizeof(Seeder) * 10);
	sharedFile->maxSize = 10;
	sharedFile->seedersSize = 0;
	sharedFile->fileName = fileName;
	sharedFile->size = size;
}
