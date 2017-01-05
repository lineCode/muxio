#include <vector>
#include <unordered_map>
#include <alloca.h>
#include "coroutine.h"
#include "socket.h"
#include "buffer.h"
#include "muxio.h"

struct sockobj {
	int fd;
	buffer send;
	buffer recv;
	void *ud;
	muxio_listen_t cb;
};

struct listenobj {
	int fd;
	void *ud;
	muxio_listen_t cb;
};

static int handle = 0x01 << 16 | 0xffff;
//sockets
static std::unordered_map<int, struct listenobj> listens;	//fd, obj
static std::unordered_map<int, struct sockobj> sockets;		//fd, obj
//read event
static std::unordered_map<int, int> readsuspend; //fd, coroutine
static std::unordered_map<int, int> readsize; //fd, size
static std::unordered_map<int, int> readresult; //fd, size
//coroutines
static std::unordered_map<int, bool> waitco; //handle, true
static std::unordered_map<int, bool> wakeupco;	//handle true
static std::unordered_map<int, struct task *> handleptr;
static std::unordered_map<struct task *, int> ptrhandle;

static int
waitforread(int fd, char *data, int sz)
{
	int err;
	readsize[fd] = sz;
	readsuspend[fd] = muxio_self();
	muxio_wait();
	err = readresult[fd];
	readsize.erase(fd);
	readresult.erase(fd);
	readsuspend.erase(fd);
	if (err < 0)
		return err;
	memcpy(data, sockets[fd].recv.data, sz);
	sockets[fd].recv.out(sz);
	assert(sz == err);
	return sz;
}

static void
wakeupread(int fd, int err)
{
	readresult[fd] = err;
	muxio_wakeup(readsuspend[fd]);
}


//socket
int
muxio_listen(const char *ip, int port, muxio_listen_t cb, void *ud)
{
	int fd = socket_listen(ip, port);
	if (fd < 0)
		return fd;
	listens[fd].fd = fd;
	listens[fd].ud = ud;
	listens[fd].cb = cb;
	return fd;
}

static inline void
clearsocket(int fd)
{
	if (readsuspend.count(fd))
		wakeupread(fd, -1);
	socket_close(fd);
	sockets.erase(fd);
	return ;
}



static inline void
addsocket(int fd, muxio_listen_t cb, void *ud)
{
	sockets[fd].fd = fd;
	sockets[fd].cb = cb;
	sockets[fd].ud = ud;
	return ;
}

int
muxio_connect(const char *ip, int port, const char *bind, int bport)
{
	int fd;
	fd = socket_connect(ip, port, bind, bport);
	if (fd < 0)
		return fd;
	addsocket(fd, NULL, NULL);
	return fd;
}

int
muxio_read(int fd, char *data, size_t sz)
{
	int err;
	auto &buff = sockets[fd].recv;
	if (buff.has(sz)) {
		memcpy(data, buff.data, sz);
		buff.out(sz);
		return sz;
	}
	err = waitforread(fd, data, sz);
	return err;
}

int
muxio_write(int fd, const void *data, size_t sz)
{
	if (sockets.count(fd) == 0)
		return -1;
	sockets[fd].send.in(data, sz);
	return sz;
}

void
muxio_close(int fd)
{
	if (sockets.count(fd) == 0)
		return ;
	clearsocket(fd);
	return ;
}


//thread api
static int
newco(muxio_func_t func, void *ud)
{
	struct task *t = coroutine_create(func, ud);
	for (;;) {
		if (handleptr.count(handle) == 0)
			break;
		++handle;
	}
	handleptr[handle] = t;
	assert(ptrhandle.count(t) == 0);
	ptrhandle[t] = handle;
	return handle++;
}

int
muxio_fork(muxio_func_t func, void *ud)
{
	int h = newco(func, ud);
	wakeupco[h] = true;
	return h;
}

int
muxio_self()
{
	struct task *t = coroutine_self();
	int h = ptrhandle[t];
	assert(h > 0);
	return h;
}

void
muxio_wait()
{
	assert(waitco[muxio_self()] == false);
	waitco[muxio_self()] = true;
	coroutine_yield();
	return ;
}

void
muxio_wakeup(int handle)
{
	assert(handleptr.count(handle) == 1);
	assert(waitco[handle]);
	waitco.erase(handle);
	wakeupco[handle] = true;
	return ;
}

static void
wakeupone()
{
	for (;;) {
		int handle;
		auto n = wakeupco.begin();
		if (n == wakeupco.end())
			return ;
		handle = n->first;
		wakeupco.erase(n);
		assert(handleptr.count(handle) == 1);
		coroutine_resume(handleptr[handle]);
	}
	return ;
}

static std::vector<int>	socketscache;
static std::vector<int>	socketserror;

static void
accept_cb(void *ud)
{
	struct sockobj *obj = (struct sockobj *)ud;
	obj->cb(obj->fd, obj->ud);
}

static inline void
tryaccept()
{
	int maxfd = 0;
	fd_set lset;
	struct timeval tv = {};
	//Listen IO
	FD_ZERO(&lset);
	//TODO:remove this limit
	assert(listens.size() < 64);
	for (auto &fd:listens) {
		FD_SET(fd.first, &lset);
		maxfd = maxfd > fd.first ? maxfd : fd.first;
	}
	select(maxfd + 1, &lset, NULL, NULL, &tv);
	for (auto &iter:listens) {
		int fd = iter.first;
		if (FD_ISSET(fd, &lset)) {
			int s = accept(fd, NULL, NULL);
			if (s < 0)
				continue;
			socket_nonblock(s);
			addsocket(s, iter.second.cb, iter.second.ud);
			muxio_fork(accept_cb, &sockets[s]);
		}
	}
}

static int
processone(int fd, fd_set *rset, fd_set *wset)
{
	if (FD_ISSET(fd, rset)) {
		auto &recv = sockets[fd].recv;
		int err = recv.read(fd);
		if (err < 0)
			return -1;
		if (readsuspend.count(fd)) {//suspend
			int sz = readsize[fd];
			assert(sz);
			if (recv.has(sz))
				wakeupread(fd, sz);
		}
	}
	if (FD_ISSET(fd, wset))
		sockets[fd].send.write(fd);
	return 0;
}

static inline void
IO()
{
	int i;
	struct timeval tv = {};
	fd_set rset;
	fd_set wset;
	int mfd;
	//TODO: support more than 64 sockets
	socketscache.clear();
	socketserror.clear();
	tryaccept();
	socketscache.resize(sockets.size());
	//SOCKET IO
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	i = 0;
	for (const auto &iter:sockets) {
		int fd = iter.first;
		socketscache[i] = fd;
		FD_SET(fd, &rset);
		FD_SET(fd, &wset);
		if (fd > mfd)
			mfd = fd;
		++i;
	}
	select(mfd + 1, &rset, &wset, NULL, &tv);
	for (auto fd:socketscache) {
		int err = processone(fd, &rset, &wset);
		if (err < 0)
			socketserror.push_back(fd);
	}
	//process error
	for (auto fd:socketserror)
		clearsocket(fd);
}

void
muxio_dispatch()
{
	IO();
	//process coroutine
	wakeupone();
}

int
muxio_init()
{
	coroutine_init();
	socket_init();
	return 0;
}

//frame work
void
muxio_exit()
{
	for (auto &iter:sockets)
		socket_close(iter.first);
	for (auto &iter:listens)
		socket_close(iter.first);
	return ;
}


