class Foo {
public:
	Foo() : x(1), y(2) {}

	int x;
	int y;
};

void foo(Foo x) {
	Foo *z = &x;
}

int main() {
	Foo z;

	foo(z);
}
