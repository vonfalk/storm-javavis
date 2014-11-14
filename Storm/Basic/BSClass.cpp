#include "stdafx.h"
#include "BSClass.h"

namespace storm {

	bs::Class::Class(Auto<Str> name, Auto<Str> content) {
		PLN(*name);
		PLN(*content);
	}

	bs::Tmp::Tmp() {}

	void bs::Tmp::add(Auto<Class> c) {}

}
