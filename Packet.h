#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "TCPHeader.h"
#include <time.h>

class Packet
{
	public:
		char data[1024];
		struct TCPHeader m_header;
		Packet();
		Packet(char* buf, unsigned int length);
		bool isSYN();
		bool isACK();
		bool isSYNACK();
		bool isFIN();
		bool isFINACK();
		bool isDATA();
		struct timeval m_time;
		void setHeaderFields(uint16_t newseqnum, uint16_t newacknum, uint16_t newwindow, bool ACK, bool SYN, bool FIN);
		int m_size;
		uint16_t getSeqNumber();
		uint16_t getAckNumber();
		uint16_t getWindowSize();

};

#endif