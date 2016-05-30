#include <cstring>
#include <string>
#include <thread>
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define BUFLEN 1040

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
	int addressLength = sizeof(serverAddress);
	memset((char *)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(std::atoi(argv[2]));
	memcpy((void *)&serverAddress.sin_addr, hp->h_addr_list[0], hp->h_length);


	char buf[BUFLEN];
	char *tempstr = "This is the first message.";
	memcpy(buf, tempstr, strlen(tempstr));
	if (sendto(sockfd, buf, strlen(tempstr), 0, (struct sockaddr *)&serverAddress, addressLength) < 0) {
		std::cout << "Couldn't send. Sorry!" << std::endl;
	}

	//The client should first start by making a handshake.

	close(sockfd);
}