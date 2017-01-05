#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ucontext.h>

#include "coroutine.h"

#define my_malloc(a)			malloc(a)
#define my_free(a)			free(a)
#define my_realloc(ptr, size)		realloc(ptr, size)

#define STACK_SIZE			(1024 * 1024)
#define	STACK_INIT			(64)

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

static pthread_key_t T;
static pthread_key_t CTX;

static inline void
setctx(struct task *t)
{
	pthread_setspecific(CTX, t);
	return ;
}

static inline struct task *
getctx()
{
	return (struct task *)pthread_getspecific(CTX);
}

static inline void
setT(struct task *t)
{
	pthread_setspecific(T, t);
}

static inline struct task *
getT()
{
	return (struct task *)pthread_getspecific(T);
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
	struct task *t = createtask(cb, ud, STACK_INIT);
	struct task *T = getT();
	t->ctx.uc_stack.ss_sp = T->stack;
	t->ctx.uc_stack.ss_size = T->stackcap;
	makecontext(&t->ctx, (void (*)())entry, 1, t);
	memcpy(t->stack, T->stack, t->stackcap);
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
	void *base = t->ctx.uc_stack.ss_sp;
	int sz = top - base;
	assert(sz > 0);
	if (sz > t->stackcap) {
		t->stack = my_realloc(t->stack, sz);
		t->stackcap = sz;
	}
	t->stacksz = sz;
	printf("save stack:%p-%d\n", base, sz);
	memcpy(t->stack, base, sz);
	return ;
}

static inline void
restorestack(struct task *t)
{
	struct task *T = getT();
	void *base = T->stack;
	assert(T->stackcap > t->stacksz);
	memcpy(base, t->stack, t->stacksz);
	t->ctx.uc_stack.ss_sp = base;
	t->ctx.uc_stack.ss_size = T->stackcap;
	fflush(stdout);
	return ;
}

void
coroutine_yield()
{
	struct task *self = getctx();
	int dummp;
	assert(self->parent); //main thread should not be yield
	assert(self->status == COROUTINE_RUNNING);
	self->status = COROUTINE_SUSPEND;
	savestack(self, &dummp);
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
	//printf("resume %p-%p\n", self, t);
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
	struct task *t = createtask(NULL, NULL, STACK_SIZE);
	printf("T %p\n", t);
	pthread_key_create(&T, NULL);
	pthread_key_create(&CTX, NULL);
	setT(t);
	setctx(t);
	return 0;
}

void
coroutine_exit()
{
	struct task *T = getT();
	freetask(T);
}

