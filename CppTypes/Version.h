#pragma once

class Version {
public:
	Version(const String &id, const String &pkg, const String &version);

	// Identifier.
	String id;

	// Package.
	String pkg;

	// Version string.
	String version;
};
