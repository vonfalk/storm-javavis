Standard library
==================

Storm provides a standard library located in the `core` package. This section is to highlight the
most important types and functions from there, the rest can be explored from within Storm.

The most important types present in the standard library are the primitive types. In Storm,
primitive types are implemented to look as much as possible like user-defined types. However, they
follow different rules in calling conventions than a user-defined type of the same size. On X86,
using the cdecl calling convention in C++, user-defined objects are always returned on the stack,
while primitive types (pointers, integers, and so on) are always returned in the `eax`
register. Primitive types in Storm will of course follow these conventions to be compatible with C++
and to not loose execution speed.

Primitive types
----------------

Storm provides a few basic numeric types:

* Bool - boolean value.
* Byte - unsigned 8-bit integer.
* Int - signed 32-bit integer.
* Nat - unsigned 32-bit integer.
* Float - single-precision floating point number (32-bit).

More types are planned (changes may be introduced):

* Long - signed 64-bit integer.
* Word - unsigned 64-bit integer.
* Double - double-precision floating point number (64-bit).

All types in storm have a fixed size across all platforms. Values may still be promoted during
calculations, so do not rely on overflow semantics. At the moment, the compiler does not optimize as
aggressively as current C compilers, so checks and similar things will not be optimized away.


Other types
------------

Storm also provides some higher-level types:

* Str - string, immutable.
* StrBuf - string buffer for more efficient string concatenations.
* Array<T> - array template.
* Thread - represents a OS thread.
* Future<T> - future, for inter-thread communication.
* FnPtr<R, ...> - function pointer.

The `Str` class currently stores characters in UTF-16, but does not provide an API to access
individual characters or to cut strings based on character positions. This is partly because the
actual representation and semantics has not yet been decided. It is, however, possible to read
characters from a string in UTF-32 using streams. The function `core.io.readStr` creates a
`TextReader` that allows you to read character by character from the string.
