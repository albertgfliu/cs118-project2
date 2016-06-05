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
#include <list>

#include <time.h>

#include <stdio.h>      /* printf */
#include <string.h>     /* strcat */
#include <stdlib.h>     /* strtol */

#include "Packet.h"

#define BUFSIZE				1032
#define MAXSEQNUM 			30720
#define INITCONGWINSIZE 	1024
#define INITSSTHRESH		30720
#define RECEIVEWINSIZE		30720
#define MSS					1024
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

	/* Inform the user that we are reading on a particular port */
	std::string tmpstr(argv[1]);
	std::cerr << "Waiting on port " + tmpstr << std::endl;

	/* Initialize variables that will inform us about the program's state and change throughput operation */
	uint16_t initSeqNum = (uint16_t)(rand() % MAXSEQNUM);
	uint16_t currWindowSize = INITCONGWINSIZE;
	uint16_t currSSThresh = INITSSTHRESH;
	fsmstate curr_state = HANDSHAKE;



	std::list<Packet> unackedPackets;


	while (1) {

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		std::list<Packet>::iterator it = unackedPackets.begin();
		while (it != unackedPackets.end()) {
			//if current packet is expired, send it out again
			if (it->hasExpired(now, TIMEOUT)) {
				it->copyIntoBuf(buf);
				sendto(sockfd, buf, it->m_size, 0, (struct sockaddr *)&clientAddress, addressLength);

				clock_gettime(CLOCK_MONOTONIC, &(it->m_time)); //start its timer over again
			}
			it++;
		}

		bytesReceived = recvfrom(sockfd, buf, BUFSIZE, MSG_DONTWAIT, (struct sockaddr *)&clientAddress, &addressLength);

		if (bytesReceived < (int) sizeof(struct TCPHeader)) {
			//std::cerr << "Invalid packet or no message to get yet." << std::endl;
			continue;
		}

		Packet received_packet(buf, bytesReceived);

		if (curr_state == HANDSHAKE) {
			if (received_packet.isSYN()) {
				std::cerr << "Received a SYN." << std::endl;

				Packet synack_packet;
				synack_packet.setHeaderFields(initSeqNum, received_packet.getSeqNumber() + 1, RECEIVEWINSIZE, true, true, false);

				sendto(sockfd, (void *)&synack_packet.m_header, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, addressLength);
				std::cerr << "Just sent out a SYN-ACK. " << std::endl;
				curr_state = TRANSFER;
				continue;
			}
		}

		else if (curr_state == TRANSFER) {
			if (received_packet.isACK()) {
				received_packet.printAckReceive();

				// std::list<Packet>::iterator it = unackedPackets.begin();
				// while (it != unackedPackets.end()) {
				// 	uint16_t expectedAckNumber = it->getExpectedAckNumber();

				// 	if (received_packet.getAckNumber() > expectedAckNumber) { //CHECK THIS LOGIC?
				// 		it.remove(); //???
				// 	}
				// 	else {
				// 		it++;
				// 	}
				// }

				/* Loop through packets "in flight", check how much data is out there */
				int bytesSent = 0;

				while (bytesSent < currWindowSize) {

					Packet delivery_packet;
					readstream.read(delivery_packet.data, MSS);
					delivery_packet.setHeaderFields(received_packet.getAckNumber(), received_packet.getSeqNumber(), RECEIVEWINSIZE, false, false, false);
					delivery_packet.m_size = sizeof(struct TCPHeader) + readstream.gcount();

					memset(buf, 0, BUFSIZE);
					delivery_packet.copyIntoBuf(buf);

					delivery_packet.printSeqSend(currWindowSize, currSSThresh, false);

					sendto(sockfd, buf, sizeof(struct TCPHeader) + readstream.gcount(), 0, (struct sockaddr *)&clientAddress, addressLength);

					if (readstream.gcount() < MSS) {
						curr_state = TEARDOWN;
						break;
					}

					bytesSent += readstream.gcount();
				}

				/* If current window size is less than half of current slow start threshold, double window size */
				if (currWindowSize < currSSThresh) { 
					currWindowSize *= 2;
					if (currWindowSize > RECEIVEWINSIZE/2) { //if window size is greater than 30720/2 = 15360, revert back to 15360
						currWindowSize /= 2;
					}
				}
				/* If current window size is greater than or equal to current slow start threshold, increase window size by 1 */
				else {
					currWindowSize += MSS;
					if (currWindowSize > RECEIVEWINSIZE/2) {
						currWindowSize -= MSS;
					}
				}


			}
		}


		else if (curr_state == TEARDOWN) {

			if (received_packet.isACK()) { //last ACK if we are in teardown phase //CHECK FOR MULTIPLE "LAST ACKs" arriving!
				received_packet.printAckReceive();
				std::cerr << "This was the last ACK. Sending a FIN." << std::endl;

				Packet fin_packet;
				fin_packet.setHeaderFields(received_packet.getAckNumber(), received_packet.getSeqNumber(), RECEIVEWINSIZE, false, false, true);
				struct TCPHeader tcphdr_fin;
				setFields((struct TCPHeader *)&tcphdr_fin, 0, 0, RECEIVEWINSIZE, false, false, true);
				sendto(sockfd, (struct TCPHeader *)&tcphdr_fin, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, addressLength);
			}

			else if (received_packet.isFINACK()) { //if received a FIN-ACK, then send an ACK and exit. we don't care if they respond or not.
				fprintf(stderr, "Got the FIN-ACK. Sending an ACK and exiting.\n");
				struct TCPHeader tcphdr_ack;
				setFields((struct TCPHeader *)&tcphdr_ack, received_packet.getAckNumber(), received_packet.getSeqNumber() + 1, RECEIVEWINSIZE, true, false, false);
				sendto(sockfd, (struct TCPHeader *)&tcphdr_ack, sizeof(struct TCPHeader), 0, (struct sockaddr *)&clientAddress, addressLength);
				break;
			}
		}

	}


	//sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientAddress, addressLength);

	readstream.close();

	close(sockfd);
}
