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
	fprintf(stdout, "Sending ACK packet %u ", ackNum);
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stdout, "\n");
}

void
printSEQ(unsigned int seqNum)
{
	fprintf(stdout, "Receiving data packet %u \n", seqNum);
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

	/*Open file for writing*/
	std::ofstream writestream("transferred_file", std::ios::in | std::ios::trunc | std::ios::binary);

	/*Basic initializations*/
	char buf[BUFSIZE]; //initiailize temporary buffer for storing data
	uint16_t currSeqNum = rand() % MAXSEQNUM; //initialize random sequence number
	uint16_t currAckNum = 0;

	int bytesReceived;
	fsmstate curr_state = HANDSHAKE;
	while(1) {

		if(curr_state == HANDSHAKE) {
			/*Set initial SYN TCPHeader*/
			struct TCPHeader tcphdr_syn;
			setFields((struct TCPHeader *)&tcphdr_syn, currSeqNum, currAckNum, RECEIVEWINSIZE, false, true, false);

			/*Send initial SYN TCPHeader out*/
			sendto(sockfd, (void *)&tcphdr_syn, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);

			curr_state = TRANSFER;
			continue;
		}

		struct TCPHeader tcphdr_in;
		bytesReceived = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serverAddress, &addressLength);
		if (bytesReceived < (int) sizeof(struct TCPHeader)) {
			std::cerr << "Problem with received packet or didn't receive anything yet" << std::endl;
			continue;
		}

		memcpy((struct TCPHeader *)&tcphdr_in, buf, sizeof(struct TCPHeader));

		if (curr_state == TRANSFER) {
			/*if SYN-ACK packet was received*/
			if(getACK((struct TCPHeader *)&tcphdr_in) && getSYN((struct TCPHeader *)&tcphdr_in) && !getFIN((struct TCPHeader *)&tcphdr_in)) {
				fprintf(stderr, "Received a SYN-ACK.\n");
				currSeqNum = getAckNum((struct TCPHeader *)&tcphdr_in);
				currAckNum = getSeqNum((struct TCPHeader *)&tcphdr_in) + 1; //consume the SYN-ACK

				/*Send ACK back to begin the process of transfer*/
				struct TCPHeader tcphdr_ack;
				setFields((struct TCPHeader *)&tcphdr_ack, currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);
				sendto(sockfd, (void *)&tcphdr_ack, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
			}
			
			/*if data packet, i.e. ASF = 000, send ACK back*/
			else if (!getACK((struct TCPHeader *)&tcphdr_in) && !getSYN((struct TCPHeader *)&tcphdr_in) && !getFIN((struct TCPHeader *)&tcphdr_in)) {
				printSEQ(getSeqNum((struct TCPHeader *)&tcphdr_in));
				int payloadSize = bytesReceived - sizeof(struct TCPHeader);
				fprintf(stderr, "Payload Size = %d\n", payloadSize);
				writestream.write(buf + sizeof(struct TCPHeader), payloadSize);
				writestream.flush();

				struct TCPHeader tcphdr_out;
				//currSeqNum++; sequence number of packets from client NOT incremented unless consuming SYN-ACK or FIN
				currAckNum = getSeqNum((struct TCPHeader *)&tcphdr_in) + payloadSize;
				setFields((struct TCPHeader *)&tcphdr_out, currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);

				sendto(sockfd, (void *)&tcphdr_out, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				printACK(getAckNum((struct TCPHeader *)&tcphdr_out), false);
			}

			/*if FIN packet from server, send FIN-ACK back and transition to teardown phase*/
			else if (!getACK((struct TCPHeader *)&tcphdr_in) && !getSYN((struct TCPHeader *)&tcphdr_in) && getFIN((struct TCPHeader *)&tcphdr_in)) {
				fprintf(stderr, "Received FIN.\n");

				struct TCPHeader tcphdr_out;
				currSeqNum++; //consuming a FIN, so incrementing count
				setFields((struct TCPHeader *)&tcphdr_out, currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, true);

				sendto(sockfd, (void *)&tcphdr_out, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				curr_state = TEARDOWN;
				continue;
			}
		}

		if (curr_state == TEARDOWN) {

			/*if ACK from server, we are done*/
			if (getACK((struct TCPHeader *)&tcphdr_in) && !getSYN((struct TCPHeader *)&tcphdr_in) && !getFIN((struct TCPHeader *)&tcphdr_in)) {
				fprintf(stderr, "Received ACK, terminating client.\n");
				break;
			}
		}
	}

	writestream.close();
	close(sockfd);
}