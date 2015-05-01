BNF Syntax
============

This is the syntax language that is implemented in the compiler itself. It is used to implement the
syntax of [Basic Storm](md://Basic_Storm), and can also be used to extend it or implement new
languages. Please refer to [Syntax](md://Storm/Syntax) for an explanation of the syntax system in
the compiler and its semantics. This section will only discuss how syntax is represented using this
language.

Input
-------

To use the BNF Syntax language, create a text file with the extension `bnf`. BNF Syntax is read
using the standard input in Storm, and therefore supports everything that is described
[here](md://Storm/Text_IO).

Name lookup and types
----------------------

The syntax language uses a `.` for separating names in types (in contrast to Basic Storm) and `<>`
to indicate parameters to a name part. When writing function calls `()` are used, just as in C. Note
that due to the implementation of names in Storm, it is not possible to combine `<>` with `()` like
this: `foo<Int>(a, b)`. There is an exception to this rule, due to constructor calls. Constructor
calls are written like this: `Type(a, b)`. This really means `Type.__ctor(a, b)`, so in this special
case, `Type<Int>(a, b)` is perfectly legal, as it means `Type<Int>.__ctor(a, b)`.

Currently, there is no support for `use`-statements in the syntax language. Therefore, type lookups
are always releative to:
1. the current package
2. the `core` package
3. the root package

Also note that, compared to Basic Storm, it is not possible to omit the parenthesis in a function
call in the syntax language. That makes the syntax language think you are referring to a variable
instead.

Function calls
---------------

The syntax language is very limited in terms of combining functions. Each syntax option has a
function call that generates the syntax tree for that option. This function call may also be a
variable in the option. Function calls take the form `<identifier>(<parameters>)`, where
`<parameters>` is a list of parameters separated by comma. Each parameter is either a variable, or a
literal. Function calls are not allowed as parameters. The only type of literals currently
implemented are numeric literals, which are expanded to `core.Int`.

Note that function calls are not resolved until they are needed. This means that invalid function
calls does not cause an error until someone tries to evaluate them. This is exploited with rules
that are not supposed to return any value, by specifying their function call as `void`. `void` is
not a special construct, but it will generate an error as the variable `void` is not defined in the
rule (unless, of course, you define it).

Comments
---------

Comments can be included anywhere. Comments start with `//` like in C, and continues to the end of
the line.

Visibility
-----------

As discussed in the [Storm](md://Storm/Syntax), there are no namespaces of syntax rules, so all
rules that are packed together in a `SyntaxSet` are considered visible. In Basic Storm, all included
packages are considered visible.

Rules
------

Rules does not need to be explicitly defined, unless they are required to take parameters. Declaring
a rule takes the form:

`<name>(<params>);`

Where `<params>` is a parameter list as it would be expressed in C. There may only be one visible
definition of a rule at the same time, definitions in different files and different packages may
collide.

Options
--------

Options are declared using the following syntax:

`<name> => <result> : <tokens>;`

__or__

`<name> => <result> :[<priority>] <tokens>;`

Where `<name>` is the name of the rule this option should be a part of, `<result>` is a function
call or a variable name that contains the value that should be returned from this option when
matched, `<tokens>` is the sequence of tokens the option contains. The optional `<priority>` is a
number indicating the priority of this option. If it is left out, the option is given priority 0.
Higher priority is executed "before" rules with a lower priority.

The tokens for the rule is a comma or dash separated list of tokens. Each token is either:

* `<name>` - the name of another rule.
* `<name>(<params>)` - the name of another rule, giving parameters to the rule.
* `"regex"` - a regular expression.

Each token is optionally followed by an indication of what to do with the match. This is either just
a name, which means that the parser should bind the match to that variable, or an arrow (`->`)
followed by a name, which means that the parser should execute `me.<name>(<match>)` when the
function call in the rule has been executed. At the moment, the `->` syntax does not work with regex
matches.

Each token is separated by either a comma or a dash (`-`). The comma separator represents the token
referring to the rule declared as the delimiter for the current file, while the dash only separates
the rules without acting as a special token. This is a convenience since the input is not
pre-tokenized, so matching some language-specific whitespace is a common task and has therefore been
given a convenient syntax.  Which rule is matched by the delimiter is declared on a file-by-file
basis like this:

`delimiter: <rule name>`

The delimiter does not allow any parameters to the rule. The delimiter must be declared before any
comma may be used, but the rule it refers to does not need to be defined before the `delimiter`
declaration, it does not even need to be declared in the same file or package.

For example, consider this file:

```
delimiter: Whitespace;
Whitespace => void : "[ \n\r\t]*";
A => void : "a", "b";
B => void : "a" - "b";
```

The rule `A` matches the same thing as the regular expression `a[ \n\r\t]*b` would do, while `B`
matches `ab` only. This difference is only because of our delimiter, and the rule we have declared
to be used as the delimiter.

It is possible to express repetition only using rules and options, but many times it is clearer to
write the repetitions explicit. The syntax language has support for the same repetitions present in
regular expressions: `*`, `?` and `+`. To use them, enclose the tokens that are to be repeated in
parenthesis and add the desired repetition after the closing parenthesis, like this:

```
A => void : "a", ("b",)* "c";
```

Which will match `a`, followed by zero or more `b`:s and finally a `c`. Each character is separated
by the delimiter rule (not specified for this example). Note that parenthesis can be either before
or after separators. There may even be separators on both side of the parenthesis. Currently, only
one repetition per option is supported. Break the option into multiple rules if more repeats are
needed. Due to the nature of repetitions, it is not practical to bind any maches inside a repeat to
a variable. Use the `->` syntax inside repetitions instead. What happens if you try to bind tokens
inside a repetition to a variable is that the variable will either not have a value, or be assigned
multiple times. Both of these cases will generate an error when the options are being
evaluated. This could be solved by adding support for array-variables, but implementing
array-variables in the syntax would significantly increase the complexity of the rules for
variables. Usually it is also more readable to generate variables explicitly by creating a new rule
for the repetition, which creates an array:

```
List => Array<T>() : (Rule -> push, )*;
```

Aside from repetitions, parenthesis can also be used to capture the string matched by a set of
tokens. This can not be used together with repetitions, since it also uses parenthesis. The syntax
is as follows:

`<tokens> ( <tokens> ) <name> <delimiter> <tokens>`

For example, to get the string within a pair of brackets (`{}`), assuming we want the string all the
way to the next matching closing bracket, we can do like this:

```
Match => void : "[^{}]";
Match => void : Match - Match;
Match => void : "{" - Match - "}";

Content => v : "{" - (Match) v - "}";
```

In this example, the rule `Match` does not return anything. Instead, we are binding whatever
characters `Match` matched to the variable `v` and return it. Of course, more than one token can be
included inside the parenthesis. This is very useful when matching complex identifiers, or skipping
parts of the input for parsing later on, possibly with a different syntax.


Examples
=========

The following syntax can be used to evaluate simple numeric expressions.

```
delimiter: Whitespace;             // Use delimiter
Whitespace => void : "[ \n\r\t]*"; // Whitespace

// Atoms. This is a second level of expressions, to ensure that our infix
// operators are parsed correctly. It is possible to accomplish this using
// priorities on options as well, but this is easier to understand.
Atom => create(a) : "[0-9]+" a;
Atom => negate(a) : Atom a;
Atom => v : "(", Expression v, ")";

// The root rule for expressions:
Expression => add(a, b) :[10] Expression a, "\+", Atom b;
Expression => sub(a, b) :[10] Expression a, "\-", Atom b;
Expression => v : Atom v;
```

Along with the following Basic Storm code, the syntax gets all of its semantics:

```
use core:lang;

// We need to store our Int somewhere since the parser can not
// handle value types.
class Number {
    Int value;

    ctor(Int v) {
        init() { value = v; }
    }

    Str toS() {
        value.toS;
    }
}

Number create(SStr n) {
    Number(n.v.toInt);
}

Number negate(Number n) {
    Number(0 - n.value);
}

Number add(Number a, Number b) {
    Number(a.value + b.value);
}

Number sub(Number a, Number b) {
    Number(a.value - b.value);
}
```

To use our syntax, we can do this:
```
use core:debug;    // for print() - will be improved later.
use core:io;       // for Url
use lang:bs:macro; // for name{}

void eval(Str v) {
    // Create a syntax set and a parser.
    SyntaxSet syntax = syntaxSet();
    Parser p(syntax, v, Url());

    // Parse, starting from the Expression rule.
    p.parse("Expression");
    if (p.hasError)
        p.throwError;

    // It succeeded, transform the result using the rules. The root rule takes
    // no parameters, as indicated by the array.
    Object result = p.transform([Object:]);

    // Print the result.
    print(v # " = " # result);
}

// Helper to create a syntax set from our code. Replace "demo" with the package
// that contains the syntax rules and the functions above.
SyntaxSet syntaxSet() {
    SyntaxSet set;
    Named n = (name{demo}).find(rootScope);
    if (n as Package)
        set.add(n);
    set;
}
```
