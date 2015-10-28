Expressions
============

Expressions in Basic Storm are anything you can write inside a function. Separate expressions are,
like in C, separated by a semicolon (`;`). Note that there are currently no real statements in Basic
Storm. What sets Basic Storm apart from C is that everything can appear as a part of an expression,
including local variable declarations, blocks and if-statements.

Literals
---------

Basic Storm supports the following literals:
* __Strings:__ enclosed in double quotes (`"abc"`). A string evaluates to a `core:Str` 
  object. Since `Str` objects are immutable, it is undefined wether each evaluation will return
  the same or a different `Str` object. (at the moment, they differ).
* __Integers:__ a simple number. Negative numbers are not supported (the unary `-` operator is 
  not implemented yet). Use `0 - 1` instead. Integer literals evaluate to a `core:Int` by default, 
  but the compiler will cast the literal to `core:Nat` or `core:Byte` automatically if the context 
  requires it, and the literal fits inside the target type without truncation. In some cases, it 
  is necessary to manually help the compiler by doing the casts manually. This is done by calling
  the `nat` or `byte` method on the `Int` object, like this: `1.nat`, or `nat(1)`.
* __Booleans:__ the reserved words `true` or `false`. Evaluates to `core:Bool`.
* __Arrays:__ enclosed in square brackets (`[]`), separated with comma (`,`). The literal may start with
  the desired type of the array followed by a colon (`:`). When the type is not entered, Basic Storm tries
  to infer the type according to the types in the array, and according to context. Arrays evaluate to an
  instance of the type `core:Array<T>`. Example: `Int:[1, 2, 3]` or `[1, 2, 3]`. Array literals are 
  implemented completely in Basic Storm, have a look at `lang:bs:array.bs`.

Function calls
---------------

