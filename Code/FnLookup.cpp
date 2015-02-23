#include "stdafx.h"
#include "FnLookup.h"

namespace code {

#ifdef WINDOWS
	// use wchar_t in DbgHelp functions.
#define DBGHELP_TRANSLATE_TCHAR
#include "Dbghelp.h"
#pragma comment (lib, "Dbghelp.lib")

	class DbgHelp : NoCopy {
	public:
		// Handle to our process.
		HANDLE process;

		// Initialized?
		bool initialized;

		// Initialize the DbgHelper library.
		DbgHelp() {
			process = GetCurrentProcess();
			SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
			if (SymInitialize(process, NULL, TRUE)) {
				initialized = true;
			} else {
				WARNING(L"Failed to initialize DbgHelp!");
				initialized = false;
			}
		}

		~DbgHelp() {
			if (initialized) {
				SymCleanup(process);
			}
		}
	};

	// Static instance (lazy).
	DbgHelp &dbgHelp() {
		static DbgHelp v;
		return v;
	}

	// Easy creation of a SYMBOL_INFO
	struct SymInfo : SYMBOL_INFO {
		SymInfo() {
			SizeOfStruct = sizeof(SYMBOL_INFO);
			MaxNameLen = maxSize;
		}

		enum { maxSize = 512 };
		// Rest of the name...
		wchar data[maxSize];
	};

	// Found somewhere, from CVCONST.H in DIA SDK (sic)
	enum BasicType {
		btNoType = 0,
		btVoid = 1,
		btChar = 2,
		btWChar = 3,
		btInt = 6,
		btUInt = 7,
		btFloat = 8,
		btBCD = 9,
		btBool = 10,
		btLong = 13,
		btULong = 14,
		btCurrency = 25,
		btDate = 26,
		btVariant = 27,
		btComplex = 28,
		btBit = 29,
		btBSTR = 30,
		btHresult = 31
	};

	// From MSDN.
	enum SymTagEnum {
		SymTagNull,
		SymTagExe,
		SymTagCompiland,
		SymTagCompilandDetails,
		SymTagCompilandEnv,
		SymTagFunction,
		SymTagBlock,
		SymTagData,
		SymTagAnnotation,
		SymTagLabel,
		SymTagPublicSymbol,
		SymTagUDT,
		SymTagEnum,
		SymTagFunctionType,
		SymTagPointerType,
		SymTagArrayType,
		SymTagBaseType,
		SymTagTypedef,
		SymTagBaseClass,
		SymTagFriend,
		SymTagFunctionArgType,
		SymTagFuncDebugStart,
		SymTagFuncDebugEnd,
		SymTagUsingNamespace,
		SymTagVTableShape,
		SymTagVTable,
		SymTagCustom,
		SymTagThunk,
		SymTagCustomType,
		SymTagManagedType,
		SymTagDimension,
		SymTagCallSite,
		SymTagInlineSite,
		SymTagBaseInterface,
		SymTagVectorType,
		SymTagMatrixType,
		SymTagHLSLType
	};

	static bool baseType(wostream &to, DWORD index) {
		switch (index) {
		case btVoid:
			to << L"Void";
			return true;
		case btChar:
			to << L"char";
			return true;
		case btWChar:
			to << L"wchar";
			return true;
		case btInt:
			to << L"int";
			return true;
		case btUInt:
			to << L"nat";
			return true;
		case btFloat:
			to << L"float";
			return true;
		case btBCD:
			to << L"BCD";
			return true;
		case btBool:
			to << L"Bool";
			return true;
		case btLong:
			to << L"long";
			return true;
		case btULong:
			to << L"ulong";
			return true;
		case btCurrency:
			to << L"Currency";
			return true;
		case btDate:
			to << L"Date";
			return true;
		case btVariant:
			to << L"Variant";
			return true;
		case btComplex:
			to << L"Complex";
			return true;
		case btBit:
			to << L"bit";
			return true;
		case btBSTR:
			to << L"BSTR";
			return true;
		case btHresult:
			to << L"HRESULT";
			return true;
		}
		return false;
	}


