#pragma once

#include "Color.h"
#include "Bitmask.h"

// A basic stream class.
// Any object that can follows the following interface can be loaded/saved using this class.
// class Foo {
// public:
//     static Foo load(Stream &from);
//     void save(Stream &to) const;
// }
// Note that even though the type of the parameter to "write" can be deduced, it is the standard
// to write it out, so that innocent changes to datatypes does not break the file structure without
// warnings.
class Stream {
public:
	enum Mode { mNone = 0x0, mRead = 0x1, mWrite = 0x2, mRandom = 0x3 };

	Stream();
	virtual ~Stream();

	// Clone this stream.
	virtual Stream *clone() const = 0;

	// Non-implemented reads and write simply assert.
	virtual nat read(nat size, void *to);
	virtual void write(nat size, const void *from);

	virtual nat64 pos() const = 0;
	virtual nat64 length() const = 0;
	virtual void seek(nat64 to) = 0;
	virtual bool more() const;

	virtual bool valid() const = 0;

	// Read formatted data from the stream. The error() flag is set on error.
	template <class T> inline T read() { return T::load(*this); }
	template <class T> inline T *readPtr() { return T::load(*this); }
	template <class T> inline void read(T &obj) { obj.load(*this); }
	template <> inline bool read<bool>() { return readBool(); }
	template <> inline char read<char>() { return readChar(); }
	template <> inline byte read<byte>() { return readByte(); }
	template <> inline nat read<nat>() { return readNat(); }
	template <> inline int read<int>() { return readInt(); }
	template <> inline nat64 read<nat64>() { return readNat64(); }
	template <> inline int64 read<int64>() { return readInt64(); }
	template <> inline double read<double>() { return readDouble(); }
	template <> inline float read<float>() { return readFloat(); }
	template <> inline String read<String>() { return readString(); }

	template <class T>
	bool readList(vector<T> &to) {
		nat length = read<nat>();
		if (error()) return false;

		to.clear();
		to.reserve(length);
		for (nat i = 0; i < length; i++) {
			to.push_back(read<T>());
			if (error()) return false;
		}
		return true;
	}

	// See if any error has occurred.
	virtual bool error() const;

	// Clear the error flag.
	virtual void clearError();

	// Write formatted data to the stream.
	template <class T> inline void write(const T &v) { v.save(*this); }
	template <class T> inline void writePtr(const T *v) { v->save(*this); }
	template <> inline void write<bool>(const bool &b) { writeBool(b); }
	template <> inline void write<char>(const char &c) { writeChar(c); }
	template <> inline void write<byte>(const byte &b) { writeByte(b); }
	template <> inline void write<nat>(const nat &v) { writeNat(v); }
	template <> inline void write<int>(const int &v) { writeInt(v); }
	template <> inline void write<nat64>(const nat64 &v) { writeNat64(v); }
	template <> inline void write<int64>(const int64 &v) { writeInt64(v); }
	template <> inline void write<double>(const double &v) { writeDouble(v); }
	template <> inline void write<float>(const float &v) { writeFloat(v); }
	template <> inline void write<String>(const String &v) { writeString(v); }

	template <class T>
	void writeList(const vector<T> &objs) {
		write<nat>(objs.size());
		for (nat i = 0; i < objs.size(); i++) write<T>(objs[i]);
	}

	// Write an entire stream.
	void write(Stream *from);

protected:
	// Error during stream operation.
	bool hasError;

private:
	// Chunk size when copying streams.
	static const nat copySize = 10 * 1024;

	// Some functions needed to get the templating to work.
	bool readBool();
	char readChar();
	byte readByte();
	nat readNat();
	int readInt();
	nat64 readNat64();
	int64 readInt64();
	double readDouble();
	float readFloat();
	String readString();

	void writeBool(bool b);
	void writeChar(char ch);
	void writeByte(byte b);
	void writeNat(nat n);
	void writeInt(int n);
	void writeNat64(nat64 n);
	void writeInt64(int64 n);
	void writeDouble(double d);
	void writeFloat(float f);
	void writeString(const String &s);
};

BITMASK_OPERATORS(Stream::Mode);
