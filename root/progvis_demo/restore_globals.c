int global_int;
struct lock l;

int main() {
	global_int = 30;
	lock_init(&l);

	return 0;
}
