Use statements
===============

The first thing present in a Basic Storm source file are the `use` statements, if needed. By
default, Basic Storm looks up names relative to the current package, then relative to the core
package and last relative to the root package. A `use` statement tells Basic Storm to look for types
relative to those packages as well. Otherwise, the programmer would always have to write full names
for types and functions not in the current package or the core package.

Basic Storm uses the following syntax for names:

```
a:b<c:d>:e
```

Where the `<>` are used to indicate parameters to that specific part (see
[Names](md://01-Storm/04-Names.md) for an explanation of names in Storm). Also note that the `.`
operator is not used to separate names. This operator is only used when accessing members of a
value, much like in C++ (using the `::` operator).