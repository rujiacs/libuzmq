#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <string>

#include <zmq.h>

#include "util.hpp"

#define LOCAL_IP "192.168.56.22"
#define LOCAL_MASK "255.255.255.0"
#define LOCAL_GW "192.168.56.1"

#define SERVER_IP "192.168.56.21"
#define SERVER_PORT "9123"

#define SERVER_ID	1
#define CLIENT_ID	8

#define TERM_CODE	"0123"

#define DATA_SIZE 200

using namespace std;

static void *ctx = NULL, *sock = NULL;

static char data[DATA_SIZE] = {0};

static int __init(void)
{
	int ret = 0;

	ret = zmq_global_init(LOCAL_IP, LOCAL_GW, LOCAL_MASK);
	if (ret < 0) {
		UZMQ_ERROR("Failed to init zmq, ret %d", ret);
		return ret;
	}

	ctx = zmq_ctx_new();
	if (ctx == NULL) {
		UZMQ_ERROR("Failed to create zmq context");
		return -1;
	}
	zmq_ctx_set(ctx, ZMQ_MAX_SOCKETS, 65535);
}

static int __start_client(void)
{
	string hostaddr;

	sock = zmq_socket(ctx, ZMQ_DEALER);
	if (sock == NULL) {
		UZMQ_ERROR("Failed to create zmq socket");
		return -1;
	}

	zmq_set_localid(sock, CLIENT_ID);
	zmq_set_remoteid(sock, SERVER_ID);

	hostaddr = "tcp://" + string(SERVER_IP) + ":" + string(SERVER_PORT);

	UZMQ_DEBUG("Connect to %s", hostaddr.c_str());

	if (zmq_connect(sock, hostaddr.c_str()) != 0) {
		UZMQ_ERROR("Failed to connect");
		return -1;
	}
	return 0;
}

static int recvdata(void)
{
	size_t recv_bytes = 0;
	zmq_msg_t *msg = new zmq_msg_t;
	int ret = 0;
	char *rx_data = NULL;

	if (zmq_msg_init(msg) != 0) {
		UZMQ_ERROR("Failed to init zmq msg");
		ret = -1;
		goto free_return;
	}

	while (true) {
		if (zmq_msg_recv(msg, sock, 0) != -1)
			break;
		if (errno == EINTR)
			continue;
		UZMQ_ERROR("Failed to recv data, err %d", errno);
		ret = -1;
		goto free_return;
	}

	rx_data = (char *)zmq_msg_data(msg);
	recv_bytes = zmq_msg_size(msg);

	if (!rx_data) {
		UZMQ_ERROR("Recv NULL data");
		ret = -1;
		goto free_return;
	}

	if (recv_bytes != DATA_SIZE && recv_bytes != sizeof(int)) {
		UZMQ_ERROR("Wrong msg size %lu", recv_bytes);
		ret = -1;
		goto free_return;
	}

	if (recv_bytes == sizeof(int)) {
		int *code = (int*)zmq_msg_data(msg);

		UZMQ_DEBUG("Recv code %d", *code);
		ret = *code;
	}
	else {
		ret = 0;
		memcpy(data, rx_data, DATA_SIZE);
	}

free_return:
	if (msg) {
		zmq_msg_close(msg);
		delete msg;
		msg = NULL;
	}
	return ret;
}

static int senddata(void)
{
	zmq_msg_t msg;

	zmq_msg_init_data(&msg, data, DATA_SIZE, NULL, NULL, SERVER_ID);

	while (true) {
		if (zmq_msg_send(&msg, sock, 0) == DATA_SIZE) break;
		if (errno == EINTR) continue;

		UZMQ_ERROR("Failed to send %u-byte data", DATA_SIZE);
		return -1;
	}
	zmq_msg_close(&msg);
	return 0;
}

static int sendterm(void)
{
	zmq_msg_t msg;

	snprintf(data, 5, "%s", TERM_CODE);

	UZMQ_DEBUG("Send TERM");
	zmq_msg_init_data(&msg, data, 4, NULL, NULL, SERVER_ID);

	while (true) {
		if (zmq_msg_send(&msg, sock, 0) == sizeof(int)) break;
		if (errno == EINTR) continue;

		UZMQ_ERROR("Failed to send term");
		return -1;
	}
	zmq_msg_close(&msg);
	return 0;
}

int main(int argc, char **argv)
{
	if (__init() < 0) {
		UZMQ_ERROR("Failed to init zmq");
		return 1;
	}

	if (__start_client() < 0)
		goto exit;

	for (int i = 0; i < 100; i++) {
		int ret = senddata();

		if (ret < 0)
			break;

		if (recvdata() < 0)
			break;
	}

	sendterm();

exit:

	if (sock) {
		int linger = 0;

		zmq_setsockopt(sock, ZMQ_LINGER, &linger, sizeof(linger));
		zmq_close(sock);
		sock = NULL;
	}
	zmq_ctx_destroy(ctx);
	return 0;
}
