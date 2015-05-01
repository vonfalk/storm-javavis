Input
======

Storm generally reads input from text files in the file system. Storm provides tools for reading and
converting text files to the internal character representation (currently UTF-16). Most languages
will this standard mechanism to read their input, and will therefore accept text files as described
here.

When reading a file (or a stream), the encoding is determined based on the first few bytes of the
file. If a Byte Order Mark (BOM) is found as the first character of the file, the encoding of the
BOM is assumed to represent the encoding of the entire file. If the BOM is not found, the file is
assumed to be encoded in UTF-8 (even if your system codepage is something different). Currently,
UTF-8 and UTF-16 (both little and big endian) are supported.

A BOM is the UTF codepoint `U+FEFF`. If encoded in UTF-8, it will be encoded into the following
bytes: `EF BB BF`, UTF-16 uses either `FE FF` or `FF FE` based on endianness. In the standard
decoding process, the BOM is silently consumed before the decoded text is passed on further, so the
users of Storm's standard text input will not notice the presence or absence of a BOM.
