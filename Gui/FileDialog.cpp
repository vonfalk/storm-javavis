#include "stdafx.h"
#include "FileDialog.h"
#include "Core/Join.h"
#include "Core/Convert.h"

namespace gui {

	FileTypes::FileTypes(Str *title) : title(title), elements(new (engine()) Array<Elem>()) {}

	void FileTypes::deepCopy(CloneEnv *env) {
		cloned(elements, env);
	}

	void FileTypes::add(Str *title, Array<Str *> *exts) {
		elements->push(Elem(title, exts));
	}

	FileTypes::Elem::Elem(Str *title, Array<Str *> *exts) : title(title), exts(exts) {}

	void FileTypes::Elem::deepCopy(CloneEnv *env) {
		cloned(exts, env);
	}

	void FileTypes::toS(StrBuf *to) const {
		*to << title;
		for (Nat i = 0; i < elements->count(); i++) {
			Elem &e = elements->at(i);
			*to << S("\n  ") << e.title << S(" (") << join(e.exts, S(", ")) << S(")");
		}

		if (allowAny)
			*to << S("\n  All files (*.*)");
	}


#ifdef GUI_WIN32

	static void addFileType(COMDLG_FILTERSPEC *to, Str *title, Array<Str *> *exts) {
		to->pszName = title->c_str();
		StrBuf *e = new (exts) StrBuf();
		join(e, exts, S(";"));
		to->pszSpec = e->toS()->c_str();
	}

	static HRESULT addFileTypes(IFileDialog *dialog, FileTypes *types) {
		Nat count = types->elements->count() + 1;
		if (types->allowAny)
			count++;

		// Note: We need to stack allocate this, since we will store GC pointers in it.
		COMDLG_FILTERSPEC *specs = (COMDLG_FILTERSPEC *)alloca(sizeof(COMDLG_FILTERSPEC) * count);

		// Fill the "full" listing.
		Array<Str *> *all = new (types) Array<Str *>();
		for (Nat i = 0; i < types->elements->count(); i++)
			all->append(types->elements->at(i).exts);

		addFileType(&specs[0], types->title, all);

		// Fill the remaining items.
		for (Nat i = 0; i < types->elements->count(); i++) {
			FileTypes::Elem &e = types->elements->at(i);
			addFileType(&specs[i + 1], e.title, e.exts);
		}

		// If we want to add "any type", do that now.
		if (types->allowAny) {
			specs[count - 1].pszName = S("Any type");
			specs[count - 1].pszSpec = S("*.*");
		}

		return dialog->SetFileTypes(count, specs);
	}


	MAYBE(Url *) showOpenDialog(FileTypes *types, MAYBE(Frame *) parent) {
		IFileDialog *dialog = null;
		HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));

		if (SUCCEEDED(result)) {
			result = dialog->SetOptions(FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);
		}

		if (SUCCEEDED(result)) {
			result = addFileTypes(dialog, types);
		}

		if (SUCCEEDED(result)) {
			HWND p = NULL;
			if (parent)
				p = parent->handle().hwnd();

			result = dialog->Show(p);
		}

		IShellItem *file = null;
		if (SUCCEEDED(result)) {
			result = dialog->GetResult(&file);
		}

		PWSTR path = NULL;
		if (SUCCEEDED(result)) {
			result = file->GetDisplayName(SIGDN_FILESYSPATH, &path);
		}

		Url *url = null;
		if (SUCCEEDED(result)) {
			url = parsePath(types->engine(), path);
		}

		// Cleanup.
		if (path)
			CoTaskMemFree(path);
		if (file)
			file->Release();
		if (dialog)
			dialog->Release();

		return url;
	}

	MAYBE(Url *) showSaveDialog(FileTypes *types, MAYBE(Frame *) parent, MAYBE(Str *) suggestedName) {
		IFileDialog *dialog = null;
		HRESULT result = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));

		if (SUCCEEDED(result)) {
			DWORD flags = FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM;
			if (!types->allowAny)
				flags |= FOS_STRICTFILETYPES;

			result = dialog->SetOptions(flags);
		}

		if (SUCCEEDED(result)) {
			result = addFileTypes(dialog, types);
		}

		// Pick a default file type.
		if (SUCCEEDED(result)) {
			Bool done = false;
			for (Nat i = 0; i < types->elements->count() && !done; i++) {
				FileTypes::Elem &e = types->elements->at(i);
				for (Nat j = 0; j < e.exts->count() && !done; j++) {
					const wchar *ext = e.exts->at(j)->c_str();

					if (ext[0] == '*' && ext[1] == '.') {
						result = dialog->SetDefaultExtension(ext + 2);
						done = true;
					}
				}
			}
		}

		if (SUCCEEDED(result)) {
			if (suggestedName)
				result = dialog->SetFileName(suggestedName->c_str());
		}

		if (SUCCEEDED(result)) {
			HWND p = NULL;
			if (parent)
				p = parent->handle().hwnd();

			result = dialog->Show(p);
		}

		IShellItem *file = null;
		if (SUCCEEDED(result)) {
			result = dialog->GetResult(&file);
		}

		PWSTR path = NULL;
		if (SUCCEEDED(result)) {
			result = file->GetDisplayName(SIGDN_FILESYSPATH, &path);
		}

		Url *url = null;
		if (SUCCEEDED(result)) {
			url = parsePath(types->engine(), path);
		}

		// Cleanup.
		if (path)
			CoTaskMemFree(path);
		if (file)
			file->Release();
		if (dialog)
			dialog->Release();

		return url;
	}

