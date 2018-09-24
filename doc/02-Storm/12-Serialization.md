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
- `Str`: A `Nat` containing the length of the string followed by that many bytes of data, encoded in utf-8.

The primitive types have pre-defined type ids, that are described by the enum `core.io.StoredId`.

Each object in a session then starts with a single `Nat` that describes the type of the object. If
the highest bit of the object identifier is set, the root is an `Object`, otherwise it is a value
type or a primitive type. If the type identifier indicates an object, the id shall be ignored, as
the header for an object contains a type id for the actual type of the object.


