#include "wrap/thread.h"
#include "wrap/sync.h"

struct parameters_to_start {
	const char *start;
};

void process_start(struct parameters_to_start *test) {
	for (const char *at = test->start; *at; at++) {
		putchar(*at);
	}
	putchar('\n');
}

int start_process(const char *name) {
	struct parameters_to_start params;
	params.start = name;

	int tid = thread_new(process_start, &params);

	return tid;
}

int main() {
	int pid = start_process("test");
	// TODO: Make some kind of printf.
	print(pid);

	return 0;
}
