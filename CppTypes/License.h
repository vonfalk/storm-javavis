#pragma once

class License {
public:
	License(const String &id, const String &pkg, const String &cond, const String &title, const String &body);

	// Identifier.
	String id;

	// Package.
	String pkg;

	// Condition (if any).
	String condition;

	// Title.
	String title;

	// Body.
	String body;
};
