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
#include <iostream>
#include <fstream>

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

void
printACK(unsigned int ackNum, bool retransmission)
{
	fprintf(stdout, "Sending ACK packet %d ", ackNum);
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stdout, "\n");
}

void
printSEQ(unsigned int seqNum)
{
	fprintf(stdout, "Receiving data packet %d \n", seqNum);
}

int
main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cerr << "Usage: " + std::string(argv[0]) + " SERVER-HOST-OR-IP PORT-NUMBER" << std::endl;
		exit(EXIT_FAILURE);
	}

	/*Create a socket*/
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		std::cerr << "Error: could not bind to socket" << std::endl;
		exit(EXIT_FAILURE);
	}

	/*Resolve a host name*/
	struct hostent *hp;
	hp = gethostbyname(argv[1]);
	if (!hp) {
		std::cout << "Error: could not obtain address of host " + std::string(argv[1]) << std::endl;
		exit(EXIT_FAILURE);
	}

	/*Set up parameters of the serverAddress*/
	struct sockaddr_in serverAddress;
	socklen_t addressLength = sizeof(serverAddress);
	memset((char *)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(std::atoi(argv[2]));
	memcpy((void *)&serverAddress.sin_addr, hp->h_addr_list[0], hp->h_length);

	/*Basic initializations*/
	char buf[BUFSIZE]; //initiailize temporary buffer for storing data
	uint16_t currSeqNum = rand() % MAXSEQNUM; //initialize random sequence number
	uint16_t currAckNum = 0;

	/*Set initial SYN TCPHeader*/
	struct TCPHeader tcphdr_syn;
	setFields((struct TCPHeader *)&tcphdr_syn, currSeqNum, currAckNum, RECEIVEWINSIZE, false, true, false);

	/*Send initial SYN TCPHeader out*/
	sendto(sockfd, (void *)&tcphdr_syn, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);

	/*Read SYN-ACK TCPHeader in*/
	struct TCPHeader tcphdr_synack;
	int bytesReceived = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serverAddress, &addressLength);
	if (bytesReceived != sizeof(struct TCPHeader)) {
		std::cerr << "Error with the SYN-ACK, ignoring it for now" << std::endl;
	}
	memcpy((struct TCPHeader *)&tcphdr_synack, buf, bytesReceived);
	currSeqNum = getAckNum((struct TCPHeader *)&tcphdr_synack);
	currAckNum = getSeqNum((struct TCPHeader *)&tcphdr_synack) + 1;

	/*Send ACK back to begin the process of transfer*/
	struct TCPHeader tcphdr_ack;
	setFields((struct TCPHeader *)&tcphdr_ack, currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);

	sendto(sockfd, (void *)&tcphdr_ack, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);

	std::ofstream writestream("transferred_file", ios::in | ios::trunc | ios::binary);

	while(1) {
		struct TCPHeader tcphdr_curr;
		bytesReceived = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serverAddress, &addressLength);
		if (bytesReceived < (int) sizeof(struct TCPHeader)) {
			std::cerr << "Problem with received packet" << std::endl;
			break;
		}
		memcpy((struct TCPHeader *)&tcphdr_curr, buf, sizeof(struct TCPHeader));

		/*If this is a data packet, i.e. ASF set to 000*/
		if (bytesReceived > (int) sizeof(struct TCPHeader)) {
			printSEQ(getSeqNum((struct TCPHeader *)&tcphdr_curr));
			int payloadSize = bytesReceived - sizeof(struct TCPHeader);
			writefile.write(buf + sizeof(struct TCPHeader), payloadSize);

			struct TCPHeader tcphdr_response;
			currSeqNum++;
			currAckNum = getSeqNum((struct TCPHeader *)&tcphdr_curr) + payloadSize;
			setFields((struct TCPHeader *)&tcphdr_ack, currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);

			sendto(sockfd, (void *)&tcphdr_response, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
			printACK(getAckNum((struct TCPHeader *)&tcphdr_response), false);
		}
	}

	writestream.close();
	close(sockfd);
}