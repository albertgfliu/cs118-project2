#include <cstring>
#include <string>
#include <thread>
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "TCPHeader.h"
#include <iostream>
#include <fstream>


#include <stdio.h>      /* printf */
#include <string.h>     /* strcat */
#include <stdlib.h>     /* strtol */

#define BUFSIZE				1032
#define MAXSEQNUM 			30720
#define INITCONGWINSIZE 	1024
#define INITSSTHRESH		30720
#define RECEIVEWINSIZE		30720

void
printACK(unsigned int ackNum)
{
	fprintf(stdout, "Receiving ACK packet %d \n", ackNum);
}

void
printDATA(unsigned int seqNum, unsigned int CWND, unsigned int SSThresh, bool retransmission)
{
	fprintf(stdout, "Sending ACK packet %d ", ackNum);
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stdout, "\n");
}

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
	if(argc != 3) {
		std::cerr << "Usage: " + std::string(argv[0]) + " PORT-NUMBER FILE-NAME" << std::endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	socklen_t addressLength = sizeof(clientAddress);

	int bytesReceived;
	int sockfd;

	char buf[BUFSIZE];

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		std::cerr << "Error: could not create UDP socket; exiting. " << std::endl;
		exit(EXIT_FAILURE);
	}

	int optval;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
		std::cerr << "Error: could not set socket option for reuse; exiting. " << std::endl;
		exit(EXIT_FAILURE);
	}

	/* Bind to any availabile IP address and a specific port specified by PORT-NUMBER*/
	memset((char *)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(std::stoi(argv[1]));

	if (bind(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		std::cerr << "Error: binding failed; exiting. " << std::endl;
		exit(EXIT_FAILURE);
	}

	/* Open the desired file for reading */
	std::ifstream readstream(argv[2], ios::in | ios::binary);

	// int fd;
	// if ((fd = open(argv[2], O_RDONLY)) < 0) {
	// 	std::cerr << "Error: could not open desired file; exiting. " << std::endl;
	// 	exit(EXIT_FAILURE);
	// }

	std::string tmpstr(argv[1]);
	std::cerr << "Waiting on port " + tmpstr << std::endl;

	struct TCPHeader tcphdr_syn;
	bytesReceived = recvfrom(sockfd, (void *)&tcphdr_syn, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, &addressLength);

	if (bytesReceived < 0) {
		std::cerr << "Uh oh, things screwed up. " << std::endl;
	}

	struct TCPHeader tcphdr_response;
	int initSeqNum = rand() % MAXSEQNUM;
	setFields((struct TCPHeader *)&tcphdr_response, initSeqNum, tcphdr.seqnum + 1, RECEIVEWINSIZE, true, true, false);

	sendto(sockfd, (struct TCPHeader *)&tcphdr_response, sizeof(tcphdr_response), 0, (struct sockaddr *)&clientAddress, addressLength);



		//sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientAddress, addressLength);

	close(sockfd);
}
