struct data {
	int a;
	int b;
	int c;
};

int main() {
	struct data *p;
	struct data d;
	d.a = 5;

	p = &d;
	(*p).b = 6;
	p->c = 7;

	int *q = &d.a;
	*q = 6;

	q = &d.b;
	*q = 7;

	return 0;
}
