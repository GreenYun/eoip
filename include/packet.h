#ifndef __EOIP_PACKET_H__
#define __EOIP_PACKET_H__

#include <netinet/ip.h>

#include "proto.h"

#define BUFFER_SIZE 65535U

union packet {
	uint16_t header;
	uint8_t buffer[BUFFER_SIZE];
	struct ip ip;
	struct eoip_packet eoip;
	struct eoip6_packet eoip6;
};

#endif /* __EOIP_PACKET_H__ */
