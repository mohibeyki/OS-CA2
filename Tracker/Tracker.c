
#include "../Common/Common.h"
#include "../Thread/Thread.h"
#include "Tracker.h"
#include <signal.h>

int 		sockfd;
struct 		sockaddr_in serv_addr;
int 		numOfFiles = 0;
int 		maxNumOfFiles = 10;
int 		SERVER_PORT;

SharedFile*	files;

void termHandle(int sig) {
	close(sockfd);
	printf("ENDED INT\n");
	exit(0);
}

int getFileIndex(char* fileName) {
	int i = 0;
	for (; i < numOfFiles; ++i)
		if (strcmp(files[i].fileName, fileName) == 0)
			return i;
	return -1;
}

void initializeSocket() {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
		exit(1);
	}
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
		exit(1);
	}
	listen(sockfd, 5);
}

void resetBuf(char* buffer) {
	memset(buffer, 0, BUFFER_SIZE);
}

void report(int n, int type) {
	if (n < 0) {
		if (type == 0)
			error("ERROR reading from socket");
		if (type == 1)
			error("ERROR writing to socket");
	}
}

void sendFileSeederWithPart(int newsockfd, char* buffer) {

	printf("AAA\n");

	int 	index = indexOf(buffer, ' ');
	char 	fileName[BUFFER_SIZE];
	char	tmpbuf[BUFFER_SIZE];
	long 	part;

	resetBuf(tmpbuf);
	resetBuf(fileName);
	strncpy(fileName, buffer + 2, index - 2);
	strcpy(tmpbuf, buffer + index + 1);
	part = atol(tmpbuf);

	printf("BBB file : %s part : %lo\n", fileName, part);

	index = getFileIndex(fileName);
	if (index < 0)
		report(send(newsockfd, "-1", 2, 0), 1);

	printf("CCC\n");

	int i = 0, flag = 0;
	for (; i < files[index].seedersSize; ++i) {
		if (((files[index].seeders[i].parts[part / 8]) >> (part % 8)) % 2) {
			flag = 1;
			printf("I'm gonna get PORT: %s\n", buffer);

			report(send(files[index].seeders[i].socketfd, buffer, strlen(buffer), 0), 1);
			printf("I sent message: %s\n", buffer);
			report(recv(files[index].seeders[i].socketfd, buffer, BUFFER_SIZE, 0), 0);

			printf("I get the message PORT: %s\n", buffer);
			printf("RECV:%s\n", buffer);

			char	tmpbuf[BUFFER_SIZE];
			resetBuf(tmpbuf);

			strcpy(tmpbuf, files[index].seeders[i].ip);
			strcat(tmpbuf, " ");
			strcat(tmpbuf, buffer + 3);

			resetBuf(buffer);

			report(send(newsockfd, tmpbuf, strlen(tmpbuf),0),1);

			removePart(&(files[index].seeders[i]), part);
			break;
		}
	}
	if (flag != 1) {
		printf("Couldnt find any seeders for file %s part %lo\n", fileName, part);
		report(send(newsockfd, "-1 -1", 5, 0), 0);
	}
}

void shareFile(int newsockfd, struct sockaddr_in cli_addr, char* buffer) {
	if (numOfFiles == maxNumOfFiles) {
		maxNumOfFiles += 10;
		files = (SharedFile*) realloc(files, sizeof(SharedFile) * maxNumOfFiles);
	}

	char	fileName[BUFFER_SIZE];
	int 	fs = indexOf(buffer, ' ');

	if (fs < 0) {
		report(send(newsockfd, "Invalid", 7, 0), 1);
		return;
	}

	strcpy(fileName, buffer + fs + 1);
	int 	size = atoi(fileName);
	strncpy(fileName, buffer + 2, fs - 2);

	if (getFileIndex(fileName) != -1) {
		report(send(newsockfd, "Invalid", 7, 0), 1);
		return;
	}

	printf("%s:%d\n", fileName, size);

	initFile(&(files[numOfFiles]), fileName, size);

	inet_ntop(AF_INET, &cli_addr.sin_addr, buffer, BUFFER_SIZE);

	addSeeder(buffer, &(files[numOfFiles++]), newsockfd);

	report(send(newsockfd, "OK", 2, 0), 1);
}

