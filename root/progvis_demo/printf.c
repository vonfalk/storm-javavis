int fn(int x) {
	return x;
}

int main() {
	int v = 10;
	const char *s = "World";
	printf("Hello %10d!\n", fn(v));
	printf("Hello %10s!\n", s);

	return 0;
}
