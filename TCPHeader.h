#ifndef TCPHEADER_H
#define TCPHEADER_H

#include <stdint.h>

struct TCPHeader {
	uint16_t seqnum;
	uint16_t acknum;
	uint16_t window;
	uint16_t nu_asf;
};

void setSeqNum(struct TCPHeader *hdr, uint16_t newseqnum);
void setAckNum(struct TCPHeader *hdr, uint16_t newacknum);
void setWindow(struct TCPHeader *hdr, uint16_t newwindow);
void setNU_ASF(struct TCPHeader *hdr, bool ACK, bool SYN, bool FIN);
void setFields(struct TCPHeader *hdr, uint16_t newseqnum, uint16_t newacknum, uint16_t newwindow, bool ACK, bool SYN, bool FIN);

uint16_t getSeqNum(struct TCPHeader *hdr);
uint16_t getAckNum(struct TCPHeader *hdr);
uint16_t getWindow(struct TCPHeader *hdr);
bool getACK(struct TCPHeader *hdr);
bool getSYN(struct TCPHeader *hdr);
bool getFIN(struct TCPHeader *hdr);

#endif