void sendInfo(int newsockfd, char* buffer) {
	int j = getFileIndex(buffer + 2), flag = 0;
	if (j == -1)
		report(send(newsockfd, "-1", 2, 0), 1);
	resetBuf(buffer);
	itoal(files[j].size, buffer);
	report(send(newsockfd, buffer, strlen(buffer), 0), 1);
}

void jobComplete(int newsockfd, struct sockaddr_in cli_addr, char* buffer) {

	char ip[BUFFER_SIZE];
	char fileName[BUFFER_SIZE];

	printf("%s\n", buffer);

	int index = indexOf(buffer, ' ');
	strncpy(ip, buffer + 2, index - 2);
	strcpy(buffer, buffer + index + 1);
	index = indexOf(buffer, ' ');
	memset(fileName, 0, BUFFER_SIZE);
	strncpy(fileName, buffer, index);
	printf("%s\n", fileName);
	long part = atol(buffer + index + 1);
	inet_ntop(AF_INET, &cli_addr.sin_addr, buffer, BUFFER_SIZE);

	int j = 0, flag = 0;
	for (; j < numOfFiles; ++j) {
		if (strcmp(buffer, files[j].fileName) == 0) {
			int i = 0;
			for (; i < files[j].seedersSize; ++i) {
				if (strcmp(files[j].seeders[i].ip, buffer) == 0 || strcmp(files[j].seeders[i].ip, ip) == 0) {
					flag = 1;
					addPart(&(files[j].seeders[i]), part);
				}
			}
			break;
		}
	}
	if (flag == 0)
		report(send(newsockfd, "-1", 2, 0), 1);
	else
		report(send(newsockfd, "OK", 2, 0), 1);
}

void Connection(int tid) {
	struct sockaddr_in 	cli_addr;
	socklen_t 			client;
	char 				buffer[BUFFER_SIZE];

	client = sizeof(cli_addr);
	printf("Waiting for Connection!\n");
	int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client);
	printf("Connection has been stablished!\n");
	if (newsockfd < 0)
		error("ERROR on accept");

	runThread(&Connection);
	resetBuf(buffer);

	while (strcmp(buffer, "EXIT") != 0) {
		resetBuf(buffer);
		report(recv(newsockfd, buffer, BUFFER_SIZE, 0), 0);
		if (strncmp(buffer, "TOF", 3) == 0) {
			printf("TOF MSG:%s\n", buffer);
			// usleep(10000);
			continue;
		}
		printf("tid: %d MSG:%s\n", tid, buffer);

		if (strncmp(buffer, "GT", 2) == 0) {
			sendFileSeederWithPart(newsockfd, buffer);
			printf("Sent file seeder with part!\n");
		} else if (strncmp(buffer, "SH", 2) == 0) {
			shareFile(newsockfd, cli_addr, buffer);
			printf("Sharing done!\n");
		} else if (strncmp(buffer, "GI", 2) == 0) {
			sendInfo(newsockfd, buffer);
			printf("Sent info!\n");
		} else if (strncmp(buffer, "JD", 2) == 0) {
			jobComplete(newsockfd, cli_addr, buffer);
			printf("Job complete\n");
		} else if (strcmp(buffer, "EXITALL") == 0) {
			close(sockfd);
			exit(0);
		} else
			report(send(newsockfd, "Invalid", 7, 0), 1);
		usleep(100000);
	}
	close(newsockfd);
	freeThread(tid);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, termHandle);
	files = (SharedFile *) malloc(sizeof(SharedFile) * maxNumOfFiles);

	if (argc != 2) {
		printf("Usage ./Tracker PORT\n");

		exit(1);
	}
	SERVER_PORT = atoi(argv[1]);

	initializeThreadManager();
	initializeSocket();
	runThread(&Connection);
	joinThreads();
	close(sockfd);
	printf("Exiting\n");
	return 0;
}

void addPart(Seeder* seeder, int part) {
	seeder->parts[part / 8] |= 1 << (part % 8 - 1);
}

void removePart(Seeder* seeder, int part) {
	seeder->parts[part / 8] &= ~(1 << (part % 8 - 1));
}

void addSeeder(char* ip, SharedFile* sharedFile, int socketfd) {
	if (sharedFile->seedersSize == sharedFile->maxSize) {
		sharedFile->maxSize += 10;
		sharedFile->seeders = (Seeder*) realloc(sharedFile->seeders, sizeof(Seeder) * sharedFile->maxSize);
	}
	Seeder seeder;
	seeder.socketfd = socketfd;
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
