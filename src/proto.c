#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

#include "proto.h"

inline uint16_t bswap_16(uint16_t num);
inline uint8_t bswap_8(uint8_t num);

uint16_t bswap_16(uint16_t num)
{
	return (num >> 8) | (num << 8);
}

uint8_t bswap_8(uint8_t num)
{
	return (num >> 4) | (num << 4);
}

union eoip_hdr populate_hdr(int tid)
{
	union eoip_hdr header;

	header.eoip.tid = bswap_16(htons(tid));
	memcpy(&header.header, EOIP_MAGIC, 4);

	return header;
}

union eoip_hdr populate_hdr6(int tid)
{
	union eoip_hdr header;

	header.eoip6.tid = htons(tid);
	header.header[0] = (bswap_8(header.header[0]) & 0xf0) | EOIPV6_VERSION;

	return header;
}

union eoip_hdr eoip_header(int af, int tid)
{
	switch (af) {
	case AF_INET:
		return populate_hdr(tid);
	case AF_INET6:
		return populate_hdr6(tid);
	default:
		break;
	}
}
