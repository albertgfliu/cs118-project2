#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include "TCPHeader.h"

void 
setSeqNum(struct TCPHeader *hdr, uint16_t newseqnum) 
{
	hdr->seqnum = htons(newseqnum);
}

void 
setAckNum(struct TCPHeader *hdr, uint16_t newacknum)
{
	hdr->acknum = htons(newacknum);
}

void 
setWindow(struct TCPHeader *hdr, uint16_t newwindow)
{
	hdr->window = htons(newwindow);
}

void 
setNU_ASF(struct TCPHeader *hdr, bool ACK, bool SYN, bool FIN)
{
	memset(&(hdr->nu_asf), 0, sizeof(hdr->nu_asf));
	if (ACK == true) {
		hdr->nu_asf |= (1 << 2);	
	}
	if (SYN == true) {
		hdr->nu_asf |= (1 << 1);
	}
	if (FIN == true) {
		hdr->nu_asf |= 1;
	}
}

uint16_t 
getSeqNum(struct TCPHeader *hdr)
{
	return ntohs(hdr->seqnum);
}

uint16_t 
getAckNum(struct TCPHeader *hdr)
{
	return ntohs(hdr->acknum);
}

uint16_t 
getWindow(struct TCPHeader *hdr)
{
	return ntohs(hdr->window);
}

bool 
getACK(struct TCPHeader *hdr)
{
	uint16_t field = hdr->nu_asf;
	field = ((field >> 2) & 1);
	if(field == 1) {
		return true;
	}
	return false;
}

bool 
getSYN(struct TCPHeader *hdr)
{
	uint16_t field = hdr->nu_asf;
	field = ((field >> 1) & 1);
	if(field == 1) {
		return true;
	}
	return false;
}

bool 
getFIN(struct TCPHeader *hdr)
{
	uint16_t field = hdr->nu_asf;
	field = ((field) & 1);
	if(field == 1) {
		return true;
	}
	return false;
}