#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <zmq.h>

#include "util.hpp"

#define LOCAL_IP "192.168.56.21"
#define LOCAL_MASK "255.255.255.0"
#define LOCAL_GW "192.168.56.1"

#define SERVER_IP "192.168.56.21"
#define SERVER_PORT "9123"


static void *ctx = NULL;

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


int main(int argc, char **argv)
{
	if (__init() < 0) {
		UZMQ_ERROR("Failed to init zmq");
		return 1;
	}

}