Function calls looks much like function calls in most other languages, with some differences. The
easiest way to explain these differences is to explain how Basic Storms treats function
calls. Whenever Basic Storm sees an identifier (`foo`, or `foo(a, b)`), it constructs a `Name`
object that represents the seen identifier (resolving the type of any parameters in the process) and
asks the type system to look it up (see [Names](md://Storm/Names.md)). This means since `foo`
and `foo()` are represented by the same `Name`, they are treated equally. Basic Storm then examines
the `Named` returned by the lookup and acts accordingly. If it is a function, it calls the
function. If it is a variable it loads the value of the variable.

The interesting parts does not end here. Basic Storm also implements a custom lookup that not only
looks in the global namespace. It also looks for members in the type of the first parameter of the
function call. This means that `a.foo()` and `foo(a)` are treated almost the same way. There is a
slight difference between the two in the context of a member function. In a member function `foo(a)`
means that the `this` pointer may be used to form `this.foo(a)` if it makes sense, where `a.foo()`
does not allow for that.

This is mainly done for flexibility and extensibility. With this scheme it is possible to add
members to classes from other places by adding something that looks like a member function without
having to modify the class itself. A good example of this from C++ is operator overloading and
io-streams. The `<<` operator can be overloaded from anywhere to provide new functionality to the
output stream. Storm takes this idea and uses it for all functions.

The indifference between `foo` and `foo()` is useful when implementing something that should look
like a member variable, but can not be a member variable for some reason. A good example at the
moment is to implement read-only member variables. Since there is no support for constant member
variables, a getter function can be used to emulate this behavior. This is done for
`core:Array:count`. Later, Basic Storm will also introduce setter methods that will be executed
whenever someone tries to assign to a member variable that does not exist. This is not done yet.

Operators
----------

Operators in storm are defined by the `Operator` syntax rule. Operators are re-ordered after they
are parsed, to make it easier to understand the syntax rules, and thereby make it easier to add new
operators. The `Operator` rule is expected to return an `OpInfo` object which contains information
about the operator's priority and associativity. This object is also asked about what the operator
means.

By default, an operator is equivalent with calling the member function with the same name as the
operator on the left hand side object. This means that `1 + 2` is equivalent to `1.+(2)`. Note that
it is not possible to write function calls like this at the moment because of restrictions in the
syntax. At the moment, it is also not possible to declare operator functions, since a name is
required to contain letters. These two shortcomings will be addressed in the future.

New operators can be implemented by adding a new option to the `Operator` rule, either using the
default `lOperator` or `rOperator` to follow the same semantics as most of the built-in operators,
or create a custom object overriding `OpInfo` and implements a custom meaning for the operator.

Currently, two operators have a special meaning: assignment and string concatenation. The assignment
operator will simply call `a.=(b)` if `a` and `b` are values, but it provides a default
implementation of the assignment operator for class and actor types. The string concatenation
operator, `#`, is implemented in Basic Storm in `lang:bs:concat.bs` and uses a `core:StrBuf` class
to build a string from all elements involved. The same string buffer is used for all elements, even
if there are more than two present. Elements are appended to the `StrBuf` object using the `add`
member of `StrBuf`. `StrBuf` contains overloads for strings, integers and booleans. If an `add`
overload is not found, the value is converted to a string using the `toS` function for the object
(either member or non-member, just like function calls).

To implement operators like `+=`, Basic Storm takes a slightly different approach compared to
C++. This kind of operators (hereby called _combined operators_), are implemented by a syntax rule
that is applied before the regular `Operator` rule. This rule is named `COperator`, and only has two
options, one which passes the `Operator` rule through it, and one that matches an `Operator` with an
`=` on the end. When the second option is matched, an instance of the `CombinedOperator` class is
created for the combined operator. This will first attempt to call the member function, just like
the default operators would do, for example `a += 1` tries to call `a.+=(1)`. If this is not
possible for some reason, it falls back to rewriting the expression from `<lhs> <op>= <rhs>` to
`<lhs> = <lhs> <op> <rhs>`. This means that if `a.+=(1)` is not callable in the example above, 
`a = a + 1` will be tried. This means that you get all the combined operators for free in Basic
Storm, but you can still provide more efficient implementations if you want. In C++, you always have
to implement these operators, even though the meaning is seldom ambiguous.


Variables
----------

Variables are declared like in C, by writing `<type> <name>`. Variables can be initialized at
creation by either `<type> <name> = <expr>`, or <type> <name>(<params>)`. When a variable is
initialized using the assignment syntax, the type can be inferred by using the syntax 
`var <name> = <expr>`. Any local variables live as long as the scope they are declared in.

Since variables are also expressions, they can be declared inside expressions. Variables still have
the scope of their enclosing block, and the declaration returns the assigned value of the variable.
Because of this, expressions like this are valid in Basic Storm: `1 + (Int a = 20);`. It is a good
idea to enclose things with parenthesis to make the intention clearer, both for you and the parser!
Expressions like: `1 + Int a = 20 + 3` are not clear what they mean. The usefulness of this kind of
expressions are found in if statements or loops, where it becomes possible to do like this:

```
if ((Int i = fn(a)) < 3) {
   print(i);
}
```

It also allows declaring variables in for loops without anything special.

Blocks
--------

Blocks are also expressions in Basic Storm. Blocks return the value of the last expression inside
them, much like `progn` in Lisp. Functions work the same way, the function returns the result of the
outermost block, which is the value of the last statement in the block. This means that it is
possible to introduce a new block with temporary variables inside an expression, which is very
useful when implementing new syntax. Compare this to macros in C, where you have to express your
entire macro as an expression if you want it to be able to return a value. This makes it difficult
(if not impossible) to introduce temporary variables in C-macros. In Basic Storm it is easy, just
introduce a new block:

```
Str result = "Hello" + {
    StrBuf tmp;
    tmp.add(", ");
    tmp.add("World");
    tmp;
} + "!";
```

This is exactly how the string concatenation operator is implemented. The code above will be
generated by `Str result = "Hello" + (", " # "World") + "!";`. As mentioned before, this would be
hard, if not impossible, to do using regular C-style macros.

Return
-------

Return is not implemented yet. Return values by placing an expression evaluating to the desired
return value as the last expression in a function. Return will be implemented later along with break
and continue.

Async
------

When Basic Storm calls a function that should run on another thread, it will by default send the
message and wait until the called function has returned. During this time the current thread will
accept new function calls from other threads. If you wish to not wait for a result, use the `spawn`
keyword right before the function call. This makes the function call return a `Futre<T>` instead,
and you can choose when and if you want to get the result back.

Automatic type casting
----------------------

Basic Storm has support for automatic type casting, in a way that is similar to C++. However, in
Basic Storm, less conversions are implicit. Automatic conversions only upcast types (ie. to a more
general type), or if a constructor is declared as `cast ctor(T from)`. For the built in types, very
few constructors are declared like this, so Basic Storm does not generally do anything unexpected.

The major usage of automatic casts in the standard library is the `Maybe` type, this is used to
allow `Maybe`-types to behave like regular object references.

In the `if` statement, Basic Storm tries to find a type that can store both the result from the true
and the false branch. At the moment, this is not too sophisticated. A common base class is chosen if
present (this works for `Maybe` as well). Otherwise, Basic Storm tries to cast one type to the
other, and chooses the combination that is possible. If all of these fails, Basic Storm considers it
is not possible to find a common type, even if there is an unrelated type that both the true and
false branch can be casted to. In this case, it is necessary to explicitly help the compiler to
figure out the type.

The automatic casting of literals is limited to very simple cases at the moment, and sometimes it is
necessary to explicitly cast literals (for example in `if` statements).

Downcasting
------------

When you have a variable of a base type, say `Object`, and want to test if it is an instance of a
more specific type, you can use `if-as` syntax:

```
if (o as Foo) {
    // o has the type Foo inside of here.
}
```

Which will cause the variable to the left of `of` to be treated as the more specific type inside the
block of the if-branch. This only works if the variable to the left of `of` is a local variable or a
parameter. Otherwise, the check is made, but it will not be possible to access the derived type. An
alternate syntax will probably fix this problem later, maybe:

`if (o = a.b as Foo)`

References and null
--------------------

At the moment, Storm has no support for null references. All reference variables (referring classes
or actors) are initialized to something when they are created, there is no null keyword and there is
no good way of testing for null references. The idea is to provide a special, nullable type, for
this purpose. This type will probably be written as `T?`, and will not be accessible until there has
been an explicit check for null, probably using:

```
if (o) {
    // o now has type T, not T?
}
```

Implementation details of this type is not finished yet. It may be a regular templated type (just as
`Array<T>`) or a more specific type.

Function pointers
------------------

Function pointers are represented by the type `core:Fn<A, B, ...>`, where the first parameter
to the template is the return type, and the rest are parameter types. To create a pointer to a
function, use this syntax:

```
Fn<Int, Int> ptr = &myFunction(Int);
```

In this case, we assume that `myFunction` returns an `Int`. As you can see, the parameters of the
function has to be declared explicitly. This may not be necessary later on.

Function pointers may also contain a this-pointer of an object or an actor (not a value). This is
done like this:

```
Fn<Int, Int> ptr = &myObject.myFunction(Int);
```

or

```
Fn<Int, Int> ptr = &myObject->myFunction(Int);
```

The difference between the two syntaxes is that the first one (using `.`) creates a function pointer
with a strong reference to `myObject` while the second one (using `->`) creates a weak reference. As
Storm does not yet fully support weak references, it is your job to keep the object alive if you use
a weak reference, otherwise the program crashes whenever the function pointer is used. This will be
improved as soon as Storm has proper weak references.

As you can see, the type of a function pointer bound with an associated object is identical to that
of a function pointer without an associated object. This means that you can easily create pointers
to functions associated with some kind of state and treat them just like regular functions. Function
pointers are also as flexible as the regular function calls. This means that if both:

```
&object.function(A);
```

and

```
&function(Object, A);
```

Works. In the first case, the function pointer will only take one parameter, while it will take two
in the second case. Bu utilizing this, it is possible to choose if the first parameter of a function
should be bound or not.

To call the function from the function pointer, use the `call` member of the `Fn` object.

Note that the threading semantics is a little different when using function pointers compared to
regular function calls. Since the function pointer does know where it is invoked from (and to make
interaction with C++ easier), the function pointer decides runtime if it should send a message or
not. This means that in some cases, the function pointer sees that a message is not needed when a
regular call would have sent a message (for example, calling a function associated to a thread from
a function that can run on any thread). The rule for this is simple, whenever the function pointed
to is to be executed on a different thread than the currently executed one, we send a message. At
the moment, it is not possible to do `spawn` when calling functions using function pointers.

The function pointer type can also be declared as follows:

```
fn(Int, Int)->Int => Fn<Int, Int, Int>
```

Different parts of can be left out. The following lines are equivalent:

```
fn()->void
fn->void
fn()->
fn->
```

This syntax is implemented in `lang:bs:fnPtr.bs`.


Null and maybe
---------------

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

Automatic type conversions are done when calling functions, so you can call a functions taking
formal parameter of type `T?` with an actual parameter of type `T`.

At the moment, automatic type casting does not always work as expected, mainly when the compiler has
to deduce the type from two separate types. For example in the if-statement in `fn` below:

```
Str? fn2() {
    "foo";
}

Str? fn() {
    if (condition) {
        fn2();
    } else {
        ?"bar";
    }
}

```

In this case, we need to help the compiler deducing the return value of the if-statement. The
compiler sees the types `Str` and `Str?`, and can fail to deduce the return type. This happens
especially when a possible type is neither `Str`, nor `Str?`. To help the compiler, add a `?` before
`"bar"` to cast it into a `Str?`.

`Maybe` instances are initialized to null, but sometimes it is desirable to assign a null value to
them after creation. In Basic Storm, there are two ways of doing this. Either by assigning a newly
created `Maybe` instance, or using the special keyword `null`. `null` can be a part of an
expression, but it has no meaning unless it is used in a context where it can be automatically
casted to a `Maybe` type. For example:

```
Str? v = "Hello, World";

// Set 'v' to null.
v = null;

// Equivalent, but more verbose.
v = Str?();
```

Note that this has no meaning:

```
// 'v' vill have the type 'void' here. 'null' does not have 
// a type unless it can be automatically casted to something usable.
var v = null;
```

Automatic type conversions
---------------------------

Automatic type conversions are done only for a few built-in types at the moment. Integer literals
are automatically converted from the standard `Int` type to either `Nat` or `Byte` as
suitable. Apart from this, `T` can be automatically casted to `T?` if needed. Currently, there are
no way of implementing custom conversions. This will probably be included later!

If two or more functions are callable after type conversions, the one which requires the least
amount of conversions is called. Casts from `T` to `T?`, as well as integer literal casts costs more
than the regular calls. In the case of equal distance calls, an error will be shown.

Syntax
-------

The syntax used to describe expressions have their roots in the `Expr` syntax rule. There is also a
rule named `Stmt`. The difference between an expression and a statement on the syntax level is that
a statement ends with a semicolon, while an expression does not. This distinction is done to allow
blocks to not end in a semicolon, as they do in C. It is, however, possible to include blocks in a
regular expression as well. After the syntax level, expressions and statements are treated equally,
as expressions.
