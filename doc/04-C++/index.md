C++
====

Storm treats C++ (almost) like any other language. This part describes how C++ interacts with the
rest of Storm, even though C++ is not implemented in the Storm compiler.

Exporting functions
--------------------

The compiler itself is implemented in C++, and we therefore need a way to export C++ functions to
the type system. This is done by a preprocessor reading all header files and looking for specific
markers, namely `STORM_CLASS`, `STORM_VALUE`, `STORM_FN` and `STORM_CTOR` respectively. Classes
are declared like this:
```
class Foo : public Object {
    STORM_CLASS;
public:
    STORM_CTOR Foo();
    // ...
};
```
Values like this:
```
class Foo {
    STORM_VALUE;
public:
    // ...
};
```

Functions have their marker like this:
```
void STORM_FN foo();
```

Note that no functions or variables are exposed unless marked. Functions exposed to the compiler needs
to follow the calling convention in the compiler with respect to reference counting. The parameters
are borrowed pointers, and the return value is the response of the caller. To aid the programmer in
reference counting, there are two classes that automatize the reference counting: `Par` and `Auto`.
`Par` is to be used for function parameters, it represents a borrowed reference. `Auto` on the other
hand owns one reference, and keeps the object alive. Assigning between `Par` and `Auto` is automatic
and does the right thing. Initializing from a pointer is more dangerous, `Par` borrows the reference
while `Auto` assumes it is to take ownership of the previously free reference (ie it will call `release`
but not `addRef`). Note that `Par` nor `Auto` is used as the return value. This is since C++ returns
user defined types differently than for example pointer types. (may be fixed later).

In many aspects, the storm runtime handles objects and values like C++, so they can interact quite
well as long as all types involved are properly exposed to the compiler's type system.

Calling conventions
--------------------
The calling convention used in the compiler varies by platform, but it is generally `cdecl`, ie.
the standard calling convention used in C. This is also used for member functions (the standard
here is usually `thiscall`) to unify the calling conventions used. However, they still differ
when dealing with return values that are not stored in registers. In the case of a non-member
function, a pointer to empty memory suitable for the result is passed as the first parameter
to the function. For member functions, the `this` pointer is always first, and the result
pointer is inserted as the second parameter.

Threads
--------

To switch between user level threads in C++, call the `os::UThread::leave()` function, or use any
synchronization primitive from `os::` that blocks the thread. Currently there is no way of doing the
same thing from another language. All of this is going through C++.
