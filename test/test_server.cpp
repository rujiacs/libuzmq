#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <string>

#include <zmq.h>

#include "util.hpp"

#define LOCAL_IP "192.168.57.21"
#define LOCAL_MASK "255.255.255.0"
#define LOCAL_GW "192.168.57.1"

#define SERVER_IP "192.168.57.21"
#define SERVER_PORT "9123"

#define SERVER_ID	8
#define CLIENT_ID	9

#define DATA_SIZE 200

#define TERM_CODE "0123"

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

static int __start_server(void)
{
	string hostaddr;

	sock = zmq_socket(ctx, ZMQ_DEALER);
	if (sock == NULL) {
		UZMQ_ERROR("Failed to create zmq socket");
		return -1;
	}

	zmq_set_localid(sock, SERVER_ID);
	zmq_set_remoteid(sock, CLIENT_ID);

	hostaddr = "tcp://" + string(SERVER_IP) + ":" + string(SERVER_PORT);

	UZMQ_DEBUG("Bind to %s", hostaddr.c_str());

	if (zmq_bind(sock, hostaddr.c_str()) != 0) {
		UZMQ_ERROR("Failed to bind");
		zmq_close(sock);
		sock = NULL;
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

	if (recv_bytes != DATA_SIZE && recv_bytes != strlen(TERM_CODE)) {
		UZMQ_ERROR("Wrong msg size %lu", recv_bytes);
		ret = -1;
		goto free_return;
	}

	if (recv_bytes == strlen(TERM_CODE)) {
		char code[5] = {0};

		strncpy(code, rx_data, strlen(TERM_CODE));
		UZMQ_DEBUG("Recv code %s", code);

		if (strcmp(code, TERM_CODE) == 0)
			ret = 1;
		else
			ret = -1;
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

	zmq_msg_init_data(&msg, data, DATA_SIZE, NULL, NULL, CLIENT_ID);

	while (true) {
		if (zmq_msg_send(&msg, sock, ZMQ_DATA) == DATA_SIZE) break;
		if (errno == EINTR) continue;

		UZMQ_ERROR("Failed to send %u-byte data", DATA_SIZE);
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

	if (__start_server() < 0)
		return 1;

	while (true) {
		int ret = recvdata();

		if (ret < 0)
			break;

		if (ret == 0) {
			if (senddata() < 0)
				break;
		}

		if (ret == 1) {
			UZMQ_DEBUG("recv terminate cmd");
			break;
		}
	}

	if (sock) {
		int linger = 0;

		zmq_setsockopt(sock, ZMQ_LINGER, &linger, sizeof(linger));
		zmq_close(sock);
		sock = NULL;
	}
	zmq_ctx_destroy(ctx);
	return 0;
}
