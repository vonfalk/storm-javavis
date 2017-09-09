#include <cstdio>
#include <cstring>

class Base {
public:
	size_t data;

	virtual ~Base() {}

	void regular() {
		printf("Regular Base\n");
	}

	virtual void dynamic() {
		printf("Dynamic Base\n");
	}

	virtual void dynamic2() {
		printf("Dynamic2 Base\n");
	}
};

class Derived : public Base {
public:
	void regular() {
		printf("Regular Derived\n");
	}

	virtual void dynamic() {
		printf("Dynamic Derived\n");
	}

	virtual void dynamic2() {
		printf("Dynamic2 Derived\n");
	}
};

typedef void (Base::*Fn)();

void doRegularCall(Base *m, Fn fn) {
	(m->*fn)();
}

void doPlainCall(Base *m, Fn fn) {
	typedef void (*Plain)(Base *);
	Plain plain;
	memcpy(&plain, &fn, sizeof(Plain));
	printf("Plain function: %p\n", plain);
	(*plain)(m);
}

void doCall(Base *m, Fn fn) {
	printf("Regular: ");
	doRegularCall(m, fn);
	// printf("Plain: ");
	// doPlainCall(m, fn);
}

void doCall(Base *m, void (Derived::*fn)()) {
	Fn f;
	memcpy(&f, &fn, sizeof(Fn));
	doCall(m, f);
}

int main() {
	printf("Size of fn pointer: %d\n", int(sizeof(Fn)));

	Base b;
	printf("-- base --\n");
	doCall(&b, &Base::regular);
	doCall(&b, &Base::dynamic);
	doCall(&b, &Base::dynamic2);

	Derived d;
	printf("-- derived --\n");
	doCall(&d, &Base::regular);
	doCall(&d, &Base::dynamic);

	printf("-- crazy -- \n");
	doCall(&d, &Derived::regular);
	doCall(&d, &Derived::dynamic);

	return 0;
}
