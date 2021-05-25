#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "packet.h"
#include "proto.h"
#include "sock.h"

int new_sock(sa_family_t af, in_port_t proto)
{
	return socket(af, SOCK_RAW, proto);
}

int sock_bind(int fd, const struct sockaddr *addr, const socklen_t addr_len)
{
	return bind(fd, addr, addr_len);
}

#if defined(__linux__)
int sock_set_dev(int fd, const char *devname, socklen_t devname_len)
{
	return setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, devname,
			  devname_len + 1);
}
#endif

void sock_listen(int fd, int tap_fd, int tid, sa_family_t af,
		 struct sockaddr *raddr)
{
	uint8_t *buffer;
	int len;
	union packet packet;
	union eoip_hdr header;
	struct sockaddr_storage saddr;
	socklen_t saddr_len = sizeof saddr;

	/* pre-build the header
	 */
	header = eoip_header(af, tid);

	while (1) {
		len = recvfrom(fd, packet.buffer, sizeof packet, 0,
			       (struct sockaddr *)&saddr, &saddr_len);

		if (af == AF_INET) {
			/* src check
			 */
			if (((struct sockaddr_in *)&saddr)->sin_addr.s_addr !=
			    ((struct sockaddr_in *)raddr)->sin_addr.s_addr)
				continue;

			buffer = packet.buffer;

			/* skip headres
			 */
			buffer += packet.ip.ip_hl * 4;
			len -= packet.ip.ip_hl * 4 + 8;

			/* sanity checks
			 */
			if (/* len left < header size
			     */
			    len <= 0 ||
			    /* not a EOIP packet
			     */
			    memcmp(buffer, EOIP_MAGIC, 4) ||
			    /* payload len mismatch
			     */
			    len != ntohs(((uint16_t *)buffer)[2]) ||
			    /* tid mismatch
			     */
			    ((uint16_t *)buffer)[3] != tid)
				continue;

			buffer += 8;
		} else if (af == AF_INET6) {
			/* src check
			 */
			if (memcmp(&((struct sockaddr_in6 *)&saddr)
					    ->sin6_addr.s6_addr,
				   &((struct sockaddr_in6 *)raddr)
					    ->sin6_addr.s6_addr,
				   16))
				continue;

			/* check header.
			 */
			if (len < 2 || memcmp(&packet.header, &header, 2))
				continue;
			buffer = packet.buffer + 2;
			len -= 2;
		}

		if (len <= 0)
			continue;

		write(tap_fd, buffer, len);
	}
}

void populate_sockaddr(in_port_t port, sa_family_t af, const char *addr,
		       struct sockaddr_storage *dst, socklen_t *addrlen)
{
	// from https://stackoverflow.com/questions/48328708/, many thanks.
	if (af == AF_INET) {
		struct sockaddr_in *dst_in4 = (struct sockaddr_in *)dst;
		*addrlen = sizeof(*dst_in4);
		memset(dst_in4, 0, *addrlen);
		dst_in4->sin_family = af;
		dst_in4->sin_port = htons(port);
		inet_pton(af, addr, &dst_in4->sin_addr);
	} else if (af == AF_INET6) {
		struct sockaddr_in6 *dst_in6 = (struct sockaddr_in6 *)dst;
		*addrlen = sizeof(*dst_in6);
		memset(dst_in6, 0, *addrlen);
		dst_in6->sin6_family = af;
		dst_in6->sin6_port = htons(port);
		inet_pton(af, addr, &dst_in6->sin6_addr);
	}
}
