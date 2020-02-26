#include "stdafx.h"
#include "FilePicker.h"
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

	Str *FileTypes::Elem::extTitle() const {
		StrBuf *buf = new (title) StrBuf();
		*buf << title << S(" (") << join(exts, S(", ")) << S(")");
		return buf->toS();
	}

	void FileTypes::toS(StrBuf *to) const {
		*to << title;
		for (Nat i = 0; i < elements->count(); i++) {
			Elem &e = elements->at(i);
			*to << S("\n  ") << e.extTitle();
		}

		if (allowAny)
			*to << S("\n  All files");
	}

	FilePicker::FilePicker(Nat mode, FileTypes *types, Str *defName)
		: mode(mode), types(types), defName(defName), res(new (engine()) Array<Url *>()) {}

	FilePicker *FilePicker::open(FileTypes *types) {
		return new (types) FilePicker(mOpen, types, null);
	}

	FilePicker *FilePicker::save(FileTypes *types, MAYBE(Str *) suggestion) {
		return new (types) FilePicker(mSave, types, suggestion);
	}

	FilePicker *FilePicker::folder(EnginePtr e) {
		return new (e.v) FilePicker(mFolder, null, null);
	}

	FilePicker *FilePicker::defaultFolder(Url *folder) {
		defFolder = folder;
		return this;
	}

	FilePicker *FilePicker::title(Str *title) {
		caption = title;
		return this;
	}

	FilePicker *FilePicker::okLabel(Str *label) {
		ok = label;
		return this;
	}

	FilePicker *FilePicker::cancelLabel(Str *label) {
		cancel = label;
		return this;
	}

	FilePicker *FilePicker::multiselect() {
		return multiselect(true);
	}

	FilePicker *FilePicker::multiselect(Bool allow) {
		if (allow)
			mode |= mMulti;
		else
			mode &= ~Nat(mMulti);
		return this;
	}

	MAYBE(Url *) FilePicker::result() {
		if (res->any())
			return res->at(0);
		else
			return null;
	}

	Array<Url *> *FilePicker::results() {
		return new (this) Array<Url *>(*res);
	}

	Url *FilePicker::cleanUrl(Nat selected, Url *url) {
		if (!types)
			return url;
		if (types->elements->empty())
			return url;

		Str *ext = url->ext();

		// The first element is the collection of all supported files.
		if (selected == 0) {
			for (Nat i = 0; i < types->elements->count(); i++) {
				FileTypes::Elem &e = types->elements->at(i);
				for (Nat j = 0; j < e.exts->count(); j++)
					if (*ext == *e.exts->at(i))
						return url;
			}

			// Add the first one.
			for (Nat i = 0; i < types->elements->count(); i++) {
				FileTypes::Elem &e = types->elements->at(i);
				for (Nat j = 0; j < e.exts->count(); j++)
					return url->withExt(e.exts->at(i));
			}
		} else {
			selected--;
			if (selected >= types->elements->count())
				return url;

			FileTypes::Elem &e = types->elements->at(selected);
			for (Nat i = 0; i < e.exts->count(); i++)
				if (*ext == *e.exts->at(i))
					return url;

			if (e.exts->any())
				return url->withExt(e.exts->at(0));
		}

		return url;
	}


