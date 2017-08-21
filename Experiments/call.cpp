class Foo {
public:
	Foo() : x(1), y(2) {}

	int x;
	int y;
};

struct SmallType {
	int v;
	SmallType(int v) : v(v) {}
	bool operator ==(const SmallType &o) const { return v == o.v; }
	bool operator !=(const SmallType &o) const { return !(*this == o); }
};


void foo(Foo x) {
	Foo *z = &x;
}

SmallType retSmall(int x, int y) {
	return SmallType(10 + x + y);
}

int main() {
	Foo z;

	foo(z);
}

int fMain() {
	SmallType s(1);
	s = retSmall(1, 2);
}
