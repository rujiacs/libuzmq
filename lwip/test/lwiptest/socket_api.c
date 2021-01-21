#include "lwip/opt.h"
#include "socket_api.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

#if LWIP_SOCKET && LWIP_IPV4 /* this uses IPv4 loopback sockets, currently */

#define TEST_TIME_SECONDS	10
#define TEST_TXRX_BUFSIZE	1000
#define TEST_LOOP			100  


static char *testdata = "This is a test ";

static void
__set_nonblocking(int sock)
{
	int flags = 0, rc;

	flags = lwip_fcntl(sock, F_GETFL, 0);
	if (flags == -1)
		flags = 0;
	rc = lwip_fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	if (rc < 0) {
		fprintf(stderr, "Failed to set sock %d to non-blocking\n", sock);
	}
}

static void
__sockets_server_inline(int sock, struct lwip_sockaddr_in *caddr)
{
	int ret, i = 0;
	ip4_addr_t cip;
	unsigned int remote_id = 9;
	char txbuf[TEST_TXRX_BUFSIZE] = {'\0'};

	cip.addr = caddr->sin_addr.s_addr;
	fprintf(stdout, "Client %s:%u\n",
					ip4addr_ntoa(&cip), caddr->sin_port);

	lwip_setlocalid(sock, 9);
	__set_nonblocking(sock);

	for (i = 0; i < TEST_LOOP; i++) {
		int ret_len = TEST_TXRX_BUFSIZE;

		while (ret_len > 0) {
			ret = lwip_send_netml(sock, txbuf, ret_len, LWIP_MSG_NETML, 8);
			if (ret == -1)
				fprintf(stderr, "failed to send data, err %d\n", errno);
			else {
				fprintf(stdout, "send %d-th data, len %d(%d)\n",
								i, ret, (ret_len - ret));
				ret_len -= ret;
			}
		}
		usleep(50000);
	}
//	while (1) {}
	lwip_close(sock);
}

void
sockets_server(const char *server_ip, u16_t port)
{
	struct lwip_sockaddr_in addr, caddr;
	socklen_t caddr_len;
	int ret, sock, csock;
	ip4_addr_t ip4;

	memset(&addr, 0, sizeof(struct lwip_sockaddr_in));
	if (ip4addr_aton(server_ip, &ip4)) {
		addr.sin_family = LWIP_AF_INET;
		addr.sin_port = lwip_htons(port);
		addr.sin_addr.s_addr = ip4_addr_get_u32(&ip4);
	}
	else
		return;

	sock = lwip_socket(LWIP_AF_INET, LWIP_SOCK_STREAM, 0);
	LWIP_ASSERT("socket sock >= 0", sock >= 0);

	ret = lwip_bind(sock, (struct lwip_sockaddr *)&addr, sizeof(addr));
	LWIP_ASSERT("bind ret == 0", ret == 0);
	fprintf(stdout, "bind\n");

	ret = lwip_listen(sock, 0);
	LWIP_ASSERT("listen ret == 0", ret == 0);
	fprintf(stdout, "listen\n");

	caddr_len = sizeof(caddr);
	memset(&caddr, 0, caddr_len);
	csock = lwip_accept(sock, (struct lwip_sockaddr *)&caddr, &caddr_len);

	__sockets_server_inline(csock, &caddr);
}

void
sockets_client(const char *remote_ip, u16_t remote_port)
{
  ip4_addr_t ip4;
  char rxbuf[TEST_TXRX_BUFSIZE], *ptr = NULL;
  int sock, ret, len;
  struct lwip_sockaddr_in remote_addr;
  u32_t max_time = sys_now() + TEST_TIME_SECONDS * 1000;

  memset(&remote_addr, 0, sizeof(struct lwip_sockaddr_in));
  if (ip4addr_aton(remote_ip, &ip4)) {
    remote_addr.sin_family = LWIP_AF_INET;
    remote_addr.sin_addr.s_addr = ip4_addr_get_u32(&ip4);
	remote_addr.sin_port = lwip_htons(remote_port);
  }
  else
	return;

	usleep(1000);

  sock = lwip_socket(LWIP_AF_INET, LWIP_SOCK_STREAM, 0);
  LWIP_ASSERT("sock >= 0", sock >= 0);

  lwip_setlocalid(sock, 8);
  ret = lwip_connect(sock, (struct lwip_sockaddr *)&remote_addr,
				  sizeof(struct lwip_sockaddr_in));
  LWIP_ASSERT("ret == 0", ret == 0);
  fprintf(stdout, "Connected.\n");
  __set_nonblocking(sock);

	fd_set readset, writeset, errset;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&errset);
	FD_SET(sock, &readset);

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	len = TEST_TXRX_BUFSIZE * TEST_LOOP;
	while (1) {
		fprintf(stdout, "-------------select-----------\n");
		fd_set readset, writeset, errset;
		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&errset);
		FD_SET(sock, &readset);
		ret = lwip_select(sock + 1, &readset, &writeset, &errset, &tv);
		fprintf(stdout, "----------select %d -----------\n", ret);

		if (ret > 0) {
			if (FD_ISSET(sock, &readset)) {
				ret = lwip_recv(sock, rxbuf, TEST_TXRX_BUFSIZE, 0);
				if (ret < 0 && errno != EAGAIN) {
					fprintf(stderr, "failed to recv\n");
					break;
				}
				else if (ret == 0) {
					fprintf(stdout, "connection closed by peer\n");
					break;
				}

				fprintf(stdout, "recv %d bytes, remain %d\n",
								ret, len - ret);
				len -= ret;
				if (len <= 0)
					break;
			}
		}
	}

	fprintf(stdout, "recv %s\n", rxbuf);
//  sys_msleep(100);
//
//  len = strlen(testdata);
//  ptr = rxbuf;
//  while (sys_now() < max_time) {
//    ret = lwip_recv(sock, ptr, TEST_TXRX_BUFSIZE, 0);
//	if (ret < 0 && ret != EAGAIN) {
//		fprintf(stderr, "failed to recv, err %d\n", errno);
//		break;
//	}
//	else if (ret == 0) {
//		break;
//	}
//	ptr += ret;
//	len -= ret;
//	if (len <= 0)
//		break;
//}
  lwip_close(sock);
}

#endif /* LWIP_SOCKET && LWIP_IPV4 */
