#ifndef _MUXIO_H
#define _MUXIO_H
 
#include <stdint.h>

typedef void (* muxio_listen_t)(int fd, void *ud);
typedef void (* muxio_func_t)(void *ud);

//frame work
int muxio_init();
void muxio_exit();
void muxio_dispatch();

//socket api
int muxio_listen(const char *ip, int port, muxio_listen_t cb, void*ud);
int muxio_connect(const char *ip, int port, const char *bind, int bport);
int muxio_read(int fd, char *data, size_t sz);
int muxio_write(int fd, const void *data, size_t sz);
void muxio_close(int fd);

//thread api
int muxio_fork(muxio_func_t cb, void *ud);
int muxio_self();
void muxio_wait();
void muxio_wakeup(int handle);

#endif

