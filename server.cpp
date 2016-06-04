#include <cstring>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <stdio.h> 
#include <stdlib.h>

#include <time.h>
#include "TCPHeader.h"

#define BUFSIZE				1032
#define MAXSEQNUM 			30720
#define INITCONGWINSIZE 	1024
#define INITSSTHRESH		30720
#define RECEIVEWINSIZE		30720
#define MSS					1024

void
printACK(unsigned int ackNum)
{
	//CHANGE TO STDOUT LATER
	fprintf(stderr, "Receiving ACK packet %u \n", ackNum);
}

void
printDATA(unsigned int seqNum, unsigned int CWND, unsigned int SSThresh, bool retransmission)
{
	//CHANGE TO STDOUT LATER
	fprintf(stderr, "Sending data packet %u %u %u ", seqNum, CWND, SSThresh);
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stderr, "\n");
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

	uint16_t initSeqNum = rand() % MAXSEQNUM;
	int currSeqNum = initSeqNum;
	fsmstate curr_state = HANDSHAKE;

	while (1) {
		struct TCPHeader tcphdr_in;
		bytesReceived = recvfrom(sockfd, buf, BUFSIZE, MSG_DONTWAIT, (struct sockaddr *)&clientAddress, &addressLength);

		if (bytesReceived < (int) sizeof(struct TCPHeader)) {
			//std::cerr << "Invalid packet or no message to get yet." << std::endl;
			continue;
		}

		memcpy((struct TCPHeader *)&tcphdr_in, buf, sizeof(struct TCPHeader));

		if (curr_state == HANDSHAKE) {
			if (getSYN((struct TCPHeader *)&tcphdr_in)) {
				fprintf(stderr, "Received a SYN.\n");
				struct TCPHeader tcphdr_synack;
				setFields((struct TCPHeader *)&tcphdr_synack, initSeqNum, getSeqNum((struct TCPHeader *)&tcphdr_in.seqnum) + 1, RECEIVEWINSIZE, true, true, false);
				sendto(sockfd, (struct TCPHeader *)&tcphdr_synack, sizeof(tcphdr_synack), 0, (struct sockaddr *)&clientAddress, addressLength);
				std::cerr << "Just sent out a SYN-ACK. " << std::endl;
				curr_state = TRANSFER;
				continue;
			}
		}

		else if (curr_state == TRANSFER) {
			if (getACK((struct TCPHeader *)&tcphdr_in)) {
				printACK(getAckNum((struct TCPHeader *)&tcphdr_in));
				struct TCPHeader tcphdr_out;
				setFields((struct TCPHeader *)&tcphdr_out, getAckNum((struct TCPHeader *)&tcphdr_in), getSeqNum((struct TCPHeader *)&tcphdr_in), RECEIVEWINSIZE, false, false, false);
				memcpy(buf, (struct TCPHeader *)&tcphdr_out, sizeof(struct TCPHeader));
				readstream.read(buf + sizeof(struct TCPHeader), MSS);

				printDATA(getSeqNum((struct TCPHeader *)&tcphdr_out), INITCONGWINSIZE, INITSSTHRESH, false);
				sendto(sockfd, buf, sizeof(struct TCPHeader) + readstream.gcount(), 0, (struct sockaddr *)&clientAddress, addressLength);

				if (readstream.gcount() < MSS) {
					curr_state = TEARDOWN;
					continue;
				}
			}
		}


		else if (curr_state == TEARDOWN) {
			if ((!getFIN((struct TCPHeader *)&tcphdr_in)) && getACK((struct TCPHeader *)&tcphdr_in)) { //if it is the last ACK
				printACK(getAckNum((struct TCPHeader *)&tcphdr_in));
				fprintf(stderr, "This was the last ACK.\n");

				fprintf(stderr, "Sending a FIN.\n");
				struct TCPHeader tcphdr_fin;
				setFields((struct TCPHeader *)&tcphdr_fin, 0, 0, RECEIVEWINSIZE, false, false, true);
				sendto(sockfd, (struct TCPHeader *)&tcphdr_fin, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, addressLength);
			}

			else if (getFIN((struct TCPHeader *)&tcphdr_in) && getACK((struct TCPHeader *)&tcphdr_in)) { //if it is FIN-ACK
				fprintf(stderr, "Got the FIN-ACK. Sending an ACK and exiting.\n");
				struct TCPHeader tcphdr_ack;
				setFields((struct TCPHeader *)&tcphdr_ack, 0, 0, RECEIVEWINSIZE, true, false, false);
				sendto(sockfd, (struct TCPHeader *)&tcphdr_ack, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, addressLength);
				break;
			}
		}
	}


	//sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientAddress, addressLength);

	readstream.close();

	close(sockfd);
}
