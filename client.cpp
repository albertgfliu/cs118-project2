#include <cstring>
#include <string>
#include <thread>
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include "TCPHeader.h"
#include <stdio.h>      /* printf */
#include <string.h>     /* strcat */
#include <stdlib.h>     /* strtol */

#define BUFSIZE 			1032
#define MAXSEQNUM 			30720
#define INITCONGWINSIZE 	1024
#define INITSSTHRESH		30720
#define RECEIVEWINSIZE		30720

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

int
main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cerr << "Usage: " + std::string(argv[0]) + " SERVER-HOST-OR-IP PORT-NUMBER" << std::endl;
		exit(EXIT_FAILURE);
	}

	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		std::cerr << "Error: could not bind to socket" << std::endl;
		exit(EXIT_FAILURE);
	}


	struct hostent *hp;
	hp = gethostbyname(argv[1]);
	if (!hp) {
		std::cout << "Error: could not obtain address of host " + std::string(argv[1]) << std::endl;
		exit(EXIT_FAILURE);
	}


	struct sockaddr_in serverAddress;
	socklen_t addressLength = sizeof(serverAddress);
	memset((char *)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(std::atoi(argv[2]));
	memcpy((void *)&serverAddress.sin_addr, hp->h_addr_list[0], hp->h_length);

	char buf[BUFSIZE];
	struct TCPHeader tcphdr;
	uint16_t initSeqNum = rand() % MAXSEQNUM;
	setSeqNum((struct TCPHeader *)&tcphdr, initSeqNum);
	setAckNum((struct TCPHeader *)&tcphdr, 0);
	setWindow((struct TCPHeader *)&tcphdr, RECEIVEWINSIZE);
	setNU_ASF((struct TCPHeader *)&tcphdr, false, true, false);

	memcpy(buf, (struct TCPHeader *)&tcphdr, sizeof(tcphdr));

	printf("Sending ACK packet %d\n", getAckNum((struct TCPHeader *)&tcphdr));
	sendto(sockfd, buf, sizeof(tcphdr), 0, (struct sockaddr *)&serverAddress, addressLength);

	struct TCPHeader tcphdr_synack;
	int bytesReceived = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serverAddress, &addressLength);
	memcpy((struct TCPHeader *)&tcphdr_synack, buf, bytesReceived);

	printf("Receiving data packet %d\n", getSeqNum((struct TCPHeader *)&tcphdr_synack));

	//The client should first start by making a handshake.

	close(sockfd);
}