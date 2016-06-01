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
#define MSS					1024

void
printACK(unsigned int ackNum)
{
	fprintf(stdout, "Receiving ACK packet %u \n", ackNum);
}

void
printDATA(unsigned int seqNum, unsigned int CWND, unsigned int SSThresh, bool retransmission)
{
	fprintf(stdout, "Sending data packet %u %u %u ", seqNum, CWND, SSThresh);
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
	std::ifstream readstream(argv[2], std::ios::in | std::ios::binary);


	std::string tmpstr(argv[1]);
	std::cerr << "Waiting on port " + tmpstr << std::endl;

	struct TCPHeader tcphdr_syn;
	bytesReceived = recvfrom(sockfd, (void *)&tcphdr_syn, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, &addressLength);


	if (bytesReceived < 0) {
		std::cerr << "Uh oh, things screwed up. " << std::endl;
	}

	struct TCPHeader tcphdr_synack;
	uint16_t initSeqNum = (uint16_t)(rand() % MAXSEQNUM);
	setFields((struct TCPHeader *)&tcphdr_synack, initSeqNum, getSeqNum((struct TCPHeader *)&tcphdr_syn.seqnum) + 1, RECEIVEWINSIZE, true, true, false);

	sendto(sockfd, (struct TCPHeader *)&tcphdr_synack, sizeof(tcphdr_synack), 0, (struct sockaddr *)&clientAddress, addressLength);

	int currSeqNum = initSeqNum;
	while (1) {
		struct TCPHeader tcphdr_in;
		bytesReceived = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientAddress, &addressLength);

		if (bytesReceived < (int) sizeof(struct TCPHeader)) {
			std::cerr << "Problem with received packet" << std::endl;
			break;
		}
		memcpy((struct TCPHeader *)&tcphdr_in, buf, sizeof(struct TCPHeader));

		if (getACK((struct TCPHeader *)&tcphdr_in)) {
			printACK(getAckNum((struct TCPHeader *)&tcphdr_in));
			struct TCPHeader tcphdr_out;
			setFields((struct TCPHeader *)&tcphdr_out, getAckNum((struct TCPHeader *)&tcphdr_in), getSeqNum((struct TCPHeader *)&tcphdr_in), RECEIVEWINSIZE, false, false, false);
			memcpy(buf, (struct TCPHeader *)&tcphdr_out, sizeof(struct TCPHeader));
			readstream.read(buf + sizeof(struct TCPHeader), MSS);

			printDATA(getSeqNum((struct TCPHeader *)&tcphdr_out), 0, 0, false);
			sendto(sockfd, buf, sizeof(struct TCPHeader) + readstream.gcount(), 0, (struct sockaddr *)&clientAddress, addressLength);
		}

		if (readstream.gcount() < MSS) {
			break;
		}
		
	}

	//sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientAddress, addressLength);

	readstream.close();

	close(sockfd);
}
