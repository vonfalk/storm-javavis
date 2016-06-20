#include "stdafx.h"
#include "Variable.h"

Variable::Variable(const CppName &name, Auto<CppType> type, const SrcPos &pos) : name(name), type(type), pos(pos) {}

void Variable::resolveTypes(World &in) {
	type = type->resolve(in);
}

wostream &operator <<(wostream &to, const Variable &v) {
	return to << v.type << L" " << v.name << L";";
}

