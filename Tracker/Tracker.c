
#include "../Common/Common.h"
#include "Tracker.h"

void error(const char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	int 		sockfd, newsockfd, portno;
	socklen_t 	client;
	char 		buffer[BUFFER_SIZE];
	struct 		sockaddr_in serv_addr, cli_addr;
	int 		n;

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
		printf("Here is the message: %s\n", buffer);
		char newBuff[BUFFER_SIZE] = "You sent : ";
		strcat(newBuff, buffer);
		n = send(newsockfd, newBuff, strlen(newBuff), 0);
		if (n < 0)
			error("ERROR writing to socket");
	}
	close(newsockfd);
	close(sockfd);
	return 0;
}

void addPart(Seeder* seeder, int part) {
	seeder->parts[part / 8] |= 1 << (part % 8);
}

void removePart(Seeder* seeder, int part) {
	seeder->parts[part / 8] &= ~(1 << (part % 8));
}

void addSeeder(char* ip, SharedFile* sharedFile) {
	if (seedersSize == maxSize) {
		maxSize += 10;
		seeders = (Seeder*) realloc(seeders, sizeof(Seeder) * maxSize);
	}
	Seeder seeder;
	strcpy(seeder.ip, ip);

	seeder.parts = (short*) malloc(sizeof(short) * sharedFile.size / 8);
	int i = 0;

	for (; i < sharedFile.size / 8; ++i)
		parts[i] = 255;

	sharedFile->seeders[seedersSize] =
	seedersSize ++;
}

void initFile(SharedFile* sharedFile, char* fileName, long size) {
	sharedFile->seeders = (Seeder*) malloc(sizeof(Seeder) * 10);
	sharedFile->maxSize = 10;
	sharedFile->seedersSize = 0;
	sharedFile->fileName = fileName;
	sharedFile->size = size;
}
