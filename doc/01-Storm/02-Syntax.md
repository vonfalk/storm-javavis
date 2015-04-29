Syntax
=======

Syntax is a central part of Storm. The Storm compiler provides a parser that reads syntax from the
type system. This system is then used to implement other languages, like Basic Storm. The syntax
definitions in the compiled are designed so that it is easy to choose a set of packages and combine
their syntax, and in that way extend the language you are using.

This section will not discuss how syntax rules are written, please refer to [BNF
Syntax](md://03-BNF_Syntax) to learn how to use the built in syntax definition language.

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

To ease the development of transforms, syntax rules may also be declared to take a number of
parameters. These parameters are treated as regular variables.