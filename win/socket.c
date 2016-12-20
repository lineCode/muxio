#include <assert.h>
#include <winsock2.h>
#include <windows.h>
#include <string.h>
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

int 
socket_nonblock(int fd)
{
	unsigned long enable = 1;
	ioctlsocket(fd, FIONBIO, (unsigned long *)&enable);
	return 0;
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
		closesocket(fd);
		return err;
	}
	listen(fd, 5);
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
	closesocket(fd);
	return -1;
}

void
socket_close(int fd)
{
	closesocket(fd);
	return ;
}

int
socket_read(int fd, char *data, int sz)
{
	return recv(fd, data, sz, 0);
}

int
socket_write(int fd, const char *data, size_t sz)
{
	return send(fd, data, sz, 0);
}

int 
socket_init()
{
	WSADATA wsa={0};
	WSAStartup(MAKEWORD(2,2),&wsa);
	return 0;
}


