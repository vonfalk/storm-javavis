void print(const char *s) {
	for (const char *i = s; *i; i++) {
		putchar(*i);
	}
	putchar('\n');
}

int main() {
	const char *s = "Hej";
	print(s);

	char a[] = "hi";
	print(a);

	return 0;
}
