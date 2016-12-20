#include <errno.h>
#include <stdlib.h>
#include "buffer.h"

#if EAGAIN == EWOULDBLOCK
#define ETRYAGAIN EAGAIN
#else
#define ETRYAGAIN EAGAIN: case EWOULDBLOCK
#endif


buffer::buffer()
{
	data = NULL;
	datacap = 0;
	datasz = 0;
}

buffer::~buffer()
{
	if (data)
		free(data);
}

int
buffer::tryread(int s, void *buff, size_t sz)
{
	int err;
	assert(sz > 0);
	for (;;) {
		err = socket_read(s, (char *)buff, sz);
		if (err >= 0)
			return err;
		switch (errno) {
		case ETRYAGAIN:
			continue;
		default:
			//perror("read");
			return -1;
		}
	}
	return -1;
}


