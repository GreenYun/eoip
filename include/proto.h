#ifndef __EOIP_PROTO_H__
#define __EOIP_PROTO_H__

#include <stdint.h>

#define PROTO_EOIP 47
#define PROTO_EOIP6 97
#define EOIP_MAGIC "\x20\x01\x64\x00"
#define EOIPV6_VERSION (uint8_t)0x03

/* EOIP packet
 */
struct eoip_packet {
	uint8_t magic[4];
	uint16_t len;
	uint16_t tid;
	uint8_t payload[0];
};

/* EOIPv6 packet
 */
struct eoip6_packet {
	uint16_t tid;
	uint8_t payload[0];
};

/* EOIP header
 */
union eoip_hdr {
	struct eoip_packet eoip;
	struct eoip6_packet eoip6;
	uint8_t header[8];
	uint16_t header_v;
};

union eoip_hdr populate_hdr(int tid);
union eoip_hdr populate_hdr6(int tid);
union eoip_hdr eoip_header(int af, int tid);

#endif /* __EOIP_PROTO_H__ */
