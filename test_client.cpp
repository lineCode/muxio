#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define	sleep(n)	Sleep(n)
#else
#include <unistd.h>
#define	sleep(n)	usleep(n * 1000)
#endif

#include "muxio.h"

int cli[2] = {-1, -1};

static void
client(void *ud)
{
	int err;
	int recv;
	int n = (intptr_t)ud;
	int fd = muxio_connect("127.0.0.1", 9009, NULL, 0);
	int data = fd + 0xff00;
	printf("connect:%d\n", fd);
	printf("send fd:%d %d\n", fd, muxio_write(fd, (char *)&data, 1));
	int o = n ^ 1;
	if (cli[o] != -1)
		muxio_wakeup(cli[o]);
	cli[n] = muxio_self();
	muxio_wait();
	cli[n] = -1;
	if (cli[o] != -1)
		muxio_wakeup(cli[o]);
	muxio_write(fd, (char *)&data + 1, 3);
	err = muxio_read(fd, (char *)&recv, 4);
	printf("recv fd:%d err:%d data:%x\n", fd, err, recv);
	return ;
}

int main()
{
	muxio_init();
	muxio_fork(client, (void *)0);
	muxio_fork(client, (void *)1);
	for (;;) {
		muxio_dispatch();
		sleep(100);
	}
	muxio_exit();
	return 0;
}


