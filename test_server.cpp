#include <Windows.h>
#include <stdio.h>
#include "muxio.h"

void comin(int fd, void *ud)
{
	int sz;
	printf("new comin:%d\n", fd);
	fflush(stdout);
	int err = muxio_read(fd, (char *)&sz, sizeof(int));
	printf("read fd:%d err:%d data:%d\n", fd, err, sz);
	fflush(stdout);
}

int main()
{
	muxio_init();
	
	muxio_listen("0.0.0.0", 9009, comin, NULL);

	for (;;) {
		muxio_dispatch();
		Sleep(3);
	}
	muxio_exit();
}


