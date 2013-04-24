
#include "../Common/Common.h"
#include "../Common/Utilities/StringUtil.h"
#include "../Thread/Thread.h"
#include "Tracker.h"
#include <signal.h>

int 		sockfd;
struct 		sockaddr_in serv_addr;
int 		numOfFiles = 0;
int 		maxNumOfFiles = 10;
int 		SERVER_PORT;

Socket*		sockets;
int 		socketsSize = 0;
int 		maxSocketsSize = 10;

SharedFile*	files;

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

int getSocket(int sockfd) {
	int i = 0;
	for (; i < socketsSize; ++i)
		if (sockets[i].sockfd == sockfd)
			return i;
	return -1;
}

void termHandle(int sig) {
	close(sockfd);
	printf("Exiting, Got intrupt signal\n");
	exit(0);
}

int getFileIndex(char* fileName) {
	int i = 0;
	for (; i < numOfFiles; ++i)
		if (strcmp(files[i].fileName, fileName) == 0)
			return i;
	return -1;
}

void sendFilesName(int newsockfd, char* buffer) {
	int i = 0;
	resetBuf(buffer);
	for (; i < numOfFiles; ++i) {
		strcat(buffer, files[i].fileName);
		strcat(buffer, "\n");
	}
	report(send(newsockfd, buffer, strlen(buffer), 0), 1);
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

void sendFileSeederWithPart(int newsockfd, char* buffer) {
	int 	index = indexOf(buffer, ' ');
	char 	fileName[BUFFER_SIZE];
	char	tmpbuf[BUFFER_SIZE];
	long long 	part;

	resetBuf(tmpbuf);
	resetBuf(fileName);
	strncpy(fileName, buffer + 2, index - 2);
	strcpy(tmpbuf, buffer + index + 1);
	part = atol(tmpbuf);

	index = getFileIndex(fileName);
	if (index < 0) {
		report(send(newsockfd, "-1", 2, 0), 1);
		return;
	}

	int i = 0, flag = 0;
	for (; i < files[index].seedersSize; ++i) {
		if (((files[index].seeders[i].parts[part / 8]) >> (part % 8)) % 2) {
			flag = 1;

			report(send(files[index].seeders[i].sockfd, buffer, strlen(buffer), 0), 1);

			int sockIndex = getSocket(files[index].seeders[i].sockfd);
			while (strncmp(sockets[sockIndex].socketBuffer, "T", 1) != 0)
				usleep(10000);

			strcpy(buffer, sockets[sockIndex].socketBuffer);
			strcpy(sockets[sockIndex].socketBuffer, "");

			char	tmpbuf[BUFFER_SIZE];
			resetBuf(tmpbuf);

			strcpy(tmpbuf, files[index].seeders[i].ip);
			strcat(tmpbuf, " ");
			strcat(tmpbuf, buffer + 3);
			strcat(tmpbuf, " ");
			resetBuf(buffer);
			convertIntToString(sockIndex, buffer);
			strcat(tmpbuf, buffer);

			report(send(newsockfd, tmpbuf, strlen(tmpbuf), 0), 1);

			removePart(&(files[index].seeders[i]), part);
			break;
		}
	}
	if (flag != 1) {
		printf("Couldnt find any seeders for file %s part %Ld\n", fileName, part);
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
	resetBuf(fileName);
	strncpy(fileName, buffer + 2, fs - 2);

	if (getFileIndex(fileName) != -1) {
		report(send(newsockfd, "Invalid", 7, 0), 1);
		return;
	}

	printf("%s:%d\tfs:%d\n", fileName, size, fs);

	initFile(&(files[numOfFiles]), fileName, size);

	inet_ntop(AF_INET, &cli_addr.sin_addr, buffer, BUFFER_SIZE);

	addSeeder(buffer, getSocket(newsockfd), &(files[numOfFiles++]), newsockfd);

	report(send(newsockfd, "OK", 2, 0), 1);
}

void sendInfo(int newsockfd, struct sockaddr_in cli_addr, char* buffer) {
	strcpy(buffer, buffer + 2);
	char tmpbuf[BUFFER_SIZE];
	resetBuf(tmpbuf);
	int index = indexOf(buffer, ' ');
	int id = atoi(buffer + index + 1);
	strncpy(tmpbuf, buffer, index);
	int j = getFileIndex(tmpbuf), flag = 0;
	if (j == -1)
		report(send(newsockfd, "-1", 2, 0), 1);
	resetBuf(buffer);
	inet_ntop(AF_INET, &cli_addr.sin_addr, buffer, BUFFER_SIZE);
	addPeer(buffer, id, &(files[j]), newsockfd);
	resetBuf(buffer);
	itoal(files[j].size, buffer);

	report(send(newsockfd, buffer, strlen(buffer), 0), 1);
}

void jobComplete(int newsockfd, struct sockaddr_in cli_addr, char* buffer) {
	char fileName[BUFFER_SIZE];
	char tmpbuf[BUFFER_SIZE];
	int idA, idB;
	long long part;

	strcpy(buffer, buffer + 2);
	int index = indexOf(buffer, ' ');
	strncpy(tmpbuf, buffer, index);
	idA = atoi(tmpbuf);
	strcpy(buffer, buffer + index + 1);
	index = indexOf(buffer, ' ');
	strncpy(tmpbuf, buffer, index);
	idB = atoi(tmpbuf);
	strcpy(buffer, buffer + index + 1);
	index = indexOf(buffer, ' ');
	strncpy(fileName, buffer, index);
	resetBuf(tmpbuf);
	strcpy(tmpbuf, buffer + index + 1);
	part = atol(tmpbuf);

	// printf("idA:%d idB:%d FILENAME:%s PART:%Ld\n", idA, idB, fileName, part);

	int j = getFileIndex(fileName), flag = 0;
	if (j < 0) {
		report(send(newsockfd, "-1", 2, 0), 1);
		return;
	}

	int i = 0;
	for (; i < files[j].seedersSize; ++i) {
		if (files[j].seeders[i].id == idA || files[j].seeders[i].id == idB) {
			flag = 1;
			addPart(&(files[j].seeders[i]), part);
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
	printf("Waiting for a connection! TID:%d\n", tid);
	int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client);

	printf("Connection has been stablished! TID:%d\n", tid);
	if (newsockfd < 0)
		error("ERROR on accept");

	int sockIndex = getSocket(newsockfd);
	if (sockIndex < 0) {
		if (maxSocketsSize == socketsSize) {
			maxSocketsSize += 10;
			sockets = (Socket*) realloc(sockets, sizeof(Socket) * maxSocketsSize);
		}
		sockets[socketsSize++].sockfd = newsockfd;
	}

	char tmpInt[BUFFER_SIZE];
	resetBuf(tmpInt);
	convertIntToString(getSocket(newsockfd), tmpInt);
	report(send(newsockfd, tmpInt, strlen(tmpInt), 0), 1);

	runThread(&Connection);
	resetBuf(buffer);

	while (TRUE) {
		resetBuf(buffer);
		printf("Waiting on RECV TID:%d\n", tid);
		report(recv(newsockfd, buffer, BUFFER_SIZE, 0), 0);
		printf("MSG:%s TID:%d\n", buffer, tid);

		if (strcmp(buffer, "EXIT") == 0)
			break;

		if (strncmp(buffer, "TOF", 3) == 0) {
			int sockIndex = getSocket(newsockfd);
			strcpy(sockets[sockIndex].socketBuffer, buffer);
			continue;
		}

		if (strncmp(buffer, "GT", 2) == 0) {
			sendFileSeederWithPart(newsockfd, buffer);
			printf("Sent file seeder with part!\n");
		} else if (strncmp(buffer, "SH", 2) == 0) {
			shareFile(newsockfd, cli_addr, buffer);
			printf("Sharing done!\n");
		} else if (strncmp(buffer, "GI", 2) == 0) {
			sendInfo(newsockfd, cli_addr, buffer);
			printf("Sent info!\n");
		} else if (strcmp(buffer, "GF") == 0) {
			sendFilesName(newsockfd, buffer);
			printf("Sent file names!\n");
		} else if (strncmp(buffer, "JD", 2) == 0) {
			jobComplete(newsockfd, cli_addr, buffer);
			printf("Job complete!\n");
		} else if (strcmp(buffer, "EXITALL") == 0) {
			termHandle(SIGINT);
		} else
			report(send(newsockfd, "Invalid", 7, 0), 1);
	}
	close(newsockfd);
	freeThread(tid);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, termHandle);
	files = (SharedFile *) malloc(sizeof(SharedFile) * maxNumOfFiles);
	sockets = (Socket*) malloc(sizeof(Socket) * maxSocketsSize);

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
	unsigned short tmp = 1 << (part % 8);
	seeder->parts[part / 8] |= tmp;
}

void removePart(Seeder* seeder, int part) {
	unsigned short tmp = ~(1 << (part % 8));
	seeder->parts[part / 8] &= tmp;
}

void addSeeder(char* ip, int id, SharedFile* sharedFile, int socketfd) {
	if (sharedFile->seedersSize == sharedFile->maxSize) {
		sharedFile->maxSize += 10;
		sharedFile->seeders = (Seeder*) realloc(sharedFile->seeders, sizeof(Seeder) * sharedFile->maxSize);
	}
	Seeder seeder;
	seeder.sockfd = socketfd;
	seeder.id = id;
	strcpy(seeder.ip, ip);

	seeder.parts = (short*) malloc(sizeof(short) * sharedFile->size / 8 + 1);
	int i = 0;

	for (; i < sharedFile->size / 8 + 1; ++i)
		seeder.parts[i] = 255;

	sharedFile->seeders[sharedFile->seedersSize ++] = seeder;
}

void addPeer(char* ip, int id, SharedFile* sharedFile, int socketfd) {
	int i = 0, flag = 0;
	for (; i < sharedFile->seedersSize; ++i)
		if (sharedFile->seeders[i].id == id)
			return;

	if (sharedFile->seedersSize == sharedFile->maxSize) {
		sharedFile->maxSize += 10;
		sharedFile->seeders = (Seeder*) realloc(sharedFile->seeders, sizeof(Seeder) * sharedFile->maxSize);
	}

	Seeder seeder;
	seeder.sockfd = socketfd;
	seeder.id = id;
	strcpy(seeder.ip, ip);

	seeder.parts = (short*) malloc(sizeof(short) * sharedFile->size / 8 + 1);

	for (i = 0; i < sharedFile->size / 8 + 1; ++i)
		seeder.parts[i] = 0;

	sharedFile->seeders[sharedFile->seedersSize ++] = seeder;
}

void initFile(SharedFile* sharedFile, char* fileName, long long size) {
	sharedFile->seeders = (Seeder*) malloc(sizeof(Seeder) * 10);
	sharedFile->maxSize = 10;
	sharedFile->seedersSize = 0;
	strcpy(sharedFile->fileName, fileName);
	sharedFile->size = size;
}
