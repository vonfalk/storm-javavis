class my_class {
public:
	my_class(int a, int b) : a(a), b(b) {}

	int a;
	int b;
};

int main() {
	my_class a{1, 2};
	my_class b{3, 4};

	my_class c{a};
	c = b;

	return 0;
}
