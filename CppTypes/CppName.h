#pragma once

/**
 * Representation of a C++ name. The underlying representation is a regular string for simplicity,
 * but this type provides some operations to aid in manipulating the names.
 */
class CppName : public String {
public:
	// Create an empty name.
	explicit CppName();

	// Create from a string.
	explicit CppName(const String &name);

	// Append.
	CppName operator +(const String &part) const;

	// Get the last part.
	String last() const;

	// Get the parent name.
	CppName parent() const;
};