#endif

#ifdef GUI_GTK

	static void addFileType(GtkFileChooser *chooser, Str *title, Array<Str *> *exts) {
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, title->utf8_str());

		for (Nat i = 0; i < exts->count(); i++)
			gtk_file_filter_add_pattern(filter, exts->at(i)->utf8_str());

		gtk_file_chooser_add_filter(chooser, filter);
	}

	static void addFileTypes(GtkFileChooser *chooser, FileTypes *types) {
		Nat count = types->elements->count() + 1;
		if (types->allowAny)
			count++;

		// Fill the "full" listing.
		Array<Str *> *all = new (types) Array<Str *>();
		for (Nat i = 0; i < types->elements->count(); i++)
			all->append(types->elements->at(i).exts);

		addFileType(chooser, types->title, all);

		// Fill the remaining items.
		for (Nat i = 0; i < types->elements->count(); i++) {
			FileTypes::Elem &e = types->elements->at(i);
			addFileType(chooser, e.title, e.exts);
		}

		// If we want to add "any type", do that now.
		if (types->allowAny) {
			GtkFileFilter *filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter, "Any type");
			gtk_file_filter_add_pattern(filter, "*");
			gtk_file_chooser_add_filter(chooser, filter);
		}
	}

	static MAYBE(Url *) showFileDialog(GtkFileChooserAction action,
									FileTypes *types,
									MAYBE(Frame *) parent,
									MAYBE(Str *) suggestedName) {

		GtkWindow *parentWin = null;
		if (parent)
			GTK_WINDOW(parent->handle().widget());
		GtkWidget *dialog = gtk_file_chooser_dialog_new(NULL, parentWin, action,
														"Cancel", GTK_RESPONSE_CANCEL,
														"OK", GTK_RESPONSE_ACCEPT,
														NULL);
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		addFileTypes(chooser, types);

		if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
			gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
		}

		if (suggestedName) {
			gtk_file_chooser_set_current_name(chooser, suggestedName->utf8_str());
		}

		Url *result = null;
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			char *filename = gtk_file_chooser_get_filename(chooser);
			Engine &e = types->engine();
			result = parsePath(new (e) Str(toWChar(e, filename)));
			g_free(filename);

			// TODO: Try to auto-append the proper file extension if we're saving.
		}

		gtk_widget_destroy(dialog);

		return result;
	}

	MAYBE(Url *) showOpenDialog(FileTypes *types, MAYBE(Frame *) parent) {
		return showFileDialog(GTK_FILE_CHOOSER_ACTION_OPEN, types, parent, null);
	}

	MAYBE(Url *) showSaveDialog(FileTypes *types, MAYBE(Frame *) parent, MAYBE(Str *) suggestedName) {
		return showFileDialog(GTK_FILE_CHOOSER_ACTION_SAVE, types, parent, suggestedName);
	}

#endif

}
