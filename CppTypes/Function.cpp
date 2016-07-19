#include "stdafx.h"
#include "Function.h"

const String Function::ctor = L"__init";
const String Function::dtor = L"__destroy";

Function::Function(const CppName &name, const SrcPos &pos, Auto<TypeRef> result) :
	name(name), pos(pos), result(result), isVirtual(false), isConst(false) {}

void Function::resolveTypes(World &w, CppName &ctx) {
	result = result->resolve(w, ctx);

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

	to << L";";
	return to;
}
