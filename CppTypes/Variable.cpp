#include "stdafx.h"
#include "Variable.h"

Variable::Variable(const CppName &name, Auto<TypeRef> type, Access access, const SrcPos &pos)
	: name(name), type(type), pos(pos), access(access) {}

void Variable::resolveTypes(World &in, const CppName &context) {
	type = type->resolve(in, context);
}

wostream &operator <<(wostream &to, const Variable &v) {
	return to << v.type << L" " << v.name << L";";
}