	static void outputSymbol(wostream &to, SymInfo &symbol);

	static void outputSymbol(wostream &to, ULONG64 base, DWORD index) {
		DbgHelp &h = dbgHelp();

		DWORD tag;
		SymGetTypeInfo(h.process, base, index, TI_GET_SYMTAG, &tag);
		DWORD tmp;

		switch (tag) {
		case SymTagBaseType:
			SymGetTypeInfo(h.process, base, index, TI_GET_BASETYPE, &tmp);
			baseType(to, tmp);
			break;
		case SymTagUDT: {
			SymInfo info;
			SymFromIndex(h.process, base, index, &info);
			outputSymbol(to, info);
			break;
		}
		case SymTagPointerType:
			SymGetTypeInfo(h.process, base, index, TI_GET_TYPEID, &tmp);
			outputSymbol(to, base, tmp);
			to << L"*";
		 	break;
		case SymTagFunctionArgType:
			SymGetTypeInfo(h.process, base, index, TI_GET_TYPEID, &tmp);
			outputSymbol(to, base, tmp);
			// There may be a name here somewhere!
			break;
			// ...
		default:
			to << L"<tag: " << tag << L">";
		}

	}

	// Output a symbol (with parameters).
	static void outputSymbol(wostream &to, SymInfo &symbol) {
		DbgHelp &h = dbgHelp();

		// Type of item:
		DWORD type;
		if (SymGetTypeInfo(h.process, symbol.ModBase, symbol.TypeIndex, TI_GET_TYPE, &type)) {
			outputSymbol(to, symbol.ModBase, type);
			to << L" ";
		}


		// Name
		to << symbol.Name;

		// Function? Should be SymTagFunction, but that is not defined...
		if (symbol.Tag != SymTagFunction)
			return;

		// Parameters.
		DWORD children = 0;
		SymGetTypeInfo(h.process, symbol.ModBase, symbol.TypeIndex, TI_GET_CHILDRENCOUNT, &children);
		to << L"(";

		byte *mem = new byte[sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG)*children];
		TI_FINDCHILDREN_PARAMS *params = (TI_FINDCHILDREN_PARAMS *)mem;
		params->Count = children;
		params->Start = 0;

		SymGetTypeInfo(h.process, symbol.ModBase, symbol.TypeIndex, TI_FINDCHILDREN, params);
		for (nat i = 0; i < nat(children); i++) {
			if (i != 0)
				to << L", ";
			outputSymbol(to, symbol.ModBase, params->ChildId[i]); //type);
		}

		delete []mem;

		to << L")";
	}

	String CppLookup::format(const StackFrame &frame) const {
		DbgHelp &h = dbgHelp();

		const nat maxNameLen = 512;
		SymInfo symbol;
		DWORD64 ptr = (DWORD64)frame.code;
		DWORD64 codeDisplacement;
		bool hasSymName = SymFromAddr(h.process, ptr, &codeDisplacement, &symbol) ? true : false;

		IMAGEHLP_LINE64 line;
		DWORD lineDisplacement;
		line.SizeOfStruct = sizeof(line);
		bool hasLine = SymGetLineFromAddr64(h.process, ptr, &lineDisplacement, &line) ? true : false;

		std::wostringstream r;

		if (hasLine)
			r << line.FileName << L"(" << line.LineNumber << L"): ";
		else
			r << L"<unknown location>: ";

		if (hasSymName)
			outputSymbol(r, symbol);
		else
			r << L"Unknown function @" << toHex(frame.code);

		return r.str();
	}

#else
#error "Please implement CppLookup::format for your OS here."
#endif


	ArenaLookup::ArenaLookup(Arena &arena) : arena(arena) {}

	String ArenaLookup::format(const StackFrame &frame) const {
		TODO(L"Implement me!");
		return CppLookup::format(frame);
	}

}
