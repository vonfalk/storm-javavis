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
local variables in a function (ie. `<type> <name>`). Initializing variables at the declaration site
is not supported yet. There are also no visibility declarations yet, everything is public at the
moment.

Note that it is not possible to inherit from a class _and_ declare the class as an actor. If the
super class was an actor the new class will be an actor as well, otherwise it will not.

Constructors
--------------

Types may have constructors. If no destructor is declared, Basic Storm generates a constructor for
you. Constructors are declared by the special syntax:

`ctor(<parameters>) { <body> }`

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
