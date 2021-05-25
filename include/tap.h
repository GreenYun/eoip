#ifndef __EOIP_TAP_H__
#define __EOIP_TAP_H__

#if defined(__linux__)
#include <linux/if_tun.h>
#define TUNNEL_DEV "/dev/net/tun"
#elif defined(__FreeBSD__)
#include <net/if_tap.h>
#define TUNNEL_DEV "/dev/tap"
#elif defined(__OpenBSD__)
#define TAP_COUNT 4
#elif defined(__APPLE__)
#define TAP_COUNT 16
#else
#error Unsupported platform, will not build.
#endif

#define TAP_FAILED 1
#define TAP_ERRSETMTU 2
#define TAP_ERRSETNAME 3

/* Try to create a TAP interface with given ifname and mtu.
 * 
 * Return: 0 on success;
 *         TAP_FAILED on failed;
 *         TAP_ERRSETMTU on failed to set MTU;
 *         TAP_ERRSETNAME if we can't set ifname, in this case, read ifname back for name.
 * 
 * REMARKS: errno will be set when returned value != 0 (linux)
 */
int make_tap(int *fd, char *ifname, int mtu);

/* Receive data from the TAP and transfer to socket.
 */
void tap_listen(int fd, int sock_fd, int tid, sa_family_t af,
		const struct sockaddr *raddr, socklen_t raddrlen);

#endif /* __EOIP_TAP_H__ */
