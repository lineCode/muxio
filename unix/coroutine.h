#ifndef _COROUTINE_H
#define _COTOUTINE_H

struct coroutine;

#define COROUTINE_SUSPEND       1
#define COROUTINE_RUNNING       2
#define COROUTINE_DEAD          3

typedef void (task_entry_t)(void *arg);

int coroutine_init();
void coroutine_exit();

struct task *coroutine_create(task_entry_t *cb, void *ud);
struct task *coroutine_self();
void coroutine_free(struct task *t);

void coroutine_yield();
void coroutine_resume(struct task *t);
int coroutine_status(struct task *t);

#endif


