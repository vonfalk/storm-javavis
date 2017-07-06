#include "stdafx.h"
#include "Function.h"
#include "World.h"

const String Function::ctor = L"__init";
const String Function::dtor = L"__destroy";

Function::Function(const CppName &name, const String &pkg, Access access, const SrcPos &pos, Auto<TypeRef> result) :
	name(name), pkg(pkg), stormName(name.last()), access(access), pos(pos), result(result),
	isMember(false), isVirtual(false), isConst(false), wrapAssign(false), castMember(false), threadType(null) {}

void Function::resolveTypes(World &w, const CppName &ctx) {
	result = result->resolve(w, ctx);

	if (!thread.empty())
		threadType = w.threads.find(thread, ctx, pos);

	for (nat i = 0; i < params.size(); i++)
		params[i] = params[i]->resolve(w, ctx);
}

wostream &operator <<(wostream &to, const Function &fn) {
	if (fn.isVirtual)
		to << L"virtual ";

	to << fn.result << L" " << fn.name << L"(";
	join(to, fn.params, L", ");
	to << L")";

	if (fn.isConst)
		to << L" const";

	if (!fn.thread.empty())
		to << L"ON(" << fn.thread << L")";

	to << L";";
	return to;
}
