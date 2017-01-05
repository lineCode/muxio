#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include "socket.h"

static void
tosockaddr(struct sockaddr *addr, const char *ip, int port)
{
	struct sockaddr_in *in = (struct sockaddr_in *)addr;
	memset(addr, 0, sizeof(*addr));
	in->sin_family = AF_INET;
	in->sin_port = htons(port);
	in->sin_addr.s_addr = inet_addr(ip);
	return ;
}

void
socket_nonblock(int fd)
{
	int err;
	int flag;
	flag = fcntl(fd, F_GETFL, 0);
	if (flag < 0) {
		perror("nonblock F_GETFL");
		return ;
	}
	flag |= O_NONBLOCK;
	err = fcntl(fd, F_SETFL, flag);
	if (err < 0) {
		perror("nonblock F_SETFL");
		return ;
	}
	return ;
}

int
socket_listen(const char *ip, int port)
{
	int fd;
	int err;
	int enable = 1;
	struct sockaddr addr;
	tosockaddr(&addr, ip, port);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(fd > 0);
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable, sizeof(int));
	err = bind(fd, &addr, sizeof(addr));
	if (err < 0) {
		close(fd);
		return err;
	}
	listen(fd, 256);
	return fd;
}

int
socket_connect(const char *ip, int port, const char *bip, int bport)
{
	int fd;
	int err;
	struct sockaddr addr;
	struct sockaddr baddr;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(fd > 0);
	tosockaddr(&addr, ip, port);
	if (bip) {
		tosockaddr(&baddr, bip, bport);
		err = bind(fd, &baddr, sizeof(baddr));
		if (err < 0)
			goto fail;
	}
	err = connect(fd,  &addr, sizeof(addr));
	if (err < 0)
		goto fail;
	socket_nonblock(fd);
	return fd;
fail:
	close(fd);
	return -1;
}

void
socket_close(int fd)
{
	close(fd);
	return ;
}

int
socket_read(int fd, char *data, int sz)
{
	return read(fd, data, sz);
}

int
socket_write(int fd, const char *data, size_t sz)
{
	return write(fd, data, sz);
}

int
socket_init()
{
	return 0;
}


