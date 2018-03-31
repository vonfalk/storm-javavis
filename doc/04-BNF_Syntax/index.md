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
using the standard text input in Storm, and therefore supports everything that is described
[here](md://Storm/Text_IO).

Name lookup and types
----------------------

The syntax language uses a `.` for separating names in types (in contrast to Basic Storm) and `<>`
to indicate parameters to a name part. When writing function calls `()` are used, just as in C. Note
that due to the implementation of names in Storm, it is not possible to combine `<>` with `()` like
this: `foo<Int>(a, b)`. There is an exception to this rule in constructor calls. Constructor calls
are written like: `Type(a, b)`. This really means `Type.__ctor(a, b)`, so in this special case,
`Type<Int>(a, b)` is perfectly legal, as it means `Type<Int>.__ctor(a, b)`.

Types are looked up in the following order in the BNF language:
1. the current package
2. any `use`d packages
3. the core package
4. the root package

The type `SStr` is always visible, even if the package `core.lang` is not visible.

Also note that, compared to Basic Storm, it is not possible to omit the parenthesis in a function
call in the syntax language. That makes the syntax language think you are referring to a variable
instead.

Extensibility
--------------

The syntax language may also be extended similarly to Basic Storm with some minor differences. In
the syntax language, it is more important to carefully consider which syntax is required in order to
be able to parse something else. Therefore, it is recommended to always create separate packages for
extensions to the syntax language in order to avoid any cycles that could cause strange behaviour.

For this reason, the syntax language uses a separate `use` statement for extending the syntax. `use`
only extends the namespace with more packages while the keyword `extend` only extends the set of
productions visible while parsing the syntax in the file. Just like in Basic Storm, `extend`
statements must be the first statements in the file. `use` statements can be located anywhere, even
if it is good practice to put them right after any `extend` statements.


Function calls
---------------

The syntax language is very limited in terms of combining functions. Each syntax option has a
function call that generates the syntax tree for that option. This function call may also be a
variable in the option. Function calls take the form `<identifier>(<parameters>)`, where
`<parameters>` is a list of parameters separated by comma. Each parameter is either a variable, a
traversal or a literal. Function calls taking parameters are not allowed as parameters. Traversals
have the form 'x.y.z' and allow accessing members (such as member variables and member functions) in
addition to plain variables. The only type of literals currently implemented are numeric literals
and booleans, which are expanded to `core.Int` and `core.Bool` respectively.

Comments
---------

Comments can be included anywhere. Comments start with `//` like in C, and continues to the end of
the line.

Visibility
-----------

All options that can be resolved from the `use`d packages (in Basic Storm, any use statements in the
`.bs`-file are included) are visible and considered during the parsing process. Currently everything
declared in the syntax language is considered `public`.

Rules
------

Rules need to be declared. Declaring a rule takes the form:

`<result> <name>(<params>);`

Where `<params>` is a parameter list as it would be expressed in C. `<result>` is the return type of
the rule. The rule always lives in the package where the syntax file is located.

Options
--------

Options are declared using the following syntax:

`<name> => <result> : <tokens>;`

__or__

`<name>[<priority>] => <result> : <tokens>;`

__or__

`<name> : <tokens>;`

__or__

`<name>[<priority>] : <tokens>;`


Where `<name>` is the name of the rule this option should be a part of (fully qualified if
required), `<result>` is a function call or a variable name that contains the value that should be
returned from this option when matched. It can be omitted if the rule is declared to return
`void`. `<tokens>` is the sequence of tokens the option contains. The optional `<priority>` is a
number indicating the priority of this option. If it is left out, the option is given priority 0.
Higher priority is executed before rules with a lower priority, so rules with higher priority are
more greedy than ones with lower priority.

The tokens for the rule is a comma or dash separated list of tokens. Each token is either:

* `<name>` - the name of another rule.
* `<name>(<params>)` - the name of another rule, giving parameters to the rule.
* `"regex"` - a regular expression.

Each token is optionally followed by an indication of what to do with the match. This is either just
a name, which means that the parser should bind the match to that variable, or an arrow (`->`)
followed by a name, which means that the parser should execute `me.<name>(<match>)` when the
function call in the rule has been executed.

By default the result of each rule is transformed before it is bound to a variable or sent to a
member function. If this is not desired (e.g. you want to capture parts of the syntax tree for later
transformation), append an `@` sign after the name or the regex like: `Foo@ -> bar`. This just
passes the raw syntax tree on to the function `bar` instead of transforming it first. Note that
because of this, it is useless to specify parameters along with `@`. When a `@` is appended to a
regex, the resulting type is `SStr` instead of `Str`.

Each token is separated by either a comma or a dash (`-`). The comma separator represents the token
referring to the rule declared as the delimiter for the current file, while the dash only separates
the rules without acting as a special token. This is a convenience since the input is not
pre-tokenized, so matching some language-specific whitespace is a common task and has therefore been
given a convenient syntax.  Which rule is matched by the delimiter is declared on a file-by-file
basis like this:

`delimiter = <rule name>`

The delimiter does not allow any parameters to the rule. The delimiter must be declared before any
comma may be used, but the rule it refers to does not need to be defined before the `delimiter`
declaration, it does not even need to be declared in the same file or package.

For example, consider this file:

```
delimiter: Whitespace;
void Whitespace();
Whitespace : "[ \n\r\t]*";

void A();
A : "a", "b";

void B();
B : "a" - "b";
```

The rule `A` matches the same thing as the regular expression `a[ \n\r\t]*b` would do, while `B`
matches `ab` only. This difference is only because of our delimiter, and the rule we have declared
to be used as the delimiter.

It is possible to express repetition only using rules and options, but many times it is clearer to
write the repetitions explicit. The syntax language has support for the same repetitions present in
regular expressions: `*`, `?` and `+`. To use them, enclose the tokens that are to be repeated in
parenthesis and add the desired repetition after the closing parenthesis, like this:

```
void A();
A : "a", ("b",)* "c";
```

Which will match `a`, followed by zero or more `b`:s and finally a `c`. Each character is separated
by the delimiter rule (not specified for this example). Note that parenthesis can be either before
or after separators. There may even be separators on both side of the parenthesis. Currently, only
one repetition per option is supported. Break the option into multiple rules if more repeats are
needed. Due to the nature of repetitions, it is not practical to bind any matches inside a repeat to
a variable. Use the `->` syntax inside repetitions instead. What happens if you try to bind tokens
inside a repetition to a variable is that the variable will either not have a value, or be assigned
multiple times. Both of these cases will generate an error when the options are being
evaluated. This could be solved by adding support for array-variables, but implementing
array-variables in the syntax would significantly increase the complexity of the rules for
variables. Usually it is also more readable to generate variables explicitly by creating a new rule
for the repetition, which creates an array:

```
Array<T> List();
List => Array<T>() : (Rule -> push, )*;
```

Aside from repetitions, parenthesis can also be used to capture the string matched by a set of
tokens. This can not be used together with repetitions, since it also uses parenthesis. The syntax
is as follows:

`<tokens> ( <tokens> ) <name> <delimiter> <tokens>`

For example, to get the string within a pair of brackets (`{}`), assuming we want the string all the
way to the next matching closing bracket, we can do like this:

```
void Match();
Match : "[^{}]";
Match : Match - Match;
Match : "{" - Match - "}";

Str Content();
Content => v : "{" - (Match) v - "}";
```

In this example, the rule `Match` does not return anything. Instead, we are binding whichever
characters `Match` matched to the variable `v` and return it. Of course, more than one token can be
included inside the parenthesis. This is very useful when matching complex identifiers, or skipping
parts of the input for parsing later on, possibly with a different syntax. The `->` and `@` syntax
is also supported with captures.

Syntax Highlighting and Indentation
====================================

The syntax language allows specifying annotations that are used by the language server to provide
syntax highlighting and automatic indentation of the code to users of a language. These are
discussed in more detail in [Syntax Highlighting](md://BNF_Syntax/Syntax_Highlighting) and
[Indentation](md://BNF_Syntax/Indentation).

Examples
=========

The example *Eval* on the [Examples](md://Introduction/Examples) page shows how to use the syntax
language together with Basic Storm to create a simple expression evaluator.
