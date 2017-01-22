#include "stdafx.h"
#include "Std.h"
#include "BuiltInLoader.h"
#include "Lib/Int.h"
#include "Lib/Bool.h"
#include "Lib/Object.h"
#include "Shared/TObject.h"
#include "Lib/ArrayTemplate.h"
#include "Lib/FutureTemplate.h"
#include "Lib/FnPtrTemplate.h"
#include "Lib/MapTemplate.h"
#include "Lib/Maybe.h"
#include "Syntax/ParserTemplate.h"

// TODO: Can we remove any of these?
#include "Exception.h"
#include "Function.h"
#include "Engine.h"
#include "NamedThread.h"
#include "TypeCtor.h"
#include "Code/VTable.h"

namespace storm {

	void addStdLib(Engine &to, BuiltInLoader &loader) {
		// Place core types in the core package.
		Auto<SimpleName> coreName = CREATE(SimpleName, to, L"core");
		Package *core = to.package(coreName, true);
		addFnPtrTemplate(core); // needed early.
		addMaybeTemplate(core);
		addArrayTemplate(core);
		addMapTemplate(core);
		core->add(steal(cloneTemplate(to))); // also needed early

		core->add(steal(intType(to)));
		core->add(steal(natType(to)));
		core->add(steal(longType(to)));
		core->add(steal(wordType(to)));
		core->add(steal(byteType(to)));
		core->add(steal(floatType(to)));
		core->add(steal(boolType(to)));
		core->add(steal(futureTemplate(to)));

		syntax::addParserTemplate(to);

		loader.loadThreads();
		loader.finalizeTypes();
		loader.loadFunctions();
		loader.loadVariables();
	}

}
