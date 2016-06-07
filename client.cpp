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
	std::ofstream writestream("received.data", std::ios::in | std::ios::trunc | std::ios::binary);

	/*Basic initializations*/
	char buf[BUFSIZE]; //initiailize temporary buffer for storing data
	uint16_t currSeqNum = rand() % MAXSEQNUM; //initialize random sequence number
	uint16_t currAckNum = 0;

	uint16_t expectedSeqNum = 0;

	int bytesReceived;
	fsmstate curr_state = HANDSHAKE;

	Packet syn_packet;
	int syn_tries = 0;
	bool syn_acked = false;

	struct timespec fin_timer;
	Packet fin_packet;

	while(1) {

		if(curr_state == HANDSHAKE) {
			/*Set initial SYN TCPHeader*/

			syn_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, false, true, false);
			clock_gettime(CLOCK_MONOTONIC, &syn_packet.m_time);

			sendto(sockfd, (void *)&syn_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
			syn_packet.printSYNSend(false);

			syn_tries++;

			curr_state = TRANSFER;
			continue;
		}

		//check for syn-packet expiration
		if (syn_acked == false) {
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (syn_packet.hasExpired(now, TIMEOUT)) {
				if (syn_tries > 2) {
					fprintf(stderr, "Could not connect to server in 3 tries. Exiting.\n");
					break;
				}
				sendto(sockfd, (void *)&syn_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				syn_packet.printSYNSend(true);
				clock_gettime(CLOCK_MONOTONIC, &syn_packet.m_time);
				syn_tries++;
			}
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
				received_packet.printSeqReceive();

				//fprintf(stderr, "Received a SYN-ACK.\n");
				currSeqNum = received_packet.getAckNumber();
				currAckNum = received_packet.getSeqNumber() + 1; //consume the SYN-ACK

				expectedSeqNum = currAckNum;

				/*Send ACK back to begin the process of transfer*/

				Packet ack_packet;
				ack_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, false);
				sendto(sockfd, (void *)&ack_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				ack_packet.printAckSend(false, false, false);

				syn_acked = true;
			}
			
			/*if data packet, i.e. ASF = 000, send ACK back*/
			else if (received_packet.isDATA()) {
				received_packet.printSeqReceive();

				if (received_packet.getSeqNumber() == expectedSeqNum) {
					int payloadSize = bytesReceived - sizeof(struct TCPHeader);
					//fprintf(stderr, "Payload Size = %d\n", payloadSize);

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
				delivery_packet.printAckSend(false, false, false);
			}

			/*if FIN packet from server, send FIN-ACK back and transition to teardown phase*/
			else if (received_packet.isFIN()) {
				received_packet.printSeqReceive();
				//fprintf(stderr, "Received FIN.\n");

				currSeqNum++;
				fin_packet.setHeaderFields(currSeqNum, currAckNum, RECEIVEWINSIZE, true, false, true);

				sendto(sockfd, (void *)&fin_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&serverAddress, addressLength);
				fin_packet.printAckSend(false, false, true);
				curr_state = TEARDOWN;
				continue;
			}

		}

		if (curr_state == TEARDOWN) {
			/*Retransmit FIN here if no reply is received*/


			/*if ACK from server, we are done*/
			if (received_packet.isACK()) {
				received_packet.printSeqReceive();
				//std::cerr << "Received ACK, terminating client." << std::endl;
				break;
			}
		}
	}

	writestream.close();
	close(sockfd);
}