#ifdef GUI_WIN32

	static void addFileType(COMDLG_FILTERSPEC *to, Str *title, Array<Str *> *exts) {
		to->pszName = title->c_str();
		StrBuf *e = new (exts) StrBuf();
		for (Nat i = 0; i < exts->count(); i++) {
			if (i > 0)
				*e << S(";");
			*e << S("*.") << exts->at(i);
		}
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
			addFileType(&specs[i + 1], e.extTitle(), e.exts);
		}

		// If we want to add "any type", do that now.
		if (types->allowAny) {
			specs[count - 1].pszName = S("Any type");
			specs[count - 1].pszSpec = S("*.*");
		}

		return dialog->SetFileTypes(count, specs);
	}

	Bool FilePicker::show(MAYBE(Frame *) parent) {
		res = new (this) Array<Url *>();

		DWORD options = FOS_FORCEFILESYSTEM;
		Nat mode = this->mode;
		if (mode & mMulti) {
			options |= FOS_ALLOWMULTISELECT;
			mode &= ~Nat(mMulti);
		}

		if (mode == mOpen)
			options |= FOS_PATHMUSTEXIST;
		else if (mode == mSave)
			options |= FOS_OVERWRITEPROMPT;
		else if (mode == mFolder)
			options |= FOS_PICKFOLDERS | FOS_PATHMUSTEXIST;


		IFileDialog *dialog = null;
		HRESULT result;
		if (mode == mSave)
			result = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
		else
			result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));

		if (SUCCEEDED(result)) {
			result = dialog->SetOptions(options);
		}

		if (SUCCEEDED(result) && defFolder) {
			IShellItem *shItem = null;
			result = SHCreateItemFromParsingName(defFolder->format()->c_str(), null, IID_PPV_ARGS(&shItem));

			if (SUCCEEDED(result))
				result = dialog->SetFolder(shItem);

			if (shItem)
				shItem->Release();
		}

		if (SUCCEEDED(result) && ok) {
			result = dialog->SetOkButtonLabel(ok->c_str());
		}

		if (SUCCEEDED(result) && caption) {
			result = dialog->SetTitle(caption->c_str());
		}

		if (SUCCEEDED(result) && types) {
			result = addFileTypes(dialog, types);
		}

		if (SUCCEEDED(result) && defName) {
			result = dialog->SetFileName(defName->c_str());
		}

		if (SUCCEEDED(result)) {
			HWND p = NULL;
			if (parent)
				p = parent->handle().hwnd();

			// Will return an "error" if cancelled.
			result = dialog->Show(p);
		}

		UINT selectedType = 1;
		if (SUCCEEDED(result)) {
			result = dialog->GetFileTypeIndex(&selectedType);
		}

		if (mode == mOpen) {
			IShellItemArray *fileArray = null;
			IFileOpenDialog *openDialog = null;

			if (SUCCEEDED(result)) {
				result = dialog->QueryInterface(IID_PPV_ARGS(&openDialog));
			}

			if (SUCCEEDED(result)) {
				result = openDialog->GetResults(&fileArray);
			}

			if (openDialog)
				openDialog->Release();

			DWORD count = 0;
			if (SUCCEEDED(result)) {
				result = fileArray->GetCount(&count);
			}

			for (DWORD at = 0; SUCCEEDED(result) && at < count; at++) {
				IShellItem *file = null;
				result = fileArray->GetItemAt(at, &file);

				PWSTR path = NULL;
				if (SUCCEEDED(result)) {
					result = file->GetDisplayName(SIGDN_FILESYSPATH, &path);
				}

				if (SUCCEEDED(result)) {
					Url *url = parsePath(engine(), path);
					if (mode == mSave)
						url = cleanUrl(selectedType - 1, url);
					res->push(url);
				}

				if (path)
					CoTaskMemFree(path);
				if (file)
					file->Release();
			}

			if (fileArray)
				fileArray->Release();
		} else {
			IShellItem *file = null;
			if (SUCCEEDED(result))
				result = dialog->GetResult(&file);

			PWSTR path = NULL;
			if (SUCCEEDED(result)) {
				result = file->GetDisplayName(SIGDN_FILESYSPATH, &path);
			}

			if (SUCCEEDED(result)) {
				Url *url = parsePath(engine(), path);
				if (mode == mSave)
					url = cleanUrl(selectedType - 1, url);
				res->push(url);
			}

			if (path)
				CoTaskMemFree(path);
			if (file)
				file->Release();
		}

		// Cleanup.
		if (dialog)
			dialog->Release();

		return SUCCEEDED(result);
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
