#include <errno.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include "proto.h"
#include "sock.h"
#include "tap.h"
#include "utils.h"

static void usage(void)
{
#if defined(__linux__)
	fprintf(stderr,
		"Usage: eoip [ OPTIONS ] IFNAME { remote RADDR } { local LADDR } [ dev DEVICE ]\n"
		"                               [ id TID ] [ mtu MTU ] [ uid UID ] [ gid GID ]\n"
		"                               [fork]\n"
		"OPTIONS := { -4 | -6 }\n");
#elif
	fprintf(stderr,
		"Usage: eoip [ OPTIONS ] IFNAME { remote RADDR } { local LADDR } [ id TID ]\n"
		"                               [ mtu MTU ] [ uid UID ] [ gid GID ] [fork]\n"
		"OPTIONS := { -4 | -6 }\n");
#endif

	exit(-1);
}

/* Set the name of the caller, inspired from NGINX's ngx_setproctitle.c
 */
void setprocname(char *name, char **dst)
{
	extern char **environ;

	size_t size = 0;

	for (int i = 0; environ[i]; i++)
		size += strlen(environ[i]) + 1;

	memset(environ, 0, size);

	size = 0;
	for (int i = 0; dst[i]; i++)
		size += strlen(dst[i]) + 1;

	memset(*dst, 0, size);

	dst[1] = NULL;
	strcpy(dst[0], name);
}

int main(int argc, char **argv)
{
	char src[INET6_ADDRSTRLEN];
	char dst[INET6_ADDRSTRLEN];
	char ifname[IFNAMSIZ];
	char procname[128];
	int res;
	int w_dead = 0;
	int sock_fd;
	int tap_fd;
	unsigned int tid = 0;
	unsigned int mtu = 1500;
	unsigned int daemon = 0;

	FILE *pidfile;
	gid_t gid = 0;
	in_port_t proto = PROTO_EOIP;
	pid_t pid;
	pid_t p_dead;
	pid_t p_sender = 1;
	pid_t p_writer = 1;
	uid_t uid = 0;
	sa_family_t af = AF_INET;
	socklen_t laddrlen, raddrlen;
	struct sockaddr_storage laddr, raddr;

#if defined(__linux__)
	char devname[IFNAMSIZ];
	socklen_t devname_len = 0;
#endif

	if (argc < 2)
		usage();

	/* assume that the first argument is IFNAME
	 */
	strncpy(ifname, argv[1], IFNAMSIZ);

	/* parse some args
	 */
	for (int i = 1; i < argc; i++) {
		if (matches(argv[i], "-4"))
			strncpy(ifname, argv[++i], IFNAMSIZ);
		if (matches(argv[i], "-6")) {
			strncpy(ifname, argv[++i], IFNAMSIZ);
			af = AF_INET6;
			proto = PROTO_EOIP6;
		}
		if (matches(argv[i], "id"))
			tid = atoi(argv[++i]);
		if (matches(argv[i], "local"))
			strncpy(src, argv[++i], INET6_ADDRSTRLEN);
		if (matches(argv[i], "remote"))
			strncpy(dst, argv[++i], INET6_ADDRSTRLEN);
		if (matches(argv[i], "mtu"))
			mtu = atoi(argv[++i]);
		if (matches(argv[i], "gid"))
			gid = atoi(argv[++i]);
		if (matches(argv[i], "uid"))
			uid = atoi(argv[++i]);
		if (matches(argv[i], "fork"))
			daemon = 1;

#if defined(__linux__)
		if (matches(argv[i], "dev")) {
			strncpy(devname, argv[++i], IFNAMSIZ);
			devname_len = strlen(devname);
		}
#endif
	}

	/* change UID/GID
	 */
	if (uid > 0 && setuid(uid)) {
		fprintf(stderr, "[ERR] can't set UID: %s\n", strerror(errno));
		exit(errno);
	}

	if (gid > 0 && setgid(gid)) {
		fprintf(stderr, "[ERR] can't set GID: %s\n", strerror(errno));
		exit(errno);
	}

	/* fork to background
	 */
	if (daemon) {
		pid = fork();

		if (pid < 0) {
			fprintf(stderr, "[ERR] can't daemonize: %s\n",
				strerror(errno));
			exit(errno);
		} else if (pid > 0) {
			pidfile = fopen("/run/eoip.pid", "w");
			fprintf(pidfile, "%d\n", pid);
			fclose(pidfile);
			exit(0);
		}
	}

	/* build sockaddr for send/recv
	 */
	populate_sockaddr(proto, af, src, &laddr, &laddrlen);
	populate_sockaddr(proto, af, dst, &raddr, &raddrlen);

	/* bind a sock
	 */
	sock_fd = new_sock(af, proto);

#if defined(__linux__)
	if (sock_set_dev(sock_fd, devname, devname_len) != 0) {
		fprintf(stderr, "[ERR] can't bind socket: %s.\n",
			strerror(errno));
		exit(errno);
	}
#endif

	if (sock_bind(sock_fd, (struct sockaddr *)&laddr, laddrlen) < 0) {
		fprintf(stderr, "[ERR] can't bind socket: %s.\n",
			strerror(errno));
		exit(errno);
	}

	/* bind a tap interface
	 */
	switch (make_tap(&tap_fd, ifname, mtu)) {
	case TAP_FAILED:
		fprintf(stderr, "[ERR] can't TUNSETIFF: %s.\n",
			strerror(errno));
		exit(errno);
	case TAP_ERRSETMTU:
		fprintf(stderr,
			"[WARN] can't SIOCSIFMTU (%s), please set MTU manually.\n",
			strerror(errno));
		break;
	case TAP_ERRSETNAME:
		fprintf(stderr,
			"[WARN] running on OpenBSD/FreeBSD/Darwin, we can't set IFNAME, "
			"the interface will be: %s.\n",
			ifname);
	default:
		break;
	}

	fprintf(stderr,
		"[INFO] attached to %s, mode %s, remote %s, local %s, tid %d, mtu %d.\n",
		ifname, af == AF_INET6 ? "EoIPv6" : "EoIP", dst, src, tid, mtu);

	/* all set, let's get to work
	 */
	snprintf(procname, 128,
		 "eoip: master process (tunnel %d, dst %s, on %s)", tid, dst,
		 ifname);
	setprocname(procname, argv);

	do {
		if (p_writer == 1)
			p_writer = fork();
		if (p_writer < 0) {
			kill(-1, SIGTERM);
			fprintf(stderr,
				"[ERR] failed to start TAP listener.\n");
			exit(errno);
		}

		if (p_writer > 1 && !w_dead)
			p_sender = fork();
		if (p_sender < 0) {
			kill(-1, SIGTERM);
			fprintf(stderr,
				"[ERR] failed to start SOCK listener.\n");
			exit(errno);
		}

		if (w_dead)
			w_dead = 0;
		if (p_writer > 0 && p_sender > 0) {
			p_dead = waitpid(-1, &res, 0);
			if (p_dead == p_writer)
				p_writer = w_dead = 1;
			continue;
		}

#if defined(__linux__)
		prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif

		if (!p_sender) {
			setprocname("eoip: TAP listener", argv);
			tap_listen(tap_fd, sock_fd, tid, af,
				   (struct sockaddr *)&raddr, raddrlen);
		}

		if (!p_writer) {
			setprocname("eoip: SOCKS listener", argv);
			sock_listen(sock_fd, tap_fd, tid, af,
				    (struct sockaddr *)&raddr);
		}
	} while (1);
}
