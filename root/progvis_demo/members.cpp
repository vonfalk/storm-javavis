class inner {
public:
	int a;
	int b;

	int sum() {
		return a + b;
	}
};


class outer {
public:
	int a;
	inner b;

	int sum() {
		return a + b.sum();
	}
};

int main() {
	// inner i;
	// print(i.sum());

	outer o;
	print(o.sum());
	print(o.b.sum());

	return 0;
}
