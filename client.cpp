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
#include <list>

#include <time.h>
#include "Packet.h"


#define BUFSIZE 			1032
#define MAXSEQNUM 			30720
#define INITCONGWINSIZE 	1024
#define INITSSTHRESH		30720
#define RECEIVEWINSIZE		30720
#define TIMEOUT				500000000

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

	uint16_t expectedSeqNum = 0;

	int bytesReceived;
	fsmstate curr_state = HANDSHAKE;

	while(1) {

		if(curr_state == HANDSHAKE) {
			/*Set initial SYN TCPHeader*/

			Packet syn_packet;
			syn_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, false, true, false);

			sendto(sockfd, (void *)&syn_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);

			curr_state = TRANSFER;
			continue;
		}

		bytesReceived = recvfrom(sockfd, buf, BUFSIZE, MSG_DONTWAIT, (struct sockaddr *)&serverAddress, &addressLength);

		if (bytesReceived < (int) sizeof(struct TCPHeader)) {
			//std::cerr << "Problem with received packet or didn't receive anything yet." << std::endl;
			continue;
		}

		Packet received_packet(buf, bytesReceived);

		if (curr_state == TRANSFER) {
			/*if SYN-ACK packet was received*/

			if (received_packet.isSYNACK()) {
				fprintf(stderr, "Received a SYN-ACK.\n");
				currSeqNum = received_packet.getAckNumber();
				currAckNum = received_packet.getSeqNumber() + 1; //consume the SYN-ACK

				expectedSeqNum = currAckNum;

				/*Send ACK back to begin the process of transfer*/

				Packet ack_packet;
				ack_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);
				sendto(sockfd, (void *)&ack_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
			}
			
			/*if data packet, i.e. ASF = 000, send ACK back*/
			else if (received_packet.isDATA()) {
				received_packet.printSeqReceive();

				if (received_packet.getSeqNumber() == expectedSeqNum) {
					int payloadSize = bytesReceived - sizeof(struct TCPHeader);
					fprintf(stderr, "Payload Size = %d\n", payloadSize);

					// for (int i = 0; i < payloadSize; i++) {
					// 	fprintf(stderr, "%c",received_packet.data[i]);
					// }
					// fprintf(stderr, "\n");

					writestream.write(received_packet.data, payloadSize);
					writestream.flush();
					currAckNum = received_packet.getSeqNumber() + payloadSize;
					if(currAckNum > MAXSEQNUM) {
						currAckNum -= MAXSEQNUM;
						//fprintf(stderr, "set it to %u\n", currAckNum);
					}
					expectedSeqNum = currAckNum;
				}

				/* currAckNum will still be our expected AckNumber */
				Packet delivery_packet;
				delivery_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);

				sendto(sockfd, (void *)&delivery_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				delivery_packet.printAckSend(false);
			}

			/*if FIN packet from server, send FIN-ACK back and transition to teardown phase*/
			else if (received_packet.isFIN()) {
				fprintf(stderr, "Received FIN.\n");

				Packet fin_packet;
				currSeqNum++;
				fin_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, true);

				sendto(sockfd, (void *)&fin_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				curr_state = TEARDOWN;
				continue;
			}
		}

		if (curr_state == TEARDOWN) {

			/*if ACK from server, we are done*/
			if (received_packet.isACK()) {
				std::cerr << "Received ACK, terminating client." << std::endl;
				break;
			}
		}
	}

	writestream.close();
	close(sockfd);
}