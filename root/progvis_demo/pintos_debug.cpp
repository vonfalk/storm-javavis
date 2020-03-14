int main() {
	char str[] = "sihtgubed";
	char *stri = &str[8];
	char *buf[9];
	char **bufi;
	char **bufend;
	bufi = buf;
	bufend = &buf[9];
	while (bufi != bufend) {
		*bufi = stri;
		bufi++;
		stri--;
	}

	// Possible bug?
	do {
		bufi--;
		*(*bufi) -= 32;
	} while (bufi != buf);

	while (bufi != bufend){
		putchar(**bufi);
		bufi++;
	}

	putchar('\n');

	return 0;
}
