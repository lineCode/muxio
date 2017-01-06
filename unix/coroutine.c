#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ucontext.h>

#include "coroutine.h"

#define my_malloc(a)			malloc(a)
#define my_free(a)			free(a)
#define my_realloc(ptr, size)		realloc(ptr, size)

#define STACK_SIZE			(1024 * 1024)
#define	STACK_INIT			(512)

struct task {
	int status;
	int stacksz;
	int stackcap;
	void *ud;
	void (*cb) (void *ud);
	unsigned char *stack;
	struct task *parent;
	struct ucontext ctx;
};

struct taskmgr {
	void *stack;
	size_t stackcap;
	struct task *main;
	struct task *ctx;
};

static pthread_key_t T;

static inline void
setT(struct taskmgr *t)
{
	pthread_setspecific(T, t);
}

static inline struct taskmgr *
getT()
{
	return (struct taskmgr *)pthread_getspecific(T);
}

static inline void
setctx(struct task *t)
{
	struct taskmgr *T = getT();
	T->ctx = t;
	return ;
}

static inline struct task *
getctx()
{
	struct taskmgr *T = getT();
	return T->ctx;
}

static void
entry(void *ud)
{
	struct task *arg = (struct task *)ud;
	arg->status = COROUTINE_RUNNING;
	arg->cb(arg->ud);
	arg->status = COROUTINE_DEAD;
	swapcontext(&arg->ctx, &arg->parent->ctx);
	return ;
}

static struct task *
createtask(void (*cb)(void *), void *ud, size_t stackcap)
{
	struct task *t = (struct task *)my_malloc(sizeof(*t));
	t->status = COROUTINE_SUSPEND;
	t->stackcap = stackcap;
	t->stacksz = stackcap;
	t->ud = ud;
	t->cb = cb;
	t->stack = my_malloc(stackcap);
	t->parent = NULL;
	getcontext(&t->ctx);
	return t;
}

static void
freetask(struct task *t)
{
	if (t->stack)
		my_free(t->stack);
	my_free(t);
	return ;
}

struct task *
coroutine_create(task_entry_t *cb, void *ud)
{
	struct taskmgr *T = getT();
	struct task *t = createtask(cb, ud, STACK_INIT);
	void *stack = T->stack;
	t->ctx.uc_stack.ss_sp = stack;
	t->ctx.uc_stack.ss_size = T->stackcap;
	makecontext(&t->ctx, (void (*)())entry, 1, t);
	memcpy(t->stack, stack, t->stackcap);
	t->stacksz = t->stackcap;
	return t;
}

void
coroutine_free(struct task *t)
{
	freetask(t);
	return ;
}

struct task *
coroutine_self()
{
	return getctx();
}

static inline void
savestack(struct task *t, void *top)
{
	size_t basecap = t->ctx.uc_stack.ss_size;
	void *base = t->ctx.uc_stack.ss_sp + basecap;
	int sz = base - top;
	assert(sz > 0);
	if (sz > t->stackcap) {
		t->stack = my_realloc(t->stack, sz);
		t->stackcap = sz;
	}
	t->stacksz = sz;
	memcpy(t->stack, top, sz);
	return ;
}

static inline void
restorestack(struct task *t)
{
	size_t basecap = t->ctx.uc_stack.ss_size;
	void *base = t->ctx.uc_stack.ss_sp + basecap;
	assert(basecap > t->stacksz);
	memcpy(base - t->stacksz, t->stack, t->stacksz);
	return ;
}

void
coroutine_yield()
{
	struct task *self = getctx();
	int dummy;
	assert(self->parent); //main thread should not be yield
	assert(self->status == COROUTINE_RUNNING);
	self->status = COROUTINE_SUSPEND;
	savestack(self, &dummy);
	swapcontext(&self->ctx, &self->parent->ctx);
	return ;
}

void
coroutine_resume(struct task *t)
{
	struct task *self = getctx();
	assert(t->status == COROUTINE_SUSPEND);
	setctx(t);
	t->parent = self;
	t->status = COROUTINE_RUNNING;
	restorestack(t);
	swapcontext(&self->ctx, &t->ctx);
	setctx(self);
	return ;
}

int
coroutine_status(struct task *t)
{
	return t->status;
}

int
coroutine_init()
{
	int dummy;
	struct taskmgr *mgr = my_malloc(sizeof(*mgr));
	(void) dummy;
	//ensure stack direction
	assert((uintptr_t)&dummy > (uintptr_t)&mgr);
	pthread_key_create(&T, NULL);
	mgr->main = createtask(NULL, NULL, 0);
	mgr->stackcap = STACK_SIZE;
	mgr->stack = my_malloc(STACK_SIZE);
	mgr->ctx = mgr->main;
	setT(mgr);
	return 0;
}

void
coroutine_exit()
{
	struct taskmgr *T = getT();
	freetask(T->main);
	if (T->stack)
		my_free(T->stack);
	return ;
}

