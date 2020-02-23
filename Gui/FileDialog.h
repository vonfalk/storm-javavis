#pragma once
#include "Core/Io/Url.h"
#include "Core/EnginePtr.h"
#include "Core/CloneEnv.h"
#include "Frame.h"

namespace gui {

	/**
	 * A collection of file types suitable to pass to the open and save dialogs.
	 *
	 * The file type patterns have the form of a UNIX-style glob pattern, e.g. *.png
	 */
	class FileTypes : public Object {
		STORM_CLASS;
	public:
		// Create. Specify a title for all file types in here. This will be shown as a "catch all"
		// option in the file picker.
		STORM_CTOR FileTypes(Str *title);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Add a set of file types with a label.
		void STORM_FN add(Str *title, Array<Str *> *exts);

		// Elements stored in here.
		class Elem {
			STORM_VALUE;
		public:
			STORM_CTOR Elem(Str *title, Array<Str *> *exts);

			void STORM_FN deepCopy(CloneEnv *env);

			Str *title;
			Array<Str *> *exts;
		};

		// Title.
		Str *title;

		// File types.
		Array<Elem> *elements;

		// Allow any file types?
		Bool allowAny;

		// To string.
		void STORM_FN toS(StrBuf *to) const;
	};

	/**
	 * Show an "open" dialog box.
	 */
	MAYBE(Url *) STORM_FN showOpenDialog(FileTypes *types, MAYBE(Frame *) parent) ON(Ui);

	/**
	 * Show a "save" dialog box.
	 *
	 * "suggestedName" is the pre-filled name in the dialog.
	 */
	MAYBE(Url *) STORM_FN showSaveDialog(FileTypes *types, MAYBE(Frame *) parent, MAYBE(Str *) suggestedName) ON(Ui);

}
