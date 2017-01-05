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
	printf("new comin:%d\n", fd);
}

int main()
{
	int i = 0;
	muxio_init();
	int d1 = 123456;
	int d2 = 654321;
	int fd1 = muxio_connect("127.0.0.1", 9009, NULL, 0);
	int fd2 = muxio_connect("127.0.0.1", 9009, NULL, 0);
	printf("connect:%d-%d\n", fd1, fd2);
	printf("send:%d\n", muxio_write(fd1, (char *)&d1, 1));
	printf("send:%d\n", muxio_write(fd2, (char *)&d2, 1));
	for (;;) {
		if (i == 0) {
			muxio_write(fd2, (char *)&d2 + 1, 3);
			i = 1;
		} else {
			i = 2;
			muxio_write(fd1, (char *)&d1 + 1, 3);
		}

		muxio_dispatch();
		sleep(100);
	}
	muxio_exit();
	return 0;
}


