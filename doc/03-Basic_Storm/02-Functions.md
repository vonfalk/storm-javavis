Functions
==========

Functions are declared at top-level, following the syntax of C++:

`<return type> <name>(<parameters>) { <body> }`

For example:

`Int foo(Int a) {}`

To support threading, Basic Storm also allows you to specify which thread each function shall be
executed on (if any), by adding `on T` (where T is the name of a thread). If a thread is declared,
the function will always be executed on the named thread, otherwise it will be executed on the same
thread as the caller.

All functions in Basic Storm are lazily compiled. This means that the function bodies will not be
compiled until they are actually executed. In the future, there will be ways to compile all code
ahead of time, but not yet.


Decorators
-----------

Functions can be annotated to provide additional information to Storm. This is done using the
following syntax:

`<return type> <name>(<parameters>) : <decorators> { <body> }`

For example:

`Int foo(Int a) : pure {}`

Multiple decorators may be applied by providing a comma-separated list. The following decorators are
available for non-member functions:

* `pure`: Indicates that this function is pure, meaning that the output of the function only depends
  on the parameters, and that the function does not produce any visible side-effects. This may be
  used to execute the function during compilation in certain cases.

There are other decorators available for member functions. See [Types](md://Storm/Types) for more
information.
