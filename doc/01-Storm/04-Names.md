Names
======

Most things in the compiler has got a name. Therefore, it is good to know how name resolution works
in the compiler. Storm provides ways to alter the way name lookups are performed on a case-by-case
basis. Therefore, this section mostly describes the framework as well as the expected way to look up
names.

A name is a sequence of parts, much like a path in your file system. In a file system each part only
contains a name, but in Storm, each part also contains zero or more type parameters. These type
parameters are direct references to a type in the type system, so no arbitrary strings or values are
allowed as parameters to parts. This does not make much sense if we only consider packages (which
usually does not take any parameters), but if we also consider template types and functions, it
starts making sense. In this way, a name is either an absolute or relative path to something in the
type system.

Everything in the type system inherits from the class `core.lang.Named`. A `Named` contains, just
like each part of a name, a string and zero or more type parameters. `Named` also inherits from
`NameLookup`, which is an interface indicating that it is possible to look up names relative to that
point in the type system. This means, that given a name and an entry point, we can traverse the
`Name` hierarchy to find out what the name resolves to.

This process works as long as all names are relative to some fixed point. However, this often
becomes cumbersome since we either have to specify full names of everything we are using, or we have
to restrict ourselves to only a part of the type system (there is nothing that is equivalent to `..`
in names). Therefore, Storm has something that is called a Scope.

A `Scope` is an entry point into the type hirerarcy along with a policy of how we want to traverse
the type system. This policy might be: traverse from the entry point, if that fails, assume the name
is an absolute name and traverse from the root of the type system. This can also be extended to look
at any includes as well, or to do any number of interesting things. By this mechanism, possible for
one language to "leak" its name resolution semantics into other languages. This may be good, or
confusing. Consider, for example, embedding languages into each other. In this case it makes sense
in some cases that the embedded language follows the name resolution of the top-level language. In
other cases it does not.

As mentioned earlier, the parameters present at each part of a name are used to implement function
overloading and templates. This functionality is implemented by the class `NameSet`, which is
basically a collection of different names. The `NameSet` enforces that names does not collide with
each other (considering parameter), and implements how to resolve parameters. By default, `NameSet`
considers inheritance when examining which overloads are suitable. This can, however, be disabled on
a overload-by-overload basis if desired. `NameSet` also implements support for templates. A
`Template` is an object that can generate `Named` objects on demand. Whenever the `NameSet` is
requested for a name with parameters it does not find a match for, it asks a `Template` with the
same name (if it is present) to generate the match.