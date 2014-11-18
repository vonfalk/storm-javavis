#include "stdafx.h"
#include "BSClass.h"

namespace storm {

	bs::Class::Class(SrcPos pos, Auto<SStr> name, Auto<SStr> content) {
		PLN(pos);
		PLN(*name);
		PLN(*content);
	}

	bs::Tmp::Tmp() {}

	void bs::Tmp::add(Auto<Class> c) {}

}
