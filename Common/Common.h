
#pragma once

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

#define BUFFER_SIZE	128 * 4
#define PACKET_SIZE	128
#define SERVER_PORT	12345
#define FALSE		0
#define TRUE		1

void error(const char *msg);
