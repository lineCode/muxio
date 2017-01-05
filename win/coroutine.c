#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>

#include "coroutine.h"

#define my_malloc(a)                    malloc(a)
#define my_free(a)                      free(a)
#define my_realloc(ptr, size)           realloc(ptr, size)

#define STACK_SIZE                      (1024 * 1024)

struct task {
        int status;
        void *arg;
	void (*func)(void *);
        LPVOID fiber;
	LPVOID parent;
};

struct task *C;

static struct task *
createtask(void (*cb)(void *), void *ud)
{
	struct task *t = (struct task *)my_malloc(sizeof(*t));
	t->status = COROUTINE_SUSPEND;
	t->arg = ud;
	t->func = cb;
	t->parent = NULL;
	return t;
}

static void WINAPI
entry(void *ud)
{
	assert(ud);
	struct task *arg = (struct task *)ud;
	arg->status = COROUTINE_RUNNING;
	arg->func(arg->arg);
	arg->status = COROUTINE_DEAD;
	assert(arg->parent);
	SwitchToFiber(arg->parent);
	return ;
}


struct task *
coroutine_create(task_entry_t *func,  void *arg)
{
	struct task *t = createtask(func, arg);
	t->fiber = CreateFiber(STACK_SIZE,  entry, t);
	return t;
}

struct task *
coroutine_self()
{
	return (struct task *)GetFiberData();
}

void
coroutine_free(struct task *t)
{
	if (t->fiber)
		DeleteFiber(t->fiber);
	my_free(t);
	return ;
}


void
coroutine_yield()
{
	struct task *self = (struct task *)GetFiberData();
	assert(self->status == COROUTINE_RUNNING);
	self->status = COROUTINE_SUSPEND;
	assert(self->parent);
	SwitchToFiber(self->parent);
	return ;
}

void
coroutine_resume(struct task *t)
{
	struct task *self = (struct task *)GetFiberData();
	assert(t->status == COROUTINE_SUSPEND);
	t->status = COROUTINE_RUNNING;
	t->parent = self->fiber;
	assert(t->parent);
	SwitchToFiber(t->fiber);
	if (t->status == COROUTINE_DEAD) {
		DeleteFiber(t->fiber);
		my_free(t);
	}
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
	C = createtask(NULL, NULL);
	C->fiber = ConvertThreadToFiber(C);
	assert(C->fiber);
	return 0;
}

void
coroutine_exit()
{
	coroutine_free(C);
	return ;
}

