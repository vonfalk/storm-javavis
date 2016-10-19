// Compile with 'cl /d1reportAllClassLayout alignment.cpp'
// to see struct layout. Also see /d1reportSingleClassLayoutXxx
// where Xxx is matched agains the class name

typedef long long int ll;

struct A {
	int a;
	int b;
};

struct B {
	ll a;
	ll b;
};

struct C {
	bool z;
	A a;
};

struct D {
	bool z;
	B b;
};

struct E {
	bool z;
	ll a;
};
