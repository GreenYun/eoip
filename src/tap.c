#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "packet.h"
#include "proto.h"
#include "sock.h"
#include "tap.h"

int make_tap(int *fd, char *ifname, int mtu)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof ifr);

#if defined(__APPLE__) || defined(__OpenBSD__)
	ifr.ifr_flags |= IFF_LINK0;
	char devpath[64];
	for (int dev = 0; dev < TAP_COUNT; dev++) {
		snprintf(devpath, 64, "/dev/tap%d", dev);
		if ((*fd = open(devpath, O_RDWR))) {
			ioctl(*fd, SIOCGIFFLAGS, &ifr); // get old flags
			ioctl(*fd, SIOCSIFFLAGS, &ifr); // set IFF_LINK0
			snprintf(ifname, IFNAMSIZ, "tap%d", dev);
			return TAP_ERRSETNAME;
		}
	}
	return TAP_FAILED;
#else
	*fd = open(TUNNEL_DEV, O_RDWR);
#endif

#if defined(__linux__)
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (ioctl(*fd, TUNSETIFF, (void *)&ifr))
		return TAP_FAILED;

	ifr.ifr_mtu = mtu;
	if (ioctl(socket(AF_INET, SOCK_STREAM, IPPROTO_IP), SIOCSIFMTU,
		  (void *)&ifr))
		return TAP_ERRSETMTU;

#elif defined(__FreeBSD__)
	ioctl(*fd, TAPGIFNAME, &ifr);
	strncpy(ifname, ifr.ifr_name, IFNAMSIZ);
	return TAP_ERRSETMTU;
#endif

	return 0;
}

void tap_listen(int fd, int sock_fd, int tid, sa_family_t af,
		const struct sockaddr *raddr, socklen_t raddrlen)
{
	union eoip_hdr header;
	union packet packet;
	int len;

	/* build the header
	 */
	header = eoip_header(af, tid);

	/* fill the header
	 */
	switch (af) {
	case AF_INET:
		memcpy(&packet.eoip, &header, 8);
		break;

	case AF_INET6:
		memcpy(&packet.eoip6, &header, 2);
		break;

	default:
		break;
	}

	while (1) {
		if (af == AF_INET) {
			if ((len = read(fd, packet.eoip.payload,
					BUFFER_SIZE - 8)) < 0)
				continue;
			packet.eoip.len = htons(len);
			len += 8;
		} else if (af == AF_INET6) {
			if ((len = read(fd, packet.eoip6.payload,
					BUFFER_SIZE - 4)) < 0)
				continue;
			len += 2;
		}

		sendto(sock_fd, packet.buffer, len, 0, raddr, raddrlen);
	}
}
