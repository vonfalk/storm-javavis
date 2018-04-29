Types
======

Type declaration also follows closely to the C++ declarations. However, since Storm has three
distinct kind of types, the syntax has been extended a bit.

* __Value:__

  `value Foo { <contents> }`

  or

  `value Foo extends T { <contents> }`

* __Class:__

  `class Foo { <contents> }`

  or

  `class Foo extends T { <contents> }`

* __Actor:__

  `class Foo on T { <contents> }` (T is the name of a thread)

  or

  `class Foo on ? { <contents> }`

In these cases, contents is a list of member functions and member variables. Member functions are
declared just like regular functions, except that they may not include a thread directive, instead
they inherit the thread directive from the enclosing type. Member variables are declared just like
local variables in a function (i.e. `<type> <name>`). Initializing variables at the declaration site
is not supported yet. There are also no visibility declarations yet, everything is public at the
moment.

Note that it is not possible to inherit from a class _and_ declare the class as an actor. If the
super class was an actor the new class will be an actor as well, otherwise it will not.

Storm allows nested type definitions. These work like in C++, which means that a type declared
inside another type will be able to access all private parts of the outer type. Aside from that,
Storm does not consider them to have any special relation to each other.

Constructors
--------------

Types may have constructors. If no destructor is declared, Basic Storm generates a constructor for
you. Constructors are declared by the special syntax:

`init(<parameters>) { <body> }`

or, if the constructor represents a type cast that can be performed implicitly:

`cast(<parameters>) { <body> }`

The constructor is special, because it needs to contain the special `init` statement at top-level in
its body. This `init` statement is in charge of calling constructors of the super class (if any) and
the constructors of any member variables. It is another interpretation of the initializer list in
C++, but more powerful, since it allows execution of arbitrary code before any initialization is
actually done. Note that `this` and thereby any member functions or variables are not accessible
before the `init` block. The `init` statement looks like this:

`init(<params>) { <init-list> }`

Where `<params>` are the parameters to be passed to the super class constructor, and `<init-list>`
is a list of what to initialize all member variables to. If a member variable is not present in
`<init-list>`, it is constructed using the empty constructor (we do not initialize things to null,
use Maybe<T>, or T? for that). `<init-list>` is a list of assignments (`<name> = <expr>`) or
constructor calls (`<name>(<params>)`) separated by a semicolon (`;`).

The constructor acts a little special when working with actors that have not been declared to be
executed on a specific thread (using the `on ?` syntax). These constructors need to take a `Thread`
as their first parameter (you do not have to give it a name), and the first parameter is always
automatically passed to the parent constructor. This is needed to make it possible for Storm to
ensure that even the constructor of the actor is running on the given thread to the not-yet created
actor.


Actors
--------

As mentioned in [the Storm documentation](md://Storm/Type_system), actors behave slightly
differently compared to classes. Each actor is associated with a thread, either statically or
dynamically upon creation. Storm will then make sure that all code in the actor is executed by the
appropriate thread. Furthermore, Storm ensures that no data is shared between different threads.

In order to ensure these properties hold, Storm will examine each function call and perform a
*thread switch* if necessary. A thread switch involves sending a message to another thread (using
`core:Future<T>` to wait for the result) and making deep copies of all parameters and return values
to avoid shared data. This means that one needs to be a bit careful when working with actors, since
a class that is passed through a function declared to be executed on another thread will be copied
along the way, thereby breaking the by-reference semantics that one might expect. This is the main
reason why the `==` operator for classes compares values rather than references.

This mechanism also introduces difficulties when dealing with member variables inside actors. In
order to avoid data races and shared data, accesses to these kind of variables need to be
synchronized, and data needs to be copied just like function calls. Basic Storm implements this
using *thread switches*, just like for function calls. A small `read` function is used to perform
the variable read on the appropriate thread with copying as appropriate. However, this solution
means that accessing variables on other threads will not behave as regular accesses. In particular,
due to the copying it is not possible to modify the variable in this manner. Basic Storm will detect
this kind of situation if the variable is immediately assigned to (eg. `foo.x = 8`), but will not
detect in more complex cases such as `foo.x++` or `foo.x.z = 10` (where `foo.x` is the access that
requires a thread switch).


Function declarations in types
-------------------------------

Declaring functions inside types in Basic Storm is mostly the same to declaring functions at the
top-level. However, functions declared inside a type will become member functions, which means that
they receive a hidden first parameter describing the instance being operated on. In Basic Storm,
this is mostly equivalent to a global function with the first parameter named `this`. The name
`this` is special in Basic Storm, since the variable named `this` will automatically be inserted as
an implicit first parameter to functions of the form `foo(bar, baz)` (as opposed to `bar.foo(baz)`),
just as one would expect for member functions.

Aside from that, member functions can be declared as assignment functions by replacing the return
type with the keyword `assign`.

Aside from assignment functions, Basic Storm provides the keywords `final`, `abstract` and
`override` that tell Storm your intentions with regards to inheritance. These keywords are specified
after the parameter list of the function but before the function body like this:

```
class Foo {
    Int foo() : final { 10; }
}
```

These keywords correspond to the function flags `fnFinal`, `fnAbstract` and `fnOverride`
respectively, which are documented [here](md://Storm/Type_system). If a function is marked
`abstract`, it is possible to omit the function body entirely. If the function is ever called (for
example by `super.foo`), it will throw an appropriate exception. For example:

```
class Foo {
    Int bar() : abstract;
}
```