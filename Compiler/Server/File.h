#pragma once
#include "Core/Str.h"
#include "Core/Io/Url.h"
#include "Core/EnginePtr.h"
#include "Compiler/Thread.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Represents a range in a file.
		 */
		class Range {
			STORM_VALUE;
		public:
			STORM_CTOR Range(Nat from, Nat to);

			Nat from;
			Nat to;
		};

		/**
		 * Text coloring options.
		 */
		enum TextColor {
			tNone,
			tComment,
			tDelimiter,
			tString,
			tConstant,
			tKeyword,
			tFnName,
			tVarName,
			tTypeName,
		};

		// Get the name of a color.
		Str *STORM_FN colorName(EnginePtr e, TextColor c);

		/**
		 * Represents a a region to be colored in a file.
		 */
		class ColoredRange {
			STORM_VALUE;
		public:
			STORM_CTOR ColoredRange(Range r, TextColor c);

			Range range;

			// TODO: Use strings or symbols here?
			TextColor color;
		};


		/**
		 * Represents an open file in the language server.
		 *
		 * TODO: We need a more efficient representation of the contents!
		 */
		class File : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR File(Nat id, Url *path, Str *content);

			// Get the range representing the entire file.
			Range STORM_FN full();

			// Perform an edit to this file. Returns how much of the syntax information was invalidated.
			Range STORM_FN replace(Range range, Str *replace);

			// Get all syntax colorings in the specified range. These are ordered and non-overlapping.
			Array<ColoredRange> *colors(Range r);

			// Our id.
			Nat id;

		private:
			// Path to the underlying file (so we can properly locate packages etc., we never actually read the file).
			Url *path;

			// File contents (TODO: Better representation!)
			Str *content;
		};

	}
}
