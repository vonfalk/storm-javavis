Syntax
=======

Syntax is a central part of Storm. The Storm compiler provides a parser that reads syntax from the
type system. This system is then used to implement other languages, like Basic Storm. The syntax
definitions in the compiled are designed so that it is easy to choose a set of packages and combine
their syntax, and in that way extend the language you are using.

This section will not discuss how syntax rules are written, please refer to [BNF Syntax](md://BNF_Syntax)
to learn how to use the built in syntax definition language.

Definition
-----------

The compiler implements a parser for context free grammars (with a few extensions, as we will see),
and therefore rules look a lot like regular BNF grammar. Grammars in Storm consist of a set of
rules. Each rule consists of a number of alternative matches, which are called options in
Storm. Each of these options contains a number of tokens. A token is either a reference to another
rule that should be matched, or a regular expression.

This is slightly different compared to regular BNF syntax. The BNF syntax does not have a separate
entry for options, but instead includes the alternative operator `|` to accomplish the same
goal. Storm provides the separate concept of options to allow rules to be easily extended from other
parts of the system, and to make the semantics of constructing a syntax tree clearer.

Storm syntax also diverges slightly from BNF by allowing the use of repetitions. Each option may
contain zero or one repetition. Currently, the repetitions present in regular expressions are
supported, ie. zero or one, zero or more and one or more times.

Semantics
----------

When parsing a string, using `core.lang.Parser`, the caller provides the rule that the parser should
start from. The parser starts from there and finds a valid parse tree that matched the given string,
or returns an error. The parser tries to match as far as possible in the given source string, and
returns how much of the source string successfully matches the input. This allows the user of the
parser to decide if a complete match is an error or not.

Some times, grammars are ambiguous. The Storm parser helps you to specify which of the many valid
parses should be chosen by looking at the priority that is assigned to each option. This priority is
an integer, and higher numbers have priority. Using these priorities, the storm parser tries to
emulate the behavior of Parsing Expression Grammar, where higher priorities are executed
"first". Since the Storm parser does not work the same way, there may be differences in some cases,
but in return Storm allows left-recursive rules.

Regular expressions are always greedy, which means that they sometimes interact in unexpected ways
with the rest of the tokens. This is most clearly illustrated by the following example:

```
A ::= "a*" "a"
B ::= "a"* "a"
```

In this case, the rule `A` uses two different regular expressions to match a sequence of at least
one `a`. However, this rule will never succeed, since the regular expression `"a*"` always matches
greedily, leaving nothing left for the final `"a"`. The rule `B`, on the other hand, works as
expected since we are instead instructing the parser to match the regular expression zero or more
times. This is not greedy.

Output
-------

When the parser has succeeded, the parser executes code for each node in the resulting tree. To
understand how this works, consider each rule to be a function that returns its part of the syntax
tree. The implementation of this function depends on which option was matched at that particular
point.

Each option has a function call declared, that tells the parser how to construct that part of the
syntax tree when that option is matched. Each of the tokens in the option may also be bound to a
variable and passed on to the function. These variables are evaluated by calling the matched
sub-rule whenever the value of the variable is needed. Regular expressions may also be bound to
variables, in which case a `core.lang.SStr` is created. It is also possible to mark a part of the
tokens in the option, and bind these to a variable as a `SStr`. The start position of the rule is
bound to the variable `pos`.

Aside from the function call, it is also possible to execute member functions on the return value
before it is returned. This is useful when tokens are within a repetition syntax, since a variable
can only be assigned once (otherwise it would not make sense). In this case, the function call may
create an array or other similar container, and instruct the parser to call the `push` member of the
array for each item in the repetition syntax. When the result is created it is bound to the
variable `me` in case it is needed.

Before the result is returned, if the object inherits from `core.lang.SObject`, the `pos` member is
set to the position where the option match started. This is why strings expand to `core.lang.SStr`
instead of regular strings. The `SStr` inherits from `SObject` and includes the position of the
string in the source file.

To ease the development of transforms, syntax rules may also be declared to take a number of
parameters. These parameters are treated as regular variables.

Everything in the syntax is resolved lazily. This means, for example, that any rules you refer to
does not need to be declared until the rule is actually considered by the parser. This can be
convenient if you know that the user will always include another package when using your syntax, or
something similar. The same holds true for function calls. The actual function and overload to be
called is not decided until the call is about to be made. This means that function calls does not
neccessarily have to be valid if they are never used, and that it is possible to express some kind
of overloads that are not possible in other, more static, languages. For example, a rule could call
the function `a(x)`, where `x` can be either a `A` or a `B`. In this case, we can declare two
versions of `a()`: `a(A x)` and `a(B x)`, even though `A` and `B` are not related. In other
languages it is neccessary to declare it like `a(Object x)`, and then manually check which type was
actually sent to `a`. Another convenient usage of this is that it is possible to indicate that
options should not return anything by specifying them returning the function call `void` (or
anything, really). Since the function call will never be used unless it actually has to be used, the
parser will work without problems as long as no one tries to get the result from the rule.

Parsing and threading
------------------------

At the moment, the parser is always executed on the `Compiler` thread. When creating the syntax
tree, only classes and actors are supported. Furthermore, actors have to be declared to run on the
compiler thread, since the parser does not yet implement support for message passing. This
restriction will probably be lifted in the future.


Representation
-----------------

NOTE: The representation is not exposed to the type system at the moment. This will be done in the
future.

NOTE: The location of these types is not decided yet. They will probably live in `core.syntax`, but
that is not decided yet.

The core compiler only knows of the in-memory representation of syntax. In each package, there is a
variable of the type `SyntaxRules`. A `SyntaxRules` object is a map from `Str` to `SyntaxRule`. Each
`SyntaxRule` in turn is a list of `SyntaxOption`s along with the declaration of the rule (if
present).

`SyntaxOption`s are what contains all tokens and the information required to build a node in the
syntax tree from the match. The tokens present in a `SyntaxOption` can not be accessed directly,
instead the `OptionIter` should be used. The `OptionIter` only allows forward iteration, but since
options can include repetition, sometimes it is not clear which token is the next one. Therefore,
the iterator provides all possible next states. Since there are only ever two next states, it simply
provides two "next" functions: `nextA` and `nextB`. Sometimes these functions will return an invalid
iterator, which means that that move was not possible, or that we are at the end of the option.

Lastly, tokens are represented by classes derived from `SyntaxToken`. The `SyntaxToken` class
contains information about any variable that is bound to the result of the token. Aside from that,
there are two subclasses: `RegexToken` and `TypeToken`, which represent a regular expression and
reference to another rule, respectively.

When combining syntax from different packages, `SyntaxSet` is useful. It provides a quick way of
merging all syntax in different packages for later use in the parser. This is what is used in Basic
Storm to compose the active syntax for parsing the file.
