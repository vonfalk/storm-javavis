package java;

import foo.bar;
import foo.bar.*;

class Simple {
	int foo[] = {}, bar = {};

	int bar(int z, ... foo) {}

	@Override
	Simple() {
	}

	int baz() {
		int foo, bar;
		final float y = 1.0f;
		final String x = "hej";
		assert (foo + bar);
		super.foo(foo, bar, this);
		int.class;
		Simple.super.this;
		if (foo == bar)
			foo = 3;
		else {
			foo = 4;
		}
		do {
			foo = 3;
		} while (3);

		try (Foo z = bar) {
		} catch (Foo | Bar x) {
		} finally {
		}

		for (int x : y)
			baz;
		for (int x = 0; x < 0; x++)
			baz;

		new Foo<Integer>.Bar<Integer>();
		super.new Foo();
	}
}
