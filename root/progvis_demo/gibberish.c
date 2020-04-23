#include "wrap/thread.h"
#include "wrap/synch.h"
#include <stdlib.h>
#include <stdio.h>

struct info {
	int tid;
	const char *message;
};

struct lock *locks;
struct semaphore *done;

// static struct info *make_info(int id, const char *message) {
static struct info *make_info(int id, const char *message) {
	struct info *i = malloc(sizeof(struct info));
	i->tid = id;
	i->message = message;
	return i;
}

static void putline(const char *message) {
	/* lock_acquire(&locks[0]); */

	while (*message) {
		putchar(*message++);
	}
	putchar('\n');

	/* lock_release(&locks[0]); */
}

static void printer(struct info *info) {
	for (int i = 0; i < 100; i++) {
		/* lock_acquire(&locks[info->tid]); */
		/* lock_acquire(&locks[0]); */
		putline(info->message);
		/* lock_release(&locks[0]); */
		/* lock_release(&locks[info->tid]); */
	}

	sema_up(&done[info->tid]);
	free(info);
}


int main(void) {
	int N = 4;

	locks = malloc(sizeof(struct lock)*N);
	done = malloc(sizeof(struct semaphore)*N);

	for (int i = 0; i < N; i++) {
		sema_init(&done[i], 0);
		lock_init(&locks[i]);
	}

	thread_new(&printer, make_info(0, "This line is not written in gibberish"));
	thread_new(&printer, make_info(1, "We want every line to be perfectly readable"));
	thread_new(&printer, make_info(2, "The quick brown fox jumps over the lazy dog"));
	thread_new(&printer, make_info(3, "Lorem ipsum dolor sit amet"));

	for (int i = 0; i < N; i++) {
		sema_down(&done[i]);
	}

	for (int i = 0; i < N; i++) {
		lock_destroy(&locks[i]);
		sema_destroy(&done[i]);
	}

	free(locks);
	free(done);

	return 0;
}
