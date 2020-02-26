#pragma once
#include "Core/Io/Url.h"
#include "Core/EnginePtr.h"
#include "Core/CloneEnv.h"
#include "Frame.h"

namespace gui {

	/**
	 * A collection of file types suitable to pass to the open and save dialogs.
	 *
	 * A file type here directly corresponds to a file extension.
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

			// Get a title with all extensions in it.
			Str *extTitle() const;
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
	 * A file picker dialog window.
	 *
	 * This is a common base class for both "open" and "save" varieties of the dialog.
	 */
	class FilePicker : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Create an open dialog for files.
		static FilePicker *STORM_FN open(FileTypes *types) ON(Ui);

		// Create a save dialog for files. 'suggestion' is the name suggestion present initially.
		static FilePicker *STORM_FN save(FileTypes *types, MAYBE(Str *) suggestion) ON(Ui);

		// Create a file picker dialog that allows selecting directories.
		static FilePicker *STORM_FN folder(EnginePtr e) ON(Ui);

		// Set the default folder when the file picker is opened. If unset, a reasonable default
		// will be picked by the underlying implementation.
		FilePicker *STORM_ASSIGN defaultFolder(Url *folder);

		// Set the title of the dialog.
		FilePicker *STORM_ASSIGN title(Str *caption);

		// Set the label of the OK button. If not set, a reasonable default will be selected by the
		// system.
		FilePicker *STORM_ASSIGN okLabel(Str *label);

		// Set the label of the Cancel button. If not set, a reasonable default will be selected by
		// the system.
		FilePicker *STORM_ASSIGN cancelLabel(Str *label);

		// Allow multiple file selection.
		FilePicker *STORM_FN multiselect();
		FilePicker *STORM_ASSIGN multiselect(Bool allow);

		// Show the dialog. Returns 'false' if cancelled.
		Bool STORM_FN show(MAYBE(Frame *) parent);

		// Get the result, assuming it is a single file.
		MAYBE(Url *) STORM_FN result();

		// Get the entire array of results. Use this for multi-select.
		Array<Url *> *STORM_FN results();

	private:
		// Mode.
		enum {
			mOpen, mSave, mFolder,
			mMulti = 0x10
		};

		// Create. Called from the factory functions.
		FilePicker(Nat mode, FileTypes *types, Str *defName);

		// Mode.
		Nat mode;

		// File types to pick. NULL if we're picking directories.
		MAYBE(FileTypes *) types;

		// Suggested filename (intended for "save" dialogs).
		MAYBE(Str *) defName;

		// Initial folder.
		MAYBE(Url *) defFolder;

		// Title.
		MAYBE(Str *) caption;

		// OK caption.
		MAYBE(Str *) ok;

		// Cancel caption.
		MAYBE(Str *) cancel;

		// Picked files.
		Array<Url *> *res;

		// Clean a Url according to the "types" object (i.e. make sure it has a proper
		// extension). 'selected' is the selected filter.
		Url *cleanUrl(Nat selected, Url *url);
	};

}
