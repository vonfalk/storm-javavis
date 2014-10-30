#pragma once

#include "Timestamp.h"

/**
 * A class for managing path names.
 */
class Path {
	friend std::wostream &operator <<(std::wostream &to, const Path &path);
public:
	// Get the path of the executable file we're currently running from.
	static Path executable();

	// Get a path representing a file relative to the executable path.
	static Path executable(const Path &path);

	// Get a path to the root of the debug environment (only valid during debug).
	static Path dbgRoot();

	// Create an empty path.
	Path();

	// Create a path object from a string.
	explicit Path(const String &path);

	// Comparison.
	bool operator ==(const Path &o) const;
	inline bool operator !=(const Path &o) const { return !(*this == o); }

	// Concat this path with another path, the other path must be relative.
	Path operator +(const Path &other) const;
	Path &operator +=(const Path &other);

	// Add a string to go deeper into the hierarchy. If this object is not
	// a directory already, it will be made into one.
	Path operator +(const String &file) const;
	Path &operator +=(const String &file);

	// To string.
	String toS() const;

	// Status about this path.
	bool isDir() const;
	bool isAbsolute() const;
	bool isEmpty() const;

	// Make this obj a directory.
	inline void makeDir() { isDirectory = true; }

	// Get parent directory.
	Path parent() const;

	// Get the title of this file or directory.
	String title() const;
	String titleNoExt() const;

	// Has file extension? (ext shall not contain .)
	bool hasExt(const String &ext) const;

	// Does the file exist?
	bool exists() const;

	// Delete the file.
	void deleteFile() const;

	// Make this path relative to another path. Absolute-making is accomplished by
	// using the + operator above.
	Path makeRelative(const Path &to) const;

	// Find the children of this path.
	vector<Path> children() const;

	// Modified time.
	Timestamp mTime() const;

	// Created time.
	Timestamp cTime() const;

private:

	// Internal representation is a list of strings, one for each part of the pathname.
	vector<String> parts;

	// Is this a directory?
	bool isDirectory;

	// Parse a path string.
	void parseStr(const String &str);

	// Simplify a path string, which means to remove any . and ..
	void simplify();
};
