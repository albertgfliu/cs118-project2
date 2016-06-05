#include <stdint.h>
#include "TCPHeader.h"
#include "Packet.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

Packet::Packet()
{
	m_size = sizeof(struct TCPHeader);
}

Packet::Packet(char* buf, unsigned int length)
{
	memcpy((void *)&m_header, buf, sizeof(struct TCPHeader));
	memcpy(data, buf, length - sizeof(struct TCPHeader));
	m_size = length;
}

bool 
Packet::isSYN()
{
	if (!getACK((struct TCPHeader *)&m_header) && getSYN((struct TCPHeader *)&m_header) && !getFIN((struct TCPHeader *)&m_header)) {
		return true;
	}
	return false;
}
		
bool 
Packet::isACK()
{
	if (getACK((struct TCPHeader *)&m_header) && !getSYN((struct TCPHeader *)&m_header) && !getFIN((struct TCPHeader *)&m_header)) {
		return true;
	}
	return false;
}

bool 
Packet::isSYNACK()
{
	if (getACK((struct TCPHeader *)&m_header) && getSYN((struct TCPHeader *)&m_header) && !getFIN((struct TCPHeader *)&m_header)) {
		return true;
	}
	return false;
}

bool 
Packet::isFIN()
{
	if (!getACK((struct TCPHeader *)&m_header) && !getSYN((struct TCPHeader *)&m_header) && getFIN((struct TCPHeader *)&m_header)) {
		return true;
	}
	return false;
}

bool 
Packet::isFINACK()
{
	if (getACK((struct TCPHeader *)&m_header) && !getSYN((struct TCPHeader *)&m_header) && getFIN((struct TCPHeader *)&m_header)) {
		return true;
	}
	return false;
}

bool 
Packet::isDATA()
{
	if (!getACK((struct TCPHeader *)&m_header) && !getSYN((struct TCPHeader *)&m_header) && !getFIN((struct TCPHeader *)&m_header)) {
		return true;
	}
	return false;
}

void 
Packet::setHeaderFields(uint16_t newseqnum, uint16_t newacknum, uint16_t newwindow, bool ACK, bool SYN, bool FIN)
{
	setFields((struct TCPHeader *)&m_header, newseqnum, newacknum, newwindow, ACK, SYN, FIN);
}

uint16_t 
Packet::getSeqNumber()
{
	return getSeqNum((struct TCPHeader *)&m_header);
}

uint16_t 
Packet::getAckNumber()
{
	return getAckNum((struct TCPHeader *)&m_header);
}

uint16_t 
Packet::getWindowSize()
{
	return getWindow((struct TCPHeader *)&m_header);
}

void
Packet::printSeqNum()
{
	fprintf(stdout, "Receiving data packet %u \n", getSeqNumber());
}

void
Packet::printAckNum(bool retransmission)
{
	fprintf(stdout, "Sending ACK packet %u ", getAckNumber());
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stdout, "\n");
}