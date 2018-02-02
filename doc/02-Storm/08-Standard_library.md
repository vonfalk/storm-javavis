Standard library
==================

Storm provides a standard library located in the `core` package. This section is to highlight the
most important types and functions from there, the rest can be explored from within Storm.

The most important types present in the standard library are the primitive types. In Storm,
primitive types are implemented to look as much as possible like user-defined types. However, they
follow different rules in calling conventions than a user-defined type of the same size. On X86,
using the `cdecl` calling convention in C++, user-defined objects are always returned on the stack,
while primitive types (pointers, integers, and so on) are always returned in the `eax`
register. Primitive types in Storm will of course follow these conventions to be compatible with C++
and to not lose execution speed.

Primitive types
----------------

Storm provides a few basic numeric types:

* Bool - boolean value.
* Byte - unsigned 8-bit integer.
* Int - signed 32-bit integer.
* Nat - unsigned 32-bit integer.
* Long - signed 64-bit integer.
* Word - unsigned 64-bit integer.
* Float - single-precision floating point number (32-bit).

More types are planned (changes may be introduced):

* Double - double-precision floating point number (64-bit).

All types in storm have a fixed size across all platforms. Values may still be promoted during
calculations, so do not rely on overflow semantics. At the moment, the compiler does not optimize as
aggressively as current C compilers, so checks and similar things will not be optimized away.


Containers
-----------

Storm provides the following container types:

* Str - string, immutable.
* Array<T> - array.
* Map<K, V> - hash map.
* Set<K> - hash set.
* WeakSet<K> - weak hash set. Stores weak references to heap-allocated objects.

All containers provide iterators that allow accessing individual elements. These iterators work
similarly to iterators in C++, except that they provide the members `k` and `v` to access the key
and value of the current element rather than using pointer dereferencing. `k` is optional, and
`Set<K>` does not provide that member. Due to its volatile nature, `WeakSet<K>` uses another type of
iterator, more similar to Java. This iterator is a one-shot iterator that can be queried for the
next element rather than an iterator to the begin and the end. Both iterators are supported in the
range-based for loop in Basic Storm.

The `Str` class stores characters in UTF-16 internally, but does not provide an API to access this
representation. Instead, iterators can be used to iterate through the string one UTF-32 codepoint at
a time. One can also use the function `core.io.readStr` to create a `TextReader` that allows reading
character by character from the string.


Other types
------------

Storm also provides some higher-level types:

* Str - string, immutable.
* StrBuf - string buffer for more efficient string concatenations.
* Thread - represents a OS thread.
* Future<T> - future, for inter-thread communication.
* FnPtr<R, ...> - function pointer.
* Moment - timestamp from a high-resolution timer.
* Duration - difference between two `Moment`s.
