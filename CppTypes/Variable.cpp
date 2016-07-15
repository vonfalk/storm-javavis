#include "stdafx.h"
#include "Variable.h"

Variable::Variable(const CppName &name, Auto<TypeRef> type, const SrcPos &pos) : name(name), type(type), pos(pos) {}

void Variable::resolveTypes(World &in, const CppName &context) {
	type = type->resolve(in, context);
}

wostream &operator <<(wostream &to, const Variable &v) {
	return to << v.type << L" " << v.name << L";";
}

