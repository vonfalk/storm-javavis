class inner {
public:
	int a;
	int b;

	inner() : a(1), b(2) {
		a++;
	}

	~inner() {
		b = 30;
	}

	int sum() {
		return a + b;
	}
};


class outer {
public:
	int a;
	inner b;

	outer() : a(1) {
		a++;
	}

	~outer() {
		a = 20;
	}

	int sum() {
		return a + b.sum();
	}
};

int main() {
	outer o;
	print(o.sum());
	print(o.b.sum());

	outer *a = new outer;
	a->sum();
	delete a;

	return 0;
}
