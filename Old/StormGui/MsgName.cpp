#include "stdafx.h"
#include "MsgName.h"

#define ENTRY(name) case name: return WIDEN(#name);

namespace stormgui {

	// List from: http://wiki.winehq.org/List_Of_Windows_Messages
	const wchar *msgName(UINT msg) {
		switch (msg) {
			ENTRY(WM_NULL);
			ENTRY(WM_CREATE);
			ENTRY(WM_DESTROY);
			ENTRY(WM_MOVE);
			ENTRY(WM_SIZE);
			ENTRY(WM_ACTIVATE);
			ENTRY(WM_SETFOCUS);
			ENTRY(WM_KILLFOCUS);
			ENTRY(WM_ENABLE);
			ENTRY(WM_SETREDRAW);
			ENTRY(WM_SETTEXT);
			ENTRY(WM_GETTEXT);
			ENTRY(WM_GETTEXTLENGTH);
			ENTRY(WM_PAINT);
			ENTRY(WM_CLOSE);
			ENTRY(WM_QUERYENDSESSION);
			ENTRY(WM_QUIT);
			ENTRY(WM_QUERYOPEN);
			ENTRY(WM_ERASEBKGND);
			ENTRY(WM_SYSCOLORCHANGE);
			ENTRY(WM_ENDSESSION);
			ENTRY(WM_SHOWWINDOW);
			// ENTRY(WM_CTLCOLOR); // obsolete
			ENTRY(WM_WININICHANGE);
			ENTRY(WM_DEVMODECHANGE);
			ENTRY(WM_ACTIVATEAPP);
			ENTRY(WM_FONTCHANGE);
			ENTRY(WM_TIMECHANGE);
			ENTRY(WM_CANCELMODE);
			ENTRY(WM_SETCURSOR);
			ENTRY(WM_MOUSEACTIVATE);
			ENTRY(WM_CHILDACTIVATE);
			ENTRY(WM_QUEUESYNC);
			ENTRY(WM_GETMINMAXINFO);
			ENTRY(WM_PAINTICON);
			ENTRY(WM_ICONERASEBKGND);
			ENTRY(WM_NEXTDLGCTL);
			ENTRY(WM_SPOOLERSTATUS);
			ENTRY(WM_DRAWITEM);
			ENTRY(WM_MEASUREITEM);
			ENTRY(WM_DELETEITEM);
			ENTRY(WM_VKEYTOITEM);
			ENTRY(WM_CHARTOITEM);
			ENTRY(WM_SETFONT);
			ENTRY(WM_GETFONT);
			ENTRY(WM_SETHOTKEY);
			ENTRY(WM_GETHOTKEY);
			ENTRY(WM_QUERYDRAGICON);
			ENTRY(WM_COMPAREITEM);
			ENTRY(WM_GETOBJECT);
			ENTRY(WM_COMPACTING);
			ENTRY(WM_COMMNOTIFY);
			ENTRY(WM_WINDOWPOSCHANGING);
			ENTRY(WM_WINDOWPOSCHANGED);
			ENTRY(WM_POWER);
			// ENTRY(WM_COPYGLOBALDATA);
			ENTRY(WM_COPYDATA);
			ENTRY(WM_CANCELJOURNAL);
			ENTRY(WM_NOTIFY);
			ENTRY(WM_INPUTLANGCHANGEREQUEST);
			ENTRY(WM_INPUTLANGCHANGE);
			ENTRY(WM_TCARD);
			ENTRY(WM_HELP);
			ENTRY(WM_USERCHANGED);
			ENTRY(WM_NOTIFYFORMAT);
			ENTRY(WM_CONTEXTMENU);
			ENTRY(WM_STYLECHANGING);
			ENTRY(WM_STYLECHANGED);
			ENTRY(WM_DISPLAYCHANGE);
			ENTRY(WM_GETICON);
			ENTRY(WM_SETICON);
			ENTRY(WM_NCCREATE);
			ENTRY(WM_NCDESTROY);
			ENTRY(WM_NCCALCSIZE);
			ENTRY(WM_NCHITTEST);
			ENTRY(WM_NCPAINT);
			ENTRY(WM_NCACTIVATE);
			ENTRY(WM_GETDLGCODE);
			ENTRY(WM_SYNCPAINT);
			ENTRY(WM_NCMOUSEMOVE);
			ENTRY(WM_NCLBUTTONDOWN);
			ENTRY(WM_NCLBUTTONUP);
			ENTRY(WM_NCLBUTTONDBLCLK);
			ENTRY(WM_NCRBUTTONDOWN);
			ENTRY(WM_NCRBUTTONUP);
			ENTRY(WM_NCRBUTTONDBLCLK);
			ENTRY(WM_NCMBUTTONDOWN);
			ENTRY(WM_NCMBUTTONUP);
			ENTRY(WM_NCMBUTTONDBLCLK);
			ENTRY(WM_NCXBUTTONDOWN);
			ENTRY(WM_NCXBUTTONUP);
			ENTRY(WM_NCXBUTTONDBLCLK);
			ENTRY(EM_GETSEL);
			ENTRY(EM_SETSEL);
			ENTRY(EM_GETRECT);
			ENTRY(EM_SETRECT);
			ENTRY(EM_SETRECTNP);
			ENTRY(EM_SCROLL);
			ENTRY(EM_LINESCROLL);
			ENTRY(EM_SCROLLCARET);
			ENTRY(EM_GETMODIFY);
			ENTRY(EM_SETMODIFY);
			ENTRY(EM_GETLINECOUNT);
			ENTRY(EM_LINEINDEX);
			ENTRY(EM_SETHANDLE);
			ENTRY(EM_GETHANDLE);
			ENTRY(EM_GETTHUMB);
			ENTRY(EM_LINELENGTH);
			ENTRY(EM_REPLACESEL);
			// ENTRY(EM_SETFONT);
			ENTRY(EM_GETLINE);
			ENTRY(EM_LIMITTEXT);
			// ENTRY(EM_SETLIMITTEXT);
			ENTRY(EM_CANUNDO);
			ENTRY(EM_UNDO);
			ENTRY(EM_FMTLINES);
			ENTRY(EM_LINEFROMCHAR);
			// ENTRY(EM_SETWORDBREAK);
			ENTRY(EM_SETTABSTOPS);
			ENTRY(EM_SETPASSWORDCHAR);
			ENTRY(EM_EMPTYUNDOBUFFER);
			ENTRY(EM_GETFIRSTVISIBLELINE);
			ENTRY(EM_SETREADONLY);
			ENTRY(EM_SETWORDBREAKPROC);
			ENTRY(EM_GETWORDBREAKPROC);
			ENTRY(EM_GETPASSWORDCHAR);
			ENTRY(EM_SETMARGINS);
			ENTRY(EM_GETMARGINS);
			ENTRY(EM_GETLIMITTEXT);
			ENTRY(EM_POSFROMCHAR);
			ENTRY(EM_CHARFROMPOS);
			ENTRY(EM_SETIMESTATUS);
			ENTRY(EM_GETIMESTATUS);
			ENTRY(SBM_SETPOS);
			ENTRY(SBM_GETPOS);
			ENTRY(SBM_SETRANGE);
			ENTRY(SBM_GETRANGE);
			ENTRY(SBM_ENABLE_ARROWS);
			ENTRY(SBM_SETRANGEREDRAW);
			ENTRY(SBM_SETSCROLLINFO);
			ENTRY(SBM_GETSCROLLINFO);
			ENTRY(SBM_GETSCROLLBARINFO);
			ENTRY(WM_INPUT);
			ENTRY(WM_KEYDOWN);
			ENTRY(WM_KEYUP);
			ENTRY(WM_CHAR);
			ENTRY(WM_DEADCHAR);
			ENTRY(WM_SYSKEYDOWN);
			ENTRY(WM_SYSKEYUP);
			ENTRY(WM_SYSCHAR);
			ENTRY(WM_SYSDEADCHAR);
			ENTRY(WM_UNICHAR);
			// ENTRY(WM_WNT_CONVERTREQUESTEX);
			// ENTRY(WM_CONVERTREQUEST);
			// ENTRY(WM_CONVERTRESULT);
			// ENTRY(WM_INTERIM);
			ENTRY(WM_IME_STARTCOMPOSITION);
			ENTRY(WM_IME_ENDCOMPOSITION);
			ENTRY(WM_IME_COMPOSITION);
			ENTRY(WM_INITDIALOG);
			ENTRY(WM_COMMAND);
			ENTRY(WM_SYSCOMMAND);
			ENTRY(WM_TIMER);
			ENTRY(WM_HSCROLL);
			ENTRY(WM_VSCROLL);
			ENTRY(WM_INITMENU);
			ENTRY(WM_INITMENUPOPUP);
			// ENTRY(WM_SYSTIMER);
			ENTRY(WM_MENUSELECT);
			ENTRY(WM_MENUCHAR);
			ENTRY(WM_ENTERIDLE);
			ENTRY(WM_MENURBUTTONUP);
			ENTRY(WM_MENUDRAG);
			ENTRY(WM_MENUGETOBJECT);
			ENTRY(WM_UNINITMENUPOPUP);
			ENTRY(WM_MENUCOMMAND);
			ENTRY(WM_CHANGEUISTATE);
			ENTRY(WM_UPDATEUISTATE);
			ENTRY(WM_QUERYUISTATE);
			ENTRY(WM_CTLCOLORMSGBOX);
			ENTRY(WM_CTLCOLOREDIT);
			ENTRY(WM_CTLCOLORLISTBOX);
			ENTRY(WM_CTLCOLORBTN);
			ENTRY(WM_CTLCOLORDLG);
			ENTRY(WM_CTLCOLORSCROLLBAR);
			ENTRY(WM_CTLCOLORSTATIC);
			ENTRY(WM_MOUSEMOVE);
			ENTRY(WM_LBUTTONDOWN);
			ENTRY(WM_LBUTTONUP);
			ENTRY(WM_LBUTTONDBLCLK);
			ENTRY(WM_RBUTTONDOWN);
			ENTRY(WM_RBUTTONUP);
			ENTRY(WM_RBUTTONDBLCLK);
			ENTRY(WM_MBUTTONDOWN);
			ENTRY(WM_MBUTTONUP);
			ENTRY(WM_MBUTTONDBLCLK);
			ENTRY(WM_MOUSELAST);
			ENTRY(WM_MOUSEWHEEL);
			ENTRY(WM_XBUTTONDOWN);
			ENTRY(WM_XBUTTONUP);
			ENTRY(WM_XBUTTONDBLCLK);
			ENTRY(WM_PARENTNOTIFY);
			ENTRY(WM_ENTERMENULOOP);
			ENTRY(WM_EXITMENULOOP);
			ENTRY(WM_NEXTMENU);
			ENTRY(WM_SIZING);
			ENTRY(WM_CAPTURECHANGED);
			ENTRY(WM_MOVING);
			ENTRY(WM_POWERBROADCAST);
			ENTRY(WM_DEVICECHANGE);
			ENTRY(WM_MDICREATE);
			ENTRY(WM_MDIDESTROY);
			ENTRY(WM_MDIACTIVATE);
			ENTRY(WM_MDIRESTORE);
			ENTRY(WM_MDINEXT);
			ENTRY(WM_MDIMAXIMIZE);
			ENTRY(WM_MDITILE);
			ENTRY(WM_MDICASCADE);
			ENTRY(WM_MDIICONARRANGE);
			ENTRY(WM_MDIGETACTIVE);
			ENTRY(WM_MDISETMENU);
			ENTRY(WM_ENTERSIZEMOVE);
			ENTRY(WM_EXITSIZEMOVE);
			ENTRY(WM_DROPFILES);
			ENTRY(WM_MDIREFRESHMENU);
			// ENTRY(WM_IME_REPORT);
			ENTRY(WM_IME_SETCONTEXT);
			ENTRY(WM_IME_NOTIFY);
			ENTRY(WM_IME_CONTROL);
			ENTRY(WM_IME_COMPOSITIONFULL);
			ENTRY(WM_IME_SELECT);
			ENTRY(WM_IME_CHAR);
			ENTRY(WM_IME_REQUEST);
			// ENTRY(WM_IMEKEYDOWN);
			ENTRY(WM_IME_KEYDOWN);
			// ENTRY(WM_IMEKEYUP);
			ENTRY(WM_IME_KEYUP);
			ENTRY(WM_NCMOUSEHOVER);
			ENTRY(WM_MOUSEHOVER);
			ENTRY(WM_NCMOUSELEAVE);
			ENTRY(WM_MOUSELEAVE);
			ENTRY(WM_CUT);
			ENTRY(WM_COPY);
			ENTRY(WM_PASTE);
			ENTRY(WM_CLEAR);
			ENTRY(WM_UNDO);
			ENTRY(WM_RENDERFORMAT);
			ENTRY(WM_RENDERALLFORMATS);
			ENTRY(WM_DESTROYCLIPBOARD);
			ENTRY(WM_DRAWCLIPBOARD);
			ENTRY(WM_PAINTCLIPBOARD);
			ENTRY(WM_VSCROLLCLIPBOARD);
			ENTRY(WM_SIZECLIPBOARD);
			ENTRY(WM_ASKCBFORMATNAME);
			ENTRY(WM_CHANGECBCHAIN);
			ENTRY(WM_HSCROLLCLIPBOARD);
			ENTRY(WM_QUERYNEWPALETTE);
			ENTRY(WM_PALETTEISCHANGING);
			ENTRY(WM_PALETTECHANGED);
			ENTRY(WM_HOTKEY);
			ENTRY(WM_PRINT);
			ENTRY(WM_PRINTCLIENT);
			ENTRY(WM_APPCOMMAND);
			ENTRY(WM_HANDHELDFIRST);
			ENTRY(WM_HANDHELDLAST);
			ENTRY(WM_AFXFIRST);
			ENTRY(WM_AFXLAST);
			ENTRY(WM_PENWINFIRST);
			// ENTRY(WM_RCRESULT);
			// ENTRY(WM_HOOKRCRESULT);
			// ENTRY(WM_GLOBALRCCHANGE);
			// ENTRY(WM_PENMISCINFO);
			// ENTRY(WM_SKB);
			// ENTRY(WM_HEDITCTL);
			// ENTRY(WM_PENCTL);
			// ENTRY(WM_PENMISC);
			// ENTRY(WM_CTLINIT);
			// ENTRY(WM_PENEVENT);
			ENTRY(WM_PENWINLAST);

			// Our messages.
			ENTRY(WM_THREAD_SIGNAL);
		}

		if (msg >= WM_APP)
			return L">= WM_APP";
		if (msg >= WM_USER)
			return L">= WM_USER";

		return L"<unknown>";
	}
}