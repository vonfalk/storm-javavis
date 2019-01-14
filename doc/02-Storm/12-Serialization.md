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

- `Bool`: A single byte containing the value 0 for `false`. Other values (usually 1) indicate `true`.
- `Byte`: A single byte containing the value.
- `Int` and `Nat`: 4 bytes stored in big endian.
- `Long` and `Word`: 8 bytes stored in big endian.
- `Float`: The IEEE754 representation stored as a `Nat`.
- `Double`: The IEEE754 representation stored as a `Word`.
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

A type description is stored as follows:
- A `Byte` indicating what kind of type is stored, described by the `core.io.TypeInfo` enum.
- A `Str` containing the name of the type, serialized as follows: The format is similar to the
  textual representation of `core.lang.Name`. Each part is stored as a string ending with the ascii
  character 1. If parameters are present, they are surrounded by the ascii character 2 and 3 (for
  start- and end parentheses). Each parameter ends in either the ascii character 4 if passed by
  value or 5 if passed by reference.
- A `Nat` containing the id of the parent type, or `core.io.StoredId.endId` if none.

The remainder of the type description depends on which values are present in the first byte
(`core.io.TypeInfo` is a bitmask):
- `tuple`: This is an array of tuples. A series of `Nat` values containing the id of each type
   in the tuple are stored. The end of the list is indicated by `core.io.StoredId.endId`.
- `maybe`: This is a maybe-type. A single `Nat` containing the contained type id is stored.
- `custom`: This is a type with custom serialization. Noting more is stored, but the serialized
  data for instances of this type can not be extracted without asking the actual type.
- Otherwise: This is a regular class- or value type (depending on the presence of `classType`).
  The members of the class or value are stored as pairs, first a `Nat` containing the type id of
  the member, followed by a `Str` containing the member's name. The end of the list is indicated
  by a type with id `core.io.StoredId.endId`. No string follows the last type id.

Each object in a session starts with a single `Nat` describing the type of the stored object,
followed by the object itself (which, for the first object in a session, starts with a type
definition unless a primitive type was stored). Types are then stored depending on their type
(described by the type description) as follows:

If the stored value is a class type (i.e. `core.io.TypeInfo.classType` is not present in the first
byte of the type description), a `Nat` describing the object's id is stored first. If this id was
serialized before, nothing more is stored; this is a reference to the previously stored
object. Otherwise, a `Nat` containing the type of the object follows (as the dynamic type might
differ from the static type). If this is a new type id, the type description directly follows the
new id, as we need to access that. If any parent classes are not yet serialized, they also
follow. Finally, the data of the object is stored as described below.

Values are simpler, as their static type always matches their dynamic type, and they are always
copied. Therefore, values are always stored as the data of the object, without the additional
headers required for storing class types.

The data of the object also depends on the first byte of the type description as follows:
- If the type is a built-in type, use the default serialization of that type (the `read` and `write`
  functions for a regular `IStream` and `OStream`).
- If the type is an array of tuples, a `Nat` containing the number of elements in the array, followed
  by that many repetitions of the types in the type description.
- If the type is a maybe type, a `Bool` indicating the presence of a value is stored first. If the
  value is true, then a single instance of the contained type is stored directly afterwards.
- If the type is a custom type, the representation is unspecified.
- If the type is a regular class- or value type, each member is serialized in the order specified
  by the type description.

