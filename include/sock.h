#ifndef __EOIP_SOCK_H__
#define __EOIP_SOCK_H__

#include <netinet/ip.h>

/* Create a new EOIP socket.
 */
int new_sock(sa_family_t af, in_port_t proto);

/* Try binding the socket.
 */
int sock_bind(int fd, const struct sockaddr *addr, const socklen_t addr_len);

#if defined(__linux__)
/* Specify the device to bind.
 */
int sock_set_dev(int fd, const char *devname, socklen_t devname_len);
#endif

/* Receive data from a socket and transfer to the tap interface.
 */
void sock_listen(int fd, int tap_fd, int tid, sa_family_t af,
		 struct sockaddr *laddr);

/* populate a sockaddr structure.
 */
void populate_sockaddr(in_port_t port, sa_family_t af, const char *addr,
		       struct sockaddr_storage *dst, socklen_t *addrlen);

#endif /* __EOIP_SOCK_H__ */
