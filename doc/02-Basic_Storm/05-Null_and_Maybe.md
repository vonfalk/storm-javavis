Null and maybe
===============

By default, reference variables (class or actor types) can not contain the value null. Sometimes,
however, it is useful to be able to return a special value for special condition. For this reason,
Storm contains the special type `Maybe<T>`. This type is a reference, just like regular pointers,
but it may contain a special value that indicates that there is no object (null). In Basic Storm,
the shorthand `T?` can be used instead of `Maybe<T>`.

`Maybe<T>` can not be used to access the underlying object without explicitly checking for
null. Null checking is done like this:

```
Str? obj;
if (obj) {
    // In here, obj is a regular Str object.
}
```

At the moment, no automatic conversions are implemented in Basic Storm. Therefore, no automatic
conversion from `T` to `T?` is done, even though it is always safe to do so. Until this is done,
Basic Storm provides the syntax `?x` to explicitly cast to a `T?`. This is useful when calling
functions, for example:

```
Int foo(Str a) { 1; }
Int foo(Str? a) { 2; }

void main() {
    foo("a"); // => 1
    foo(?"a"); // => 2
}

```

The type `T?` overloads constructors and assignment operators so that assignments and creations of
the `T?` from `T` can be done without any explicit casting.
