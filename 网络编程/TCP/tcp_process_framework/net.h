#ifndef __NET_H_
#define __NET_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERVER_PORT 5001
#define SERVER_IP_ADDR "192.168.5.11"
#define BACKLOG 5
#define QUIT_STR "quit"

#endif