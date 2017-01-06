#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define	sleep(n)	Sleep(n)
#else
#include <unistd.h>
#define	sleep(n)	usleep(n * 1000)
#endif
#include "muxio.h"

void comin(int fd, void *ud)
{
	for (;;) {
		int sz;
		printf("new comin:%d\n", fd);
		fflush(stdout);
		int err = muxio_read(fd, (char *)&sz, 4);
		printf("read fd:%d err:%d data:%d\n", fd, err, sz);
		muxio_write(fd, (char *)&sz, 4);
		fflush(stdout);
	}
}

int main()
{
	muxio_init();
	muxio_listen("0.0.0.0", 9009, comin, NULL);
	for (;;) {
		muxio_dispatch();
		sleep(300);
	}
	printf("-----------------\n");
	muxio_exit();
	return 0;
}


