#ifndef _SOCKET_H
#define	_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int socket_init();
int socket_listen(const char *ip, int port);
int socket_connect(const char *ip, int port, const char *bip, int bport);
void socket_close(int fd);
void socket_nonblock(int fd);
int socket_read(int fd, char *data, int sz);
int socket_write(int fd, const char *data, size_t sz);

#endif

