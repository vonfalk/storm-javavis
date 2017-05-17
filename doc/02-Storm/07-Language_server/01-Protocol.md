Protocol
=========

The language server expects to communicate with a client using standard input and standard
output. Because of this, it is the client's responsibility to start the language server as it
desires. To allow regular use of standard input and standard output for debugging purposes, all data
in the protocol starts with the null-byte. Everything else is considered UTF-8-encoded text which is
to be shown to the user.

The protocol used in the language server is based on s-expressions in LISP. Therefore, it can
represent four basic datatypes: integers, strings, symbols and cons-cells. A cons-cell is an object
containing two members, usually referred to as `car` and `cdr` or `first` and `rest`. These members
may refer to any instance of the datatypes in the protocol, including other cons-cells. They can
therefore be used to form more complex data structures, such as singly linked lists. A singly linked
list is implemented by creating one cons-cell for each element in the list. The list content is
stored in the `car` or `first` member of each cons cell. `cdr` or `rest` is used to point to the
next cons-cell in the list. The last element contains the symbol `nil`, which is the LISP equivalent
of `null` in C++ or Java.

The protocol is encoded in binary and based around messages. A message is a complete s-expression,
usually a list of some sort. As mentioned earlier, messages always start with a null-byte. The
null-byte is followed by the length of the message, in bytes, encoded as a 32-bit (= 4-bytes)
integer in big-endian format. The *length* bytes following the size constitutes the message body,
which contains a single s-expression. S-expressions always start with a single byte indicating the
type of the stored element, followed by the actual data. The supported types are as follows:

* `0x00`: the symbol `nil`. No additional data is required. `nil` is a commonly used symbol,
  and as such it has a special encoding.
* `0x01`: a cons-cell. Following this byte are two additional s-expressions. The first one
  is the contents of the `car` or `first` member, the second is the contents of the `cdr`
  or `rest` member.
* `0x02`: a number. The following four bytes contain the actual value of a 32-bit number stored
  in big-endian format, just like the size.
* `0x03`: a string. The first four bytes contain the length of the string (in bytes) as a 32-bit
  number in big-endian format. Following the size is the bytes consituting the string. These are
  encoded in UTF-8.
* `0x04`: a symbol. This encoding is used for a symbol that has not previously been sent. The first
  4 bytes is an integer containing an id to use for this symbol in future communication. Following
  the id is a string (without the type-mark) containing the name of the symbol.
* `0x05`: a symbol. This encoding is used for symbols that have previously been sent encoded as `0x04`.
  This representation only contains 4 bytes containing the identifier of the symbol. This identifier
  should be the same as a symbol previously sent using `0x04` in either direction.

