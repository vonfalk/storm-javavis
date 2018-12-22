Serialization
==============

Storm has a library that provides serialization facilities for objects. These facilities are
implemented by the classes `core.io.ObjIStream` and `core.io.ObjOStream`, which specify the binary
format and provide the low-level operations required to implement serialization. These classes are
complemented by a serialization library written in Basic Storm, that generates the members `read`
and `write` for the classes that should be serialized.

The format used for serialization contains enough information to inspect it without having access to
the original type information; the needed type information is stored inside the serialization
stream. The class `util.TextObjStream` can be used to inspect and pretty-print previously serialized
objects.

The serialization format is designed to allow serializing multiple objects inside a single stream
while minimizing overhead. Therefore, type information is only sent once in one session (represented
by an instance of `ObjIStream` or `ObjOStream` respectively), and thus care needs to be taken to
ensure that the encoder and decoder use the same length of sessions. For example, if the encoder
writes two objects using one instance of `ObjOStream`, but the reader reads the two objects using
different instances of `ObjIStream`, deserialization will likely fail.


The binary format
-------------------

Primitive types are encoded as follows:

- `Byte`: A single byte containing the value.
- `Int` and `Nat`: 4 bytes stored in big endian.
- `Long` and `Word`: 8 bytes stored in big endian.
- `Str`: A `Nat` containing the length of the string followed by that many bytes of data, encoded in UTF-8.

Each serialized type is assigned a type id in the form of a `Nat` in order to refer to types in an
efficient manner. Type identifiers may appear before they are defined in the stream, for example
when referring to member types in a class. However, types are always defined whenever the first
instance of that particular type is read or written to the stream, so that the reader knows the
format of the serialized objects without access to the actual implementation. Primitive types
have pre-defined ids, defined by the enum `core.io.StoredId`. The id `core.io.StoredId.endId` (value
0) is of special interest. It is used to indicate the absence of a type, or the end of a
list. Identifiers below `core.io.StoredId.firstCustomId` are reserved for future primitive types,
and shall not be used.

A type definition starts with a single `Byte` indicating what kind of type is stored, described by
the `core.io.TypeInfo` enum. This is followed by the full name of the type in a format similar to
`core.lang.SimpleName`, serialized as a single string with parts ending with the ascii character 1,
start parenthesis as 2, end parenthesis as 3 and parameters are trailed by either character 4 if
they are passed by value or 5 if they are passed by reference (these are used in order not to
disallow any characters in class names). After the name, a `Nat` containing the type id of the
parent class follows. `endId` is used to indicate the absence of a parent class (for value types),
or that the parent is `Object` (for class types). After the parent class is a list of data
members. Each member consists of a `Nat` containing the type id of the member, and the name of the
member as a string. The end of the list is indicated by a `Nat` with the value `endId`. No string
follows this last type id.

Each object in a session starts with a single `Nat` describing the type of the stored object,
followed by the object itself (which, for the first object in a session, starts with a type
definition unless a primitive type was stored). Value types are simple, they consist of each member
variable serialized in the order they appear in the corresponding type definition (data member of
parent classes before the derived class). Each of these data members may start with a type
definition, if that type has not been encountered before. Classes, on the other hand, are a bit more
complex as we need to consider the possibility of cyclic object hierarchies and inheritance. A
serialized class starts with a `Nat` that identifies this particular object instance inside this
serialized object (instance identifiers are cleared between each object in a session, a single
top-level object may cause the creation of several instances). If the instance has been serialized
before, no more data follows. Otherwise, another `Nat` follows, indicating the actual type of the
instance (which may differ from the formal type stored in the type definition). After this id, the
members of the actual type is stored, just as for value types.
