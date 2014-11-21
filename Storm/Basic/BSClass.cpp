#include "stdafx.h"
#include "BSClass.h"

namespace storm {

	bs::Class::Class(SrcPos pos, Auto<SStr> name, Auto<SStr> content) : Type(name->v->v, typeClass) {}

}
