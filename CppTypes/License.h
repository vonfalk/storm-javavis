#pragma once

class License {
public:
	License(const String &id, const String &pkg, const String &title, const String &body);

	// Identifier.
	String id;

	// Package.
	String pkg;

	// Title.
	String title;

	// Body.
	String body;
};
