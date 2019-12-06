#pragma once

class License {
public:
	License(const String &id, const String &pkg, const String &cond, const String &title, const String &author, const String &body);

	// Identifier.
	String id;

	// Package.
	String pkg;

	// Condition (if any).
	String condition;

	// Title.
	String title;

	// Author(s).
	String author;

	// Body.
	String body;
};
