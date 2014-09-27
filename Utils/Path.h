#pragma once

// Nice functions for manipulating paths. These are LEGACY and should not be used! Use the Path class below instead!

// Get the file from a path.
String getPathFile(const String &path);

// Get the directory from a path. If it is already a directory, "path" is returned.
String getDirectory(const String &path);

// Get the parent directory from a path. Assumes "path" is a directory.
String getParentDir(const String &path);

// Is this a directory?
bool isDirectory(const String &path);

// Get the path of our executable file.
String getExecutablePath();

// Get a path relative to the executable path.
String getExecutablePath(const String &rel);

// Make a path relative another.
String makePathRelative(const String &path, const String &to);
String makePathNotRelative(const String &relative, const String &to);

// A class for managing path names.
class Path {
	friend std::wostream &operator <<(std::wostream &to, const Path &path);
public:
	// Get the path of the executable file we're currently running from.
	static Path executable();

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
	void makeDir() { isDirectory = true; }

	// Get parent directory.
	Path parent() const;

	// Get the title of this file or directory.
	String title() const;
	String titleNoExt() const;

	// Does the file exist?
	bool exists() const;

	// Delete the file.
	void deleteFile() const;

	// Make this path relative to another path. Absolute-making is accomplished by
	// using the + operator above.
	Path makeRelative(const Path &to) const;
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
