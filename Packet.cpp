#include <stdint.h>
#include "TCPHeader.h"
#include "Packet.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>

Packet::Packet()
{
	m_size = sizeof(struct TCPHeader);
	clock_gettime(CLOCK_MONOTONIC, &m_time);
}

Packet::Packet(char *buf, unsigned int length)
{
	memcpy((void *)&m_header, buf, sizeof(struct TCPHeader));
	memcpy(data, buf, length - sizeof(struct TCPHeader));
	m_size = length;
	clock_gettime(CLOCK_MONOTONIC, &m_time);
}

void 
Packet::copyIntoBuf(char *buf)
{
	memcpy((void *)buf, (void *)&m_header, sizeof(struct TCPHeader));
	memcpy((void *)(buf + sizeof(struct TCPHeader)), data, (m_size - sizeof(struct TCPHeader)));
}

void
Packet::copyHeaderIntoBuf(char *buf)
{
	memcpy((void *)buf, (void *)&m_header, sizeof(struct TCPHeader));
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
Packet::printSeqReceive()
{
	fprintf(stdout, "Receiving data packet %u \n", getSeqNumber());
}

void
Packet::printAckSend(bool retransmission)
{
	fprintf(stdout, "Sending ACK packet %u ", getAckNumber());
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stdout, "\n");
}

void
Packet::printAckReceive()
{
	fprintf(stdout, "Receiving ACK packet %u \n", getAckNumber());
}

void
Packet::printSeqSend(unsigned int CWND, unsigned int SSThresh, bool retransmission)
{
	fprintf(stdout, "Sending data packet %u %u %u ", getSeqNumber(), CWND, SSThresh);
	if (retransmission) {
		fprintf(stdout, "Retransmission");
	}
	fprintf(stdout, "\n");
}

bool
Packet::hasExpired(struct timespec now, int duration_ns)
{
	int elapsed_time;
	elapsed_time = (int) (((double)now.tv_sec*pow(10,9) + (double)now.tv_nsec) - ((double)m_time.tv_sec*pow(10,9) + (double)m_time.tv_nsec));
	if (elapsed_time > duration_ns) {
		return true;
	}
	return false;
}