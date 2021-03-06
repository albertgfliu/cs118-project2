#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "TCPHeader.h"
#include <time.h>

class Packet
{
	public:
		/* Fields */
		char data[1024];
		struct TCPHeader m_header;
		int m_size;
		struct timespec m_time;

		/* Constructors */
		Packet();
		Packet(char* buf, unsigned int length);

		/* Utilities */
		void copyIntoBuf(char* buf);
		void copyHeaderIntoBuf(char* buf);
		void setHeaderFields(uint16_t newseqnum, uint16_t newacknum, uint16_t newwindow, bool ACK, bool SYN, bool FIN);
		uint16_t getSeqNumber();
		uint16_t getAckNumber();
		uint16_t getWindowSize();

		uint16_t getExpectedAckNumber(int maxSeqNum);

		/* Print Functions */
		void printSYNSend(bool retransmission);
		void printSeqReceive();
		void printAckSend(bool retransmission, bool synflag, bool finflag);

		void printSeqSend(unsigned int CWND, unsigned int SSThresh, bool retransmission, bool synflag, bool finflag);
		void printAckReceive();

		/* Boolean Identification Functions */
		bool isSYN();
		bool isACK();
		bool isSYNACK();
		bool isFIN();
		bool isFINACK();
		bool isDATA();

		bool hasExpired(struct timespec now, int duration_ns);
		
		
};

#